#include "Player.hh"
#include <queue>
#include <set>
#include <stack>
#include <map>

/* Per executar:
./Game Demo Demo Demo Demo -s 30 -i default.cnf -o default.res
*/

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME duMMy

struct PLAYER_NAME : public Player {
    
    /**
     * Factory: returns a new instance of this class.
    * Do not modify this function.
    */
    static Player* factory () {
    return new PLAYER_NAME;
    }

    /**
    * Types and attributes for your player can be defined here.
    */
    //Constants
    const int INF = 1e9;
    //Maxima fraccio de CPU que es fa servir abans de triar moviment random
    const double MAX_STATUS = 0.99;
    //Maxima distancia a la qual el cotxe mirara per trobar soldats enemics
    const int CAR_RANGE = 30;
    //Maxima distancia de la carretera a la qual estara un cotxe en qualsevol moment
    const int MAX_DIST_ROAD = 3;
    //Distancia a la qual el cotxe mirara al voltant d'un soldat per trobar altres soldats a prop
    const int ACCUMULATION_RADIUS = 7;
    //Maxima distancia a la qual un soldat mirara per trobar cotxes enemics
    const int ENEMY_CAR_RANGE = 8;
    //Minima aigua a la qual intentara anar a buscar aigua
    const int MIN_WATER = 18;
    //Minim menjar al qual intentara anar a buscar menjar
    const int MIN_FOOD = 18;
    //Minim fuel al qual intentara anar a buscar fuel
    const int MIN_FUEL = 30;
    //Minimes ciutats amb les que em conformo si no vaig perdent
    const int MIN_CITIES_OWNED = 4;
    //Distancia a la qual el warrior intentara apropar-se a una ciutat
    const int MAX_DIST_CITY = 8;
    //Distancia a la qual el warrior intentara anar a una ciutat buida propera, com a primera opcio
    const int DIST_EMPTY_CITY = 3;
    
    // Typedefs
    typedef vector<int> VI;
    typedef vector<VI> VVI;
    typedef set<Pos> SP;
    typedef vector<Pos> VP;
    typedef vector<VP> VVP;
    typedef pair<int, Pos> IP;

    // Variables globals
    int num_cities_owned;
    VI my_warriors;
    VI my_cars;
    VI enemy_warriors;
    VI enemy_cars;
    SP already_moved;
    SP already_attacked;
    VVP all_cities;
    VI allies_in_city;
    VI enemies_in_city;
    VI allies_entering_city;
    VI allies_leaving_city;
    VI city_owner;
    VVI car_map;
    VVI warrior_map;
    VVI distance_from_water;
    VVI distance_from_fuel_weighted;
    VVI distance_from_fuel_not_weighted;
    VVI distance_from_city;
    VVI distance_from_road;
    VVI distance_from_enemy_car;

    template<typename T>
    class CompPairIP {
    public:
        bool operator()(const T& a, const T& b) {
            if (a.first > b.first) return true;
            if (a.first < b.first) return false;
            if (a.second.i < b.second.i) return true;
            if (a.second.i > b.second.i) return false;
            return a.second.j < b.second.j;
        }
    };

    //Funcions auxiliars
    /**
    * Returns info about units at a given cell
    *   0 means no unit there
    *   1 means friendly warrior
    *   2 means friendly car
    *   3 means enemy warrior
    *   4 means enemy car
    */
    int cell_unit(const Cell& c) {
        if (c.id == -1) return 0;
        Unit u = unit(c.id);
        if (u.player == me() and u.type == Warrior) return 1;
        if (u.player == me() and u.type == Car) return 2;
        if (u.type == Warrior) return 3;
        return 4;
    }
    
    //Returns allies in city - enemies in city
    int city_balance(int i) {
        return allies_in_city[i] + allies_entering_city[i] - allies_leaving_city[i] - enemies_in_city[i];
    }
    
    int enemy_cars_around(const Pos& p) {
        int count = 0;
        for (int i = 0; i < (int)enemy_cars.size(); i++) {
            Pos enemy = unit(enemy_cars[i]).pos;
            int dist = distance(p, enemy);
            if (distance(p, enemy) <= ENEMY_CAR_RANGE) count += 5*(ENEMY_CAR_RANGE-dist);
        }
        return count;
    }
    
    int enemy_warriors_around(const Pos& p) {
        int count = 0;
        for (int i = -ACCUMULATION_RADIUS; i <= ACCUMULATION_RADIUS; i++) {
            for (int j = -ACCUMULATION_RADIUS; j <= ACCUMULATION_RADIUS; j++) {
                Pos curr = Pos(p.i+i, p.j+j);
                if (pos_ok(curr) and cell_unit(cell(curr)) == 3) count += (cell(curr).type == City ? 1 : 3);
            }
        }
        return count;
    }
    
    int score(const Pos& p, bool car) {
        if (not car) return warrior_map[p.i][p.j] - enemy_cars_around(p);
        else return car_map[p.i][p.j] + enemy_warriors_around(p);
    }
    
    bool car_can_go(const Pos& p) {
        if (not pos_ok(p)) return false;
        Cell c = cell(p);
        return (c.type == Road or c.type == Desert);
    }
    
    bool warrior_can_go(const Pos& p) {
        if (not pos_ok(p)) return false;
        Cell c = cell(p);
        return (c.type == Road or c.type == Desert or c.type == City);
    }
    
    bool can_go(const Pos& p, bool car) {
        if (car) return car_can_go(p);
        else return warrior_can_go(p);
    }
    
    bool is_empty(const Cell& c) {
        return (c.id == -1);
    }
    
    bool is_friendly(const Cell& c) {
        int a = cell_unit(c);
        return (a == 1 or a == 2);
    }
    
    bool is_enemy(const Cell& c) {
        int a = cell_unit(c);
        return (a == 3 or a == 4);
    }
    
    bool is_warrior(const Cell& c) {
        int a = cell_unit(c);
        return (a == 1 or a == 3);
    }
    
    bool is_car(const Cell & c) {
        int a = cell_unit(c);
        return (a == 2 or a == 4);
    }

    void move(int id, Dir d) {
        Pos original = unit(id).pos;
        Pos p = original + d;
        Cell attacked = cell(p);
        command(id, d);
        if (unit(id).type == Car or cell_unit(attacked) != 3 or min(unit(attacked.id).food, unit(attacked.id).water) <= 6) {
            already_moved.insert(p);
        }
        if (unit(id).type == Warrior) {
            Cell c1 = cell(original);
            Cell c2 = cell(p);
            if (c1.type == City and c2.type != City) {
                int n = num_city(original);
                allies_leaving_city[n]++;
            }
            else if (c1.type != City and c2.type == City) {
                int n = num_city(p);
                allies_entering_city[n]++;
            }
        }
    }
    
    int distance(const Pos& p1, const Pos& p2) {
        return max(abs(p2.i-p1.i), abs(p2.j-p1.j));
    }
    
    bool is_safe(const Pos& p, bool car) {
        //Cannot move there
        if (not pos_ok(p)) return false;
        if (car and not car_can_go(p)) return false;
        if (not car and not warrior_can_go(p)) return false;
        
        //No friendly unit there
        if (is_friendly(cell(p))) return false;
        if (already_moved.count(p)) return false;
        
        //No car in the immediate surroundings
        if (car and distance_from_enemy_car[p.i][p.j] < 2) return false;
        if (cell(p).type != City and not car and distance_from_enemy_car[p.i][p.j] < INF) return false;
        
        return true;
    }
    
    bool is_safe_escape(const Pos& p) {
        //Cannot move there
        if (not pos_ok(p)) return false;
        if (not warrior_can_go(p)) return false;
        
        //No friendly unit there
        if (is_friendly(cell(p))) return false;
        if (already_moved.count(p)) return false;
        
        return true;
    }
    
    Dir most_improved_direction(const Pos& start, bool car) {
        Dir best_direction = None;
        int best_score = score(start, car);
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p = start + d;
            if (is_safe(p, car)) {
                int curr_score = score(p, car);
                if (best_direction == None or (curr_score > best_score or (curr_score == best_score and random(0, 1)))) {
                    best_direction = d;
                    best_score = curr_score;
                }
            }
        }
        return best_direction;
    }
    
    bool is_better(int curr_dist, int curr_acc, int best_dist, int best_acc) {
        if (curr_dist <= best_dist and curr_acc >= best_acc) return true;
        if (curr_acc >= 2*best_acc and curr_dist < 2*best_dist) return true;
        return false;
    }
    
    bool already_attacked_near(const Pos& p) {
        for (Pos q : already_attacked) {
            if (distance(p, q) < ACCUMULATION_RADIUS) return true;
        }
        return false;
    }
    
    //Busca el soldat enemic mes proper / mes beneficios i troba la direccio apropiada per aproparse
    Dir dijkstra(const Pos& start) {
        priority_queue<IP, vector<IP>, CompPairIP<IP>> PQ;
        VVI distances(60, VI(60, INF));
        PQ.push(make_pair(0, start));
        distances[start.i][start.j] = 0;
        bool cont = true;
        
        Pos best(-1, -1);
        int best_acc = 0;
        
        map<Pos, Pos> pare;
        pare[start] = start;
        
        while (not PQ.empty()) {
            Pos p = PQ.top().second;
            int dist = PQ.top().first; 
            PQ.pop();
            if (dist == distances[p.i][p.j]) {
                if (cell_unit(cell(p)) == 3 and not already_attacked_near(p)) {
                    int curr_acc = enemy_warriors_around(p);
                    if (distance_from_road[p.i][p.j] < MAX_DIST_ROAD) {
                        if (not pos_ok(best) or is_better(dist, curr_acc, distances[best.i][best.j], best_acc)) {
                            best_acc = curr_acc;
                            best = p;
                        }
                    }
                }
                
                int cost = (cell(p).type == Road ? 1 : 4);
                if (cell(start).type == Road and pare[p] == start) {
                    int my = me();
                    int r = (round()+1)%4;
                    if (r == my) cost = 1;
                }
                for (int i = 0; i < 8; i++) {
                    Dir d = Dir(i);
                    Pos p2 = p+d;
                    int curr_dist = dist + cost;
                    if (pos_ok(p2) and car_can_go(p2) and curr_dist < CAR_RANGE and (is_safe(p2, true) or distance(start, p2) > 2)) {
                        if (curr_dist < distances[p2.i][p2.j]) {
                            PQ.push(make_pair(curr_dist, p2));
                            distances[p2.i][p2.j] = curr_dist;
                            pare[p2] = p;
                        }
                    }
                }
            }
        }
        if (not pos_ok(best)) return None;
        
        already_attacked.insert(best);
        Pos p = best;
        while (pare[p] != start) {
            p = pare[p];
        }

        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            if (start+d == p) return d;
        }
        return None;
    }

    //Hauria de retornar una pos i despres fer bfs o dijkstra alli
    Dir find_city_to_conquer(const Pos& start) {
        int min_dist = INF;
        Pos best_pos(-1, -1);
        for (int i = 0; i < (int)all_cities.size(); i++) {
            for (int j = 0; j < (int)all_cities[i].size(); j++) {
                Pos p = all_cities[i][j];
                int dist = distance(start, p);
                if ((cell(p).owner != me()) and dist < min_dist) {
                    min_dist = dist;
                    best_pos = p;
                }
            }
        }
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p = start + d;
            if (is_safe(p, false) and distance(p, best_pos) < min_dist) {
                return d;
            }
        }
        return None;
    }

    Dir adjacent_enemy(const Pos& start, bool car) {
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p = start+d;
            if ((not car or is_safe(p, true)) and cell_unit(cell(p)) == 3 and (not car or cell(p).type != City)) return d;
        }
        return None;
    }
    
    Dir find_nearest_water(const Pos& start) {
        int start_dist = distance_from_water[start.i][start.j];
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p2 = start+d;
            if (is_safe(p2, false) and distance_from_water[p2.i][p2.j] < start_dist) return d;
        }        
        return None;
    }
    
    Dir find_nearest_fuel(const Pos& start, bool hasfuel) {
        int best_dist = INF;
        Dir best_dir = None;
        bool going_to_road = false;
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p2 = start+d;
            int curr_dist = -1; //per inicialitzar correctament
            if (pos_ok(p2) and hasfuel) curr_dist = distance_from_fuel_weighted[p2.i][p2.j];
            else if (pos_ok(p2)) curr_dist = distance_from_fuel_not_weighted[p2.i][p2.j];
            bool better = false;
            if (cell(p2).type == Road and curr_dist < best_dist) better = true;
            else if (cell(p2).type == Road and not going_to_road and curr_dist <= best_dist+1) better = true;
            else if (curr_dist < best_dist) better = true;
            if (pos_ok(p2) and is_safe(p2, true) and better) {
                best_dist = curr_dist;
                best_dir = d;
                going_to_road = (cell(p2).type == Road);
            }
        }        
        return best_dir;
    }
    
    Dir find_nearest_city(const Pos& start) {
        int start_dist = distance_from_city[start.i][start.j];
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p2 = start+d;
            if (is_safe(p2, true) and distance_from_city[p2.i][p2.j] < start_dist) return d;
        }        
        return None;
    }
    
    Pos find_nearest_friendly_car(const Pos& start) {
        int min_distance = 1e9;
        Pos best_pos(-1, -1);
        for (int id : my_cars) {
            Pos p = unit(id).pos;
            int d = distance(start, p);
            if (d != 0 and d < min_distance) {
                min_distance = d;
                best_pos = p;
            }
        }
        return best_pos;
    }
    
    Pos find_nearest_enemy_car(const Pos& start) {
        int min_distance = 1e9;
        Pos best_pos(-1, -1);
        for (int id : enemy_cars) {
            Pos p = unit(id).pos;
            int d = distance(start, p);
            if (d < min_distance) {
                min_distance = d;
                best_pos = p;
            }
        }
        return best_pos;
    }
   
    
    Pos find_nearest_enemy_warrior(const Pos& start) {
        int min_distance = 1e9;
        Pos best_pos(-1, -1);
        for (int id : enemy_warriors) {
            Pos p = unit(id).pos;
            int d = distance(start, p);
            if (d < min_distance and cell(p).type != City) {
                min_distance = d;
                best_pos = p;
            }
        }
        return best_pos;
    }
    
    void bfs_water(const Pos& start) {
        VVI used(60, VI(60, false));
        queue<pair<int, Pos>> Q;
        Q.push(make_pair(0, start));
        used[start.i][start.j] = true;
        while (not Q.empty()) {
            Pos p = Q.front().second;
            int dist = Q.front().first;
            Q.pop();
            distance_from_water[p.i][p.j] = min(distance_from_water[p.i][p.j], dist);
            for (int i = 0; i < 8; i++) {
                Dir d = Dir(i);
                Pos p2 = p+d;
                if (pos_ok(p2) and warrior_can_go(p2) and not used[p2.i][p2.j]) {
                    Q.push(make_pair(dist+1, p2));
                    used[p2.i][p2.j] = true;
                }
            }
        }
    }
    
    void bfs_city(const Pos& start) {
        VVI used(60, VI(60, false));
        queue<pair<int, Pos>> Q;
        Q.push(make_pair(0, start));
        used[start.i][start.j] = true;
        while (not Q.empty()) {
            Pos p = Q.front().second;
            int dist = Q.front().first;
            Q.pop();
            distance_from_city[p.i][p.j] = min(distance_from_city[p.i][p.j], dist);
            for (int i = 0; i < 8; i++) {
                Dir d = Dir(i);
                Pos p2 = p+d;
                if (pos_ok(p2) and warrior_can_go(p2) and not used[p2.i][p2.j]) {
                    Q.push(make_pair(dist+1, p2));
                    used[p2.i][p2.j] = true;
                }
            }
        }
    }
    
    void bfs_fuel_weighted(const Pos& start) {
        queue<pair<int, Pos>> Q;
        Q.push(make_pair(0, start));
        distance_from_fuel_weighted[start.i][start.j] = 0;
        while (not Q.empty()) {
            Pos p = Q.front().second;
            int dist = Q.front().first;
            Q.pop();
            int cost = (cell(p).type == Road ? 1 : 4);
            for (int i = 0; i < 8; i++) {
                Dir d = Dir(i);
                Pos p2 = p+d;
                if (pos_ok(p2) and car_can_go(p2) and dist+cost < distance_from_fuel_weighted[p2.i][p2.j]) {
                    Q.push(make_pair(dist+cost, p2));
                    distance_from_fuel_weighted[p2.i][p2.j] = dist+cost;
                }
            }
        }
    }
    
    void bfs_fuel_not_weighted(const Pos& start) {
        VVI used(60, VI(60, false));
        queue<pair<int, Pos>> Q;
        Q.push(make_pair(0, start));
        used[start.i][start.j] = true;
        while (not Q.empty()) {
            Pos p = Q.front().second;
            int dist = Q.front().first;
            Q.pop();
            distance_from_fuel_not_weighted[p.i][p.j] = min(distance_from_fuel_not_weighted[p.i][p.j], dist);
            for (int i = 0; i < 8; i++) {
                Dir d = Dir(i);
                Pos p2 = p+d;
                if (pos_ok(p2) and car_can_go(p2) and not used[p2.i][p2.j]) {
                    Q.push(make_pair(dist+1, p2));
                    used[p2.i][p2.j] = true;
                }
            }
        }
    }
    
    void bfs_road(const Pos& start) {
        VVI used(60, VI(60, false));
        queue<pair<int, Pos>> Q;
        Q.push(make_pair(0, start));
        used[start.i][start.j] = true;
        while (not Q.empty()) {
            Pos p = Q.front().second;
            int dist = Q.front().first;
            Q.pop();
            distance_from_road[p.i][p.j] = min(distance_from_road[p.i][p.j], dist);
            for (int i = 0; i < 8; i++) {
                Dir d = Dir(i);
                Pos p2 = p+d;
                if (pos_ok(p2) and car_can_go(p2) and not used[p2.i][p2.j]) {
                    Q.push(make_pair(dist+1, p2));
                    used[p2.i][p2.j] = true;
                }
            }
        }
    }
    
    void initialize_distances() {
        distance_from_water = distance_from_fuel_weighted = distance_from_fuel_not_weighted = distance_from_city = distance_from_road = VVI(60, VI(60, INF));
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                Cell c = cell(i, j);
                if (c.type == Water) {
                    bfs_water(Pos(i, j));
                }
                else if (c.type == Station) {
                    bfs_fuel_not_weighted(Pos(i, j));
                    bfs_fuel_weighted(Pos(i, j));
                }
                else if (c.type == City) {
                    bfs_city(Pos(i, j));
                }
                else if (c.type == Road) {
                    bfs_road(Pos(i, j));
                }
            }
        }
    }
    
    VI initialize_enemy_warriors() {
        VI v(80 - my_warriors.size());
        int j = 0;
        for (int i = 1; i < 4; i++) {
            VI v2 = warriors((me()+i)%4);
            for (int k = 0; k < (int)v2.size(); k++) {
                v[j] = v2[k];
                j++;
            }
        } 
        return v;
    }
    
    VI initialize_enemy_cars() {
        VI v(12 - my_cars.size());
        int j = 0;
        for (int i = 1; i < 4; i++) {
            VI v2 = cars((me()+i)%4);
            for (int k = 0; k < (int)v2.size(); k++) {
                v[j] = v2[k];
                j++;
            }
        } 
        return v;
    }
    
    void initialize_distances_to_cars() {
        int RANGE = ENEMY_CAR_RANGE;
        if (round()%4 != me()) RANGE = 3;
        distance_from_enemy_car = VVI(60, VI(60, INF));
        queue<IP> Q;
        for (int i = 0; i < (int)enemy_cars.size(); i++) {
            Pos car = unit(enemy_cars[i]).pos;
            Q.push(IP(0, car));
            distance_from_enemy_car[car.i][car.j] = 0;
        }
        while (not Q.empty()) {
            Pos p = Q.front().second;
            int dist = Q.front().first;
            Q.pop();
            int cost = (cell(p).type == Road ? 1 : 4);
            if (dist == 0 and distance_from_road[p.i][p.j] == 1) cost = 1;
            for (int i = 0; i < 8; i++) {
                Dir d = Dir(i);
                Pos p2 = p+d;
                if (pos_ok(p2) and dist+cost < distance_from_enemy_car[p2.i][p2.j] and dist+cost < RANGE) {
                    Q.push(IP(dist+cost, p2));
                    distance_from_enemy_car[p2.i][p2.j] = dist+cost;
                }
            }
        }
    }
    
    void initialize_cities() {
        VVI used(60, VI(60, false));
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                if (not used[i][j]) {
                    used[i][j] = true;
                    if (cell(i, j).type == City) {
                        vector<Pos> v;
                        queue<Pos> q;
                        q.push(Pos(i, j));
                        used[i][j] = true;
                        while (not q.empty()) {
                            Pos p = q.front();
                            q.pop();
                            if (cell(p).type == City) {
                                v.push_back(p);
                                for (int i = 0; i < 8; i++) {
                                    Dir d = Dir(i);
                                    Pos p2 = p+d;
                                    if (pos_ok(p2) and not used[p2.i][p2.j]) {
                                        q.push(p2);
                                        used[p2.i][p2.j] = true;
                                    }
                                }
                            }
                        }
                        all_cities.push_back(v);
                    }
                }
            }
        }
    }
    
    void initialize_city_stats() {
        num_cities_owned = 0;
        city_owner = VI(8, false);
        allies_in_city = VI(8, 0);
        enemies_in_city = VI(8, 0);
        allies_entering_city = VI(8, 0);
        allies_leaving_city = VI(8, 0);
        for (int i = 0; i < (int)all_cities.size(); i++) {
            if (cell(all_cities[i][0]).owner == me()) {
                city_owner[i] = true;
                num_cities_owned++;
            }
            for (int j = 0; j < (int)all_cities[i].size(); j++) {
                int u = cell_unit(cell(all_cities[i][j]));
                if (u == 1) allies_in_city[i]++;
                else if (u == 3) enemies_in_city[i]++;
            }
        }
    }
    
    void initialize_map() {
        warrior_map = car_map = VVI(60, VI(60, 0));
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                Pos here = Pos(i, j);
                Cell p = cell(here);
                int& w = warrior_map[i][j];
                int& c = car_map[i][j];
                
                int dist_city = distance_from_city[i][j];
                int dist_fuel = distance_from_fuel_weighted[i][j];
                int dist_water = distance_from_water[i][j];
                int dist_road = distance_from_road[i][j];
                
                if (not warrior_can_go(here)) w = -1;
                else {
                    if (p.type == City) w += 50;
                    if (dist_water < 8) w += 20-dist_water;
                    if (dist_city < 8) w += 20-dist_city;
                    if (dist_water + dist_city < 12) w += 50;
                }
                
                if (not car_can_go(here)) c = -1;
                else {
                    c += 100*(5 - dist_road);
                    if (dist_water + dist_city < 16) c += 100;
                    c += (30-distance(here, Pos(30, 30)));
                }
            }
        }
    }
    
    //pre: p is in a city
    //returns number 0-7 with the number of the city it belongs to
    int num_city(const Pos& p) {
        for (int i = 0; i < (int)all_cities.size(); i++) {
            for (int j = 0; j < (int)all_cities[i].size(); j++) {
                if (p == all_cities[i][j]) return i;
            }
        }
        return -1;
    }
    
    bool calculate_aggressive() {
        //Si anem perdent
        int my = total_score(me());
        for (int i = 1; i < 4; i++) {
            if (total_score((me()+i)%4) > my and round() > 200) return true;
        }
        
        //Si tenim menys d'un cert nombre de ciutats ciutats
        if (num_cities_owned < MIN_CITIES_OWNED and round() > 100 and round() < 400) return true;
        
        return false;
    }
    
    Dir nearby_empty_city(const Pos& p) {
        int min_dist = INF;
        Pos best_pos(-1, -1);
        for (int i = 0; i < (int)all_cities.size(); i++) {
            if (allies_in_city[i] == 0 and enemies_in_city[i] == 0) {
                for (int j = 0; j < (int)all_cities[i].size(); j++) {
                    Pos city = all_cities[i][j];
                    int dist = distance(p, city);
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_pos = city;
                    }
                }
            }
        }
        if (min_dist < DIST_EMPTY_CITY) {
            for (int i = 0; i < 8; i++) {
                Dir d = Dir(i);
                Pos p2 = p + d;
                int dist = distance(p2, best_pos);
                if (is_safe(p2, false) and dist < min_dist) return d;
            }
        }
        return None;
    }
    
    Dir escape_from_car(const Pos& me) {
        int dist_car = distance_from_enemy_car[me.i][me.j];
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p = me + d;
            if (pos_ok(p) and cell(p).type == City) return d;
            if (pos_ok(p) and is_safe_escape(p) and distance_from_enemy_car[p.i][p.j] > dist_car) return d;
        }
        return None;
    }

    Dir find_enemy_in_city(const Pos& start, int n_city) {
        int best_dist = INF;
        Pos best_pos(-1, -1);
        for (Pos p : all_cities[n_city]) {
            if (cell_unit(cell(p)) == 3) {
                int dist = distance(start, p);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_pos = p;
                }
            }
        }
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p = start+d;
            if (pos_ok(p)) {
                Cell c = cell(p);
                if (c.type == City and is_safe(p, false) and distance(p, best_pos) < best_dist) return d;
            }
        }
        return None;
    }
    
    Dir fight(const Pos& me, const Dir& other) {
        return other; //TO DO, need to find something that works better than this
    }

    void move_warriors() {
        if (round()%4 != me()) return; //no es el meu torn
        bool aggressive = calculate_aggressive();
        
        //Per cada warrior que tinc
        for (int w = 0; w < (int)my_warriors.size(); w++) {
            //cerr << "Warrior " << w << " (id " << my_warriors[w] << ")" << endl;
            Unit curr = unit(my_warriors[w]);
            bool moved = false;
            
            if (status(me()) < MAX_STATUS) {
                //Prioritat 1: Si estem a prop d'una ciutat buida

                //cerr << "  Empty city" << endl;
                if (curr.water >= MIN_WATER and cell(curr.pos).type != City) {
                    Dir d = nearby_empty_city(curr.pos);
                    if (d != None) {
                        //cerr << "  Moved to empty city from " << curr.pos << endl;
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
                //cerr << "  End empty city" << endl;
                
                //Prioritat 2: Si estem massa a prop d'un cotxe i som fora d'una ciutat
                
                //cerr << "  Escape" << endl;
                int dist_car = distance_from_enemy_car[curr.pos.i][curr.pos.j];
                if (not moved and dist_car < INF) {
                    if (cell(curr.pos).type != City) {
                        Dir d = escape_from_car(curr.pos);
                        if (d != None) {
                            //cerr << "  Moved to escape from car" << endl;
                            move(my_warriors[w], d);
                            moved = true;
                        }
                    }
                }
                //cerr << "  End escape" << endl;
                
                //Prioritat 3: Si estem a prop d'un soldat enemic
                
                //TO DO: Millorar fight AI
                //if an enemy soldier is next to you
                //cerr << "  Fight" << endl;
                Dir adj_enemy = adjacent_enemy(curr.pos, false);
                if (not moved and adj_enemy != None) {
                    Dir d = fight(curr.pos, adj_enemy);
                    move(my_warriors[w], d);
                    moved = true;
                }
                //cerr << "  End fight" << endl;
                
                //Prioritat 4: Si tenim set
                
                //if you need water - aquesta part ja esta be
                //cerr << "  Water" << endl;
                if (not moved and curr.water < MIN_WATER) {
                    Dir d = find_nearest_water(curr.pos);
                    if (d != None) {
                        //cerr << "  Moved to find water" << endl;
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
                //cerr << "  End water" << endl;
                
                //Prioritat 5: Si tenim gana
                
                if (not moved and curr.food < MIN_FOOD) {
                    Dir d = find_nearest_city(curr.pos);
                    if (d != None) {
                        //cerr << "  Moved to find food" << endl
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
                
                //Prioritat 6: Si estem dins d'una ciutat 
                
                if (not moved and cell(curr.pos).type == City) {
                    int n_city = num_city(curr.pos);
                    if (city_balance(n_city) > 0 and enemies_in_city[n_city] > 0) {
                        Dir d = find_enemy_in_city(curr.pos, n_city);
                        //cerr << "  Moved to find enemy in city" << endl;
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
                
                //Prioritat 7: Si estem en mode agressiu i volem conquerir
                
                //cerr << "  Conquer" << endl;
                if (not moved and (aggressive or distance_from_city[curr.pos.i][curr.pos.j] > MAX_DIST_CITY)) {
                    int n_city = -1;
                    if (cell(curr.pos).type == City) n_city = num_city(curr.pos);
                    int allies_city_n = allies_in_city[n_city] + allies_entering_city[n_city] - allies_leaving_city[n_city];
                    if (cell(curr.pos).type != City or allies_city_n > max(2, enemies_in_city[n_city] + 1)) {
                        //anem a conquerir una ciutat a prop
                        Dir d = find_city_to_conquer(curr.pos);
                        if (d != None) {
                            //cerr << "  Moved to conquer city" << endl;
                            move(my_warriors[w], d);
                            moved = true;
                        }
                    }
                }
                //cerr << "  End conquer" << endl;
            }
            
            //Si no hem mogut en el torn
            if (not moved) {
                //cerr << "  Moved to most improved direction" << endl;
                Dir d_improve = most_improved_direction(curr.pos, false);
                move(my_warriors[w], d_improve);
            }
        }
    }
    
    //De moment els deixo aixi, no cal millorar mes fins que els soldats siguin decents
    void move_cars() {
        //Per cada cotxe que tinc
        for (int c = 0; c < (int)my_cars.size(); c++) {
            //cerr << "Car " << my_cars[c] << endl;
            Unit curr = unit(my_cars[c]);
            //cerr << "At position " << curr.pos << endl;
            //cerr << "Current fuel: " << curr.food << endl;
            bool moved = false;
            //si es pot moure aquest torn
            if (can_move(my_cars[c])) {
                if (status(me()) < MAX_STATUS) {
                    //Si no tenim fuel anem a buscar-ne
                    if (curr.food < MIN_FUEL) {
                        //si tenim un warrior enemic adjacent
                        Dir adj = adjacent_enemy(curr.pos, true);
                        if (adj != None) {
                            //cerr << "  Attacked an adjacent enemy warrior" << endl;
                            move(my_cars[c], adj);
                            moved = true;
                        }
                        else {
                            bool hasfuel = curr.food != 0;
                            Dir d = find_nearest_fuel(curr.pos, hasfuel);
                            if (d != None) {
                                //cerr << "  Looking for fuel" << endl;
                                move(my_cars[c], d);
                                moved = true;
                            }
                        }
                    }
                    if (not moved) {
                        Dir d = dijkstra(curr.pos);
                        if (d != None) {
                            //cerr << "  Looking for enemy warrior" << endl;
                            move(my_cars[c], d);
                            moved = true;
                        }
                    }
                }
                if (not moved) {
                    //cerr << "  Moving in most improved direction" << endl;
                    Dir d_improve = most_improved_direction(curr.pos, true);
                    move(my_cars[c], d_improve);
                }
            }
        }
    } 

    /**
    * Play method, invoked once per each round.
    */
    virtual void play () {
        //Preliminars ronda 0
        if (round() == 0) {
            initialize_cities();
            initialize_distances();
            initialize_map();
        }
        
        //Preliminars cada ronda
        my_warriors = warriors(me());
        enemy_warriors = initialize_enemy_warriors();
        my_cars = cars(me());
        enemy_cars = initialize_enemy_cars();
        initialize_distances_to_cars();
        already_moved = already_attacked = SP();
        initialize_city_stats();
        
        //Principal
        move_cars();
        move_warriors();
        
        //Debug
        /*
        if (round() == 499) {
            cerr << "These are the cities: " << endl;
            for (int i = 0; i < (int)all_cities.size(); i++) {
                cerr << "City " << i << endl;
                for (int j = 0; j < (int)all_cities[i].size(); j++) {
                    cerr << "  " << all_cities[i][j] << endl;
                }
            }
        }
        */
        
        /*
        if (round() == 499) {
            cerr << "Distances from water: " << endl;
            for (int i = 0; i < 60; i++) {
                for (int j = 0; j < 60; j++) {
                    cerr << distance_from_water[i][j] << " ";
                }
                cerr << endl;
            }
        }
        */
        
        /*
        if (round() == 499) {
            cerr << "Distances from fuel: " << endl;
            for (int i = 0; i < 60; i++) {
                for (int j = 0; j < 60; j++) {
                    if (distance_from_fuel[i][j] == INF) cerr << "NO ";
                    else {
                        if (distance_from_fuel[i][j] < 10) cerr << 0;
                        cerr << distance_from_fuel[i][j] << " ";
                    }
                }
                cerr << endl;
            }
        }
        */
        
        /*
        if (round() == 499) {
            cerr << endl << "Warrior map:" << endl;
            for (int i = 0; i < 60; i++) {
                for (int j = 0; j < 60; j++) {
                    cerr << warrior_map[i][j] << " ";
                }
                cerr << endl;
            }
        }
        
        if (round() == 499) {
            cerr << endl << "Car map:" << endl;
            for (int i = 0; i < 60; i++) {
                for (int j = 0; j < 60; j++) {
                    cerr << car_map[i][j] << " ";
                }
                cerr << endl;
            }
        }
        */
        
    }
};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

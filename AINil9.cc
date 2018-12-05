#include "Player.hh"
#include <queue>
#include <set>
#include <stack>

/* Per executar:
./Game Demo Demo Demo Demo -s 30 -i default.cnf -o default.res
*/

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Nil9


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
    // Maxima fraccio de CPU que es fa servir abans de triar moviment random
    const double MAX_STATUS = 0.99;
    // Maxima distancia a la qual el cotxe mirara per trobar soldats enemics
    const int CAR_DETECT_RANGE = 45;
    // Maxima distancia a la qual el cotxe mirara per atacar soldats enemics
    const int CAR_ATTACK_RANGE = 30;
    //Maxima distancia a la qual un soldat mirara per trobar cotxes enemics
    const int ENEMY_CAR_RANGE = 2;
    //Minima aigua a la qual intentara anar a buscar aigua
    const int MIN_WATER = 16;
    //Minim menjar al qual intentara anar a buscar menjar
    const int MIN_FOOD = 12;
    //Minim fuel al qual intentara anar a buscar fuel
    const int MIN_FUEL = 25;
    
    // Typedefs
    typedef vector<int> VI;
    typedef vector<VI> VVI;
    typedef set<Pos> SP;
    typedef vector<Pos> VP;
    typedef vector<VP> VVP;
    typedef pair<int, Pos> IP;

    // Variables globals
    VI my_warriors;
    VI my_cars;
    VI enemy_warriors;
    VI enemy_cars;
    SP already_moved;
    VVP all_cities;
    VVI car_map;
    VVI warrior_map;
    VVI distance_from_water;
    VVI distance_from_fuel;
    VVI distance_from_city;

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
    
    int enemy_cars_around(const Pos& p) {
        int count = 0;
        for (int i = 0; i < (int)enemy_cars.size(); i++) {
            Pos enemy = unit(enemy_cars[i]).pos;
            int dist = distance(p, enemy);
            if (distance(p, enemy) <= ENEMY_CAR_RANGE) count += 5*(ENEMY_CAR_RANGE-dist);
        }
        return count;
    }
    
    //returns weighted sum of warriors around pos p by distance to p
    //warriors in a city count significantly less
    int enemy_warriors_around(const Pos& p) {
        int count = 0;
        for (int i = 0; i < (int)enemy_warriors.size(); i++) {
            Pos enemy = unit(enemy_warriors[i]).pos;
            int dist = distance(p, enemy);
            if (dist <= CAR_DETECT_RANGE) {
                if (cell(enemy).type == City) count += (CAR_DETECT_RANGE-dist)/5;
                else count += (CAR_DETECT_RANGE-dist)/2;
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
    
    bool aggressive() {
        if (round() >= 400) {
            int my = total_score(me());
            if (not (my > total_score((me()+1)%4))) return true;
            if (not (my > total_score((me()+2)%4))) return true;
            if (not (my > total_score((me()+3)%4))) return true;
        }
        return false;
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
        Pos p = unit(id).pos;
        p += d;
        command(id, d);
        already_moved.insert(p);
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
        Pos c = find_nearest_enemy_car(p);
        if (car and distance(p, c) < 2) return false;
        if (not car and distance(p, c) < ENEMY_CAR_RANGE) return false;
        
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
    
    Dir find_direction_bfs(const Pos& start, const Pos& end, bool car) {
        priority_queue<IP, vector<IP>, CompPairIP<IP>> Q;
        VVI dist(60, VI(60, INF));
        map<Pos, Pos> pare;
        Q.push(make_pair(0, start));
        pare[start] = start;
        dist[start.i][start.j] = 0;
        bool cont = true;
        while (not Q.empty() and cont) {
            Pos p = Q.top().second;
            int d = Q.top().first; 
            Q.pop();
            if (d == dist[p.i][p.j] and d < 15) {
                if (p == end) cont = false;
                else {
                    for (int i = 0; i < 8; i++) {
                        Dir di = Dir(i);
                        Pos p2 = p+di;
                        if (pos_ok(p2) and can_go(p2, car) and (distance(start, p2) > 2 or is_safe(p2, car))) {
                            int cost = 1;
                            if (car and cell(p2).type != Road) cost = 4;
                            if (d + cost < dist[p2.i][p2.j]) {
                                Q.push(make_pair(d+cost, p2));
                                dist[p2.i][p2.j] = d+cost;
                                pare[p2] = p;
                            }
                        }
                    }
                }
            }
        }
        if (cont) return None;
        Pos p = end;
        while (pare[p] != start) p = pare[p];
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            if (start+d == p) return d;
        }
        return None;
    }

    //Aquesta funcio sera un Dijkstra
    //De moment es una merda
    Dir find_direction(const Pos& start, const Pos& end, bool car) {
        int initial_dist = distance(start, end);
        int best_dist = initial_dist;
        int best_score = score(start, car);
        Dir best_dir = None;
        for (int i = 0; i < 8; i++) {
            Dir curr = Dir(i);
            Pos curr_pos = start+curr;
            if (pos_ok(curr_pos)) {
                int curr_score = score(curr_pos, car);
                int curr_dist = distance(start+curr, end);
                if (curr_dist == 0 and is_safe(curr_pos, car)) return curr;
                if (is_safe(curr_pos, car)) {
                    if (car) {
                        if (curr_dist < best_dist or (curr_dist == best_dist and curr_score > best_score)) {
                            best_dir = curr;
                            best_dist = curr_dist;
                            best_score = curr_score;
                        }
                    }
                    else {
                        if (curr_dist < best_dist or (curr_dist == best_dist and curr_score > best_score)) {
                            best_dir = curr;
                            best_dist = curr_dist;
                            best_score = curr_score;
                        }
                    }
                }
            }
        }
        return best_dir;
    }

    Dir adjacent_enemy(const Pos& start) {
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p = start+d;
            if (pos_ok(p) and cell_unit(cell(p)) == 3) return d;
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
    
    Dir find_nearest_fuel(const Pos& start) {
        int start_dist = distance_from_fuel[start.i][start.j];
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p2 = start+d;
            if (is_safe(p2, true) and distance_from_fuel[p2.i][p2.j] < start_dist) return d;
        }        
        return None;
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
    
    void bfs_fuel(const Pos& start) {
        VVI used(60, VI(60, false));
        queue<pair<int, Pos>> Q;
        Q.push(make_pair(0, start));
        used[start.i][start.j] = true;
        while (not Q.empty()) {
            Pos p = Q.front().second;
            int dist = Q.front().first;
            Q.pop();
            distance_from_fuel[p.i][p.j] = min(distance_from_fuel[p.i][p.j], dist);
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
        distance_from_water = distance_from_fuel = distance_from_city = VVI(60, VI(60, INF));
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                Cell c = cell(i, j);
                if (c.type == Water) {
                    bfs_water(Pos(i, j));
                }
                else if (c.type == Station) {
                    bfs_fuel(Pos(i, j));
                }
                else if (c.type == City) {
                    bfs_city(Pos(i, j));
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

    void initialize_map() {
        warrior_map = car_map = VVI(60, VI(60, 0));
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                Pos here = Pos(i, j);
                Cell p = cell(here);
                int& w = warrior_map[i][j];
                int& c = car_map[i][j];
                
                int dist_city = distance_from_city[i][j];
                int dist_fuel = distance_from_fuel[i][j];
                int dist_water = distance_from_water[i][j];
                
                if (not warrior_can_go(here)) w = -1;
                else {
                    if (p.type == City) w += 18;
                    if (dist_water < 5) w += 5-dist_water;
                    if (dist_city < 8) w += 2*(8-dist_city);
                    if (dist_water + dist_city < 12) w += 15;
                }
                
                if (not car_can_go(here)) c = -1;
                else {
                    if (p.type == Road) c += 30;
                    if (dist_water + dist_city < 16) c += 15;
                    if (dist_city < 15) c += 15-dist_city;
                    if (dist_city < 3) c -= 2*(4-dist_city);
                    c += (30-distance(here, Pos(30, 30)))/3;
                }
            }
        }
    }

    void move_warriors() {
        if (round()%4 != me()) return; //no es el meu torn
        
        //Per cada warrior que tinc
        for (int w = 0; w < (int)my_warriors.size(); w++) {
            Unit curr = unit(my_warriors[w]);
            bool moved = false;
            if (status(me()) < MAX_STATUS) {
                //TO DO: if in a city
                //escape from a car
                Pos nearest_car = find_nearest_enemy_car(curr.pos);
                int dist_car = distance(curr.pos, nearest_car);
                if (cell(curr.pos).type != City and (dist_car < ENEMY_CAR_RANGE or curr.food < MIN_FOOD)) {
                    Dir d = find_nearest_city(curr.pos);
                    if (d != None) {
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
                
                //TO DO: Millorar fight AI
                // if an enemy soldier is next to you
                Dir adj_enemy = adjacent_enemy(curr.pos);
                if (not moved and adj_enemy != None) {
                    Unit enemy = unit(cell(curr.pos+adj_enemy).id);
                    if (random(0, 1) or min(curr.food, curr.water) > min(enemy.food, enemy.water)) {
                        move(my_warriors[w], adj_enemy);
                        moved = true;
                    }
                }
                //if you need water
                else if (not moved and curr.water < MIN_WATER) {
                    Dir d = find_nearest_water(curr.pos);
                    if (d != None) {
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
            }
            if (not moved) {
                Dir d_rand = most_improved_direction(curr.pos, false);
                move(my_warriors[w], d_rand);
                
            }
        }
    }
    
    void move_cars() {
        //Per cada cotxe que tinc
        for (int c = 0; c < (int)my_cars.size(); c++) {
            Unit curr = unit(my_cars[c]);
            bool moved = false;
            //si es pot moure aquest torn
            if (can_move(my_cars[c])) {
                if (status(me()) < MAX_STATUS) {
                    //Si no tenim fuel anem a buscar-ne
                    if (curr.food < MIN_FUEL) {
                        Dir d = find_nearest_fuel(curr.pos);
                        if (d != None) {
                            move(my_cars[c], d);
                            moved = true;
                        }
                    }
                    if (not moved) {
                        Pos near_enemy = find_nearest_enemy_warrior(curr.pos);
                        if (distance(curr.pos, near_enemy) < CAR_ATTACK_RANGE) {
                            Dir d = find_direction_bfs(curr.pos, near_enemy, true);
                            if (d != None) {
                                move(my_cars[c], d);
                                moved = true;
                            }
                        }
                    }
                }
                if (not moved) {
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
        already_moved = SP();
        
        //Principal
        move_warriors();
        move_cars();
        
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
                    cerr << distance_from_fuel[i][j] << " ";
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

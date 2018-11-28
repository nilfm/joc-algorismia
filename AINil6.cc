#include "Player.hh"
#include <queue>
#include <set>

/* Per executar:
./Game Demo Demo Demo Demo -s 30 -i default.cnf -o default.res
*/

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Nil6


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
    // Maxima fraccio de CPU que es fa servir abans de triar moviment random
    const double MAX_STATUS = 0.99;
    // Maxima distancia a la qual el cotxe mirara per trobar soldats enemics
    const int CAR_DETECT_RANGE = 15;
    // Maxima distancia a la qual el cotxe mirara per atacar soldats enemics
    const int CAR_ATTACK_RANGE = 5;
    //Maxima distancia a la qual un soldat mirara per trobar cotxes enemics
    const int ENEMY_CAR_RANGE = 3;
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

    // Variables globals
    VI my_warriors;
    VI my_cars;
    VI enemy_warriors;
    VI enemy_cars;
    SP already_moved;
    SP water;
    SP fuel;
    VVP all_cities;
    VVI car_map;
    VVI warrior_map;

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
    
    int amount_enemy_cars_around(const Pos& p) {
        int count = 0;
        for (int i = 0; i < (int)enemy_cars.size(); i++) {
            Pos enemy = unit(enemy_cars[i]).pos;
            if (distance(p, enemy) <= ENEMY_CAR_RANGE) count++;
        }
        return count;
    }
    
    //returns weighted sum of warriors around pos p by distance to p
    //warriors in a city count significantly less
    int amount_enemy_warriors_around(const Pos& p) {
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
        if (not car) return warrior_map[p.i][p.j] - amount_enemy_cars_around(p);
        else return car_map[p.i][p.j] + amount_enemy_warriors_around(p);
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
    
    //if other, it will try to find only cities that are not controlled by me
    //if all cities are controlled by me, it will return (-1, -1)
    Pos find_nearest_city(const Pos& start, bool other) {
        int min_distance = 1e9;
        Pos best_pos(-1, -1);
        for (VP& v : all_cities) {
            for (Pos p : v) {
                if (not other or cell(p).owner != me()) {
                    int d = distance(start, p);
                    if (d < min_distance) {
                        min_distance = d;
                        best_pos = p;
                    }
                }
            }
        }
        return best_pos;
    }
    
    Pos find_nearest_water(const Pos& start) {
        int min_distance = 1e9;
        Pos best_pos(-1, -1);
        for (SP::iterator it = water.begin(); it != water.end(); it++) {
            Pos p = *it;
            int d = distance(start, p);
            if (d < min_distance) {
                min_distance = d;
                best_pos = p;
            }
        }
        return best_pos;
    }
    
    Pos find_nearest_fuel(const Pos& start) {
        int min_distance = 1e9;
        Pos best_pos(-1, -1);
        for (SP::iterator it = fuel.begin(); it != fuel.end(); it++) {
            Pos p = *it;
            int d = distance(start, p);
            if (d < min_distance) {
                min_distance = d;
                best_pos = p;
            }
        }
        return best_pos;
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
    
    void initialize_water_fuel() {
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                Cell c = cell(i, j);
                if (c.type == Water) {
                    water.insert(Pos(i, j));
                }
                else if (c.type == Station) {
                    fuel.insert(Pos(i, j));
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

    void initialize_map() {
        warrior_map = car_map = VVI(60, VI(60, 0));
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                Pos here = Pos(i, j);
                Cell p = cell(here);
                int& w = warrior_map[i][j];
                int& c = car_map[i][j];
                
                Pos nearest_water = find_nearest_water(here);
                int dist_water = distance(here, nearest_water);
                Pos nearest_city = find_nearest_city(here, false);
                int dist_city = distance(here, nearest_city);
                Pos nearest_fuel = find_nearest_fuel(here);
                int dist_fuel = distance(here, nearest_fuel);
                
                if (not warrior_can_go(here)) w = -1;
                else {
                    if (p.type == City) w += 18;
                    if (dist_water < 5) w += 5-dist_water;
                    if (dist_city < 8) w += 2*(8-dist_city);
                    if (dist_water + dist_city < 12) w += 15;
                }
                
                if (not car_can_go(here)) c = -1;
                else {
                    if (p.type == Road) c += 25;
                    if (dist_water < 10) c += 5-dist_water/2;
                    if (dist_city < 15) c += 15-dist_city;
                    if (dist_city < 3) c -= 2*(4-dist_city);
                    if (dist_fuel < 10) c += 5;
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

                //if in a city - TO DO
                
                //escape from a car
                Pos nearest_car = find_nearest_enemy_car(curr.pos);
                int dist_car = distance(curr.pos, nearest_car);
                if (dist_car < ENEMY_CAR_RANGE) {
                    Pos city = find_nearest_city(curr.pos, false);
                    Dir d = find_direction(curr.pos, city, false);
                    if (d != None) {
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
                
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
                    Pos near_water = find_nearest_water(curr.pos);
                    if (near_water.i != -1 or near_water.j != -1) {
                        Dir d = find_direction(curr.pos, near_water, false);
                        if (d != None and is_safe(curr.pos+d, false)) {
                            move(my_warriors[w], d);
                            moved = true;
                        }
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
                        Pos near_station = find_nearest_fuel(curr.pos);
                        Dir d = find_direction(curr.pos, near_station, true);
                        if (d != None) {
                            move(my_cars[c], d);
                            moved = true;
                        }
                    }
                    if (not moved) {
                        Pos near_enemy = find_nearest_enemy_warrior(curr.pos);
                        if (distance(curr.pos, near_enemy) < CAR_ATTACK_RANGE) {
                            Dir d = find_direction(curr.pos, near_enemy, true);
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
            all_cities = cities();
            initialize_water_fuel();
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
        
    }
};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

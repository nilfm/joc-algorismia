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
#define PLAYER_NAME Nil5


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
    const int CAR_RANGE = 12;
    //Maxima distancia a la qual un soldat mirara per trobar cotxes enemics
    const int ENEMY_CAR_RANGE = 2;
    //Minima aigua a la qual intentara anar a buscar aigua
    const int MIN_WATER = 10;
    //Minim menjar al qual intentara anar a buscar menjar
    const int MIN_FOOD = 12;
    //Minim fuel al qual intentara anar a buscar fuel
    const int MIN_FUEL = 20;
    
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
    VVI weighted_map;

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
    
    bool car_can_go(const Pos& p) {
        if (not pos_ok(p)) return false;
        Cell c = cell(p);
        return (c.type == Road or c.type == Desert or c.type == Station);
    }
    
    bool warrior_can_go(const Pos& p) {
        if (not pos_ok(p)) return false;
        Cell c = cell(p);
        return (c.type == Road or c.type == Station or c.type == Desert or c.type == City);
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
    
    bool is_forbidden(const Pos& p, bool car) {
        //Cannot move there
        if (not pos_ok(p)) return true;
        if (car and not car_can_go(p)) return true;
        if (not car and not warrior_can_go(p)) return true;
        
        //No friendly unit there
        if (is_friendly(cell(p))) return true;
        if (already_moved.count(p)) return true;
        
        //No car in the immediate surroundings
        if (is_car(cell(p))) return true;
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(i);
            Pos p2 = p + d;
            if (pos_ok(p2) and cell_unit(cell(p2)) == 4) return true;
        }
        
        return false;
    }
    
    Dir random_safe_direction(const Pos& start, bool car) {
        VI dirs = random_permutation(8);
        for (int i = 0; i < 8; i++) {
            Dir d = Dir(dirs[i]);
            Pos p = start + d;
            if (not is_forbidden(p, car)) {
                return d;
            }
        }
        return None;
    }
    
    //Aquesta funcio sera un Dijkstra
    //De moment es una merda
    Dir find_direction(const Pos& start, const Pos& end, bool car) {
        int initial_dist = distance(start, end);
        int best_dist = initial_dist;
        Dir best_dir = None;
        bool road = cell(start).type == Road;
        for (int i = 0; i < 8; i++) {
            Dir curr = Dir(i);
            int curr_dist = distance(start+curr, end);
            if (curr_dist == 0 and not is_forbidden(start+curr, car)) return curr;
            if (not is_forbidden(start+curr, car)) {
                if (car) {
                    bool cond1 = not road and cell(start+curr).type == Road and curr_dist <= best_dist;
                    bool cond2 = road and cell(start+curr).type == Road and curr_dist < best_dist;
                    bool cond3 = not road and curr_dist < best_dist;
                    bool cond4 = curr_dist <= 2;
                    if (cond1 or cond2 or cond3 or cond4) {
                        best_dir = curr;
                        best_dist = curr_dist;
                        road = cell(start+curr).type == Road;
                    }
                }
                else {
                    if (curr_dist < best_dist) {
                        best_dir = curr;
                        best_dist = curr_dist;
                    }
                }
            }
        }
        return best_dir;
    }
    
    Dir find_escape(const Pos& start, const Pos& danger) {
        int best_dist = distance(start, danger);
        Dir best_dir = None;
        for (int i = 0; i < 8; i++) {
            Dir curr = Dir(i);
            int curr_dist = distance(start+curr, danger);
            if (curr_dist > best_dist and not is_forbidden(start+curr, false)) {
                best_dist = curr_dist;
                best_dir = curr; 
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
    
    //Find functions

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
            if (d < min_distance) {
                min_distance = d;
                best_pos = p;
            }
        }
        return best_pos;
    }
    
    //Initialize functions
    
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
    
    void initialize_weighted_map() {
        for (int i = 0; i < 60; i++) {
            for (int j = 0; j < 60; j++) {
                //TODO
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
                
                Pos nearest_car = find_nearest_enemy_car(curr.pos);
                int dist_car = distance(curr.pos, nearest_car);
                if (dist_car < ENEMY_CAR_RANGE) {
                    Dir d = find_escape(curr.pos, nearest_car);
                    if (d != None) {
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
                
                Dir adj_enemy = adjacent_enemy(curr.pos); //calculem per despres
                // if an enemy soldier is next to you
                if (not moved and adj_enemy != None) {
                    Unit enemy = unit(cell(curr.pos+adj_enemy).id);
                    if (min(curr.food, curr.water) > min(enemy.food, enemy.water)) {
                        move(my_warriors[w], adj_enemy);
                        moved = true;
                    }
                }
                
                //if you need water
                else if (not moved and curr.water < MIN_WATER) {
                    Pos near_water = find_nearest_water(curr.pos);
                    if (near_water.i != -1 or near_water.j != -1) {
                        Dir d = find_direction(curr.pos, near_water, false);
                        if (d != None and not is_forbidden(curr.pos+d, false)) {
                            move(my_warriors[w], d);
                            moved = true;
                        }
                    }
                }
                //go to nearby city
                else if (not moved) {
                    Pos p;
                    //si ens falta menjar o estem a una ciutat ja controlada per mi
                    p = find_nearest_city(curr.pos, false);
                    Dir d = find_direction(curr.pos, p, false);
                    if (d != None) {
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
            }
            //Per si de cas no hem pogut moure en tot el torn
            if (not moved) {
                Dir d_rand = random_safe_direction(curr.pos, false);
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
                    else {
                        Pos near_enemy = find_nearest_enemy_warrior(curr.pos);
                        if (distance(curr.pos, near_enemy) < CAR_RANGE) {
                            Dir d = find_direction(curr.pos, near_enemy, true);
                            if (not is_forbidden(curr.pos+d, true)) {
                                move(my_cars[c], d);
                                moved = true;
                            }
                        }
                    }
                }
                if (not moved) {
                    Dir d_rand = random_safe_direction(curr.pos, true);
                    move(my_cars[c], d_rand);
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
            initialize_weighted_map();
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
    }

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

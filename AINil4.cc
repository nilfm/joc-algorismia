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
#define PLAYER_NAME Nil4


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
    //Maxima distance a la qual el cotxe mirara per trobar fuel station
    const int FUEL_RANGE = 15;
    //Maxima distancia a la qual el soldat mirara per anar a buscar aigua
    const int WATER_RANGE = 10;
    //Minima aigua a la qual intentara anar a buscar aigua
    const int MIN_WATER = 12;
    //Minim menjar al qual intentara anar a buscar menjar
    const int MIN_FOOD = 8;
    
    // Typedefs
    typedef vector<int> VI;
    typedef vector<VI> VVI;
    typedef set<Pos> SP;
    typedef vector<Pos> VP;
    typedef vector<VP> VVP;

    // Variables globals
    VI my_warriors;
    VI my_cars;
    SP already_moved;
    SP water;
    VVP all_cities;

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
    
    //target 0: enemy warrior
    //target 1: fuel
    //target 2: water
    Pos bfs(const Pos& start, int target) {
        queue<pair<Pos, int>> Q;
        SP seen;
        Q.push(make_pair(start, 0));
        bool cont = true;
        while (not Q.empty() and cont) {
            int dist = Q.front().second;
            int RANGE = 0;
            if (target == 0) RANGE = CAR_RANGE;
            else if (target == 1) RANGE = FUEL_RANGE;
            else if (target == 2) RANGE = WATER_RANGE;
            if (dist >= RANGE) cont = false;
            Pos p = Q.front().first;
            Q.pop();
            if (pos_ok(p)) {
                if (target == 0 and cell_unit(cell(p)) == 3) return p;
                else if (target == 1 and cell(p).type == Station) return p;
                else if (target == 2 and cell(p).type == Water) return p;
            }
            if (pos_ok(p) and car_can_go(p) and not seen.count(p)) {
                seen.insert(p);
                for (int i = 0; i < 8; i++) {
                    Dir d = Dir(i);
                    Pos p2 = p + d;
                    Q.push(make_pair(p2, dist+1));
                }
            }
            
        }
        return Pos(-1, -1);
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
    Dir find_direction(const Pos& start, const Pos& end) {
        if (start.i == end.i and start.j == end.j) return None;
        if (start.i < end.i) {
            if (start.j < end.j) return BR;
            else if (start.j > end.j) return LB;
            else return Bottom;
        }
        else if (start.i > end.i) {
            if (start.j < end.j) return RT;
            else if (start.j > end.j) return TL;
            else return Top;
        }
        else {
            if (start.j < end.j) return Right;
            return Left;
        }
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

    void move_warriors() {
        if (round()%4 != me()) return; //no es el meu torn
        for (int w = 0; w < (int)my_warriors.size(); w++) {
            Unit curr = unit(my_warriors[w]);
            bool moved = false;
            if (status(me()) < MAX_STATUS) {
                // if an enemy soldier is next to you
                Dir adj_enemy = adjacent_enemy(curr.pos);
                if (adj_enemy != None) {
                    move(my_warriors[w], adj_enemy);
                    moved = true;
                }
                //if you need water
                else if (curr.water < MIN_WATER) {
                    Pos near_water = bfs(curr.pos, 2);
                    if (near_water.i != -1 or near_water.j != -1) {
                        Dir d = find_direction(curr.pos, near_water);
                        if (d != None and not is_forbidden(curr.pos+d, false)) {
                            move(my_warriors[w], d);
                            moved = true;
                        }
                    }
                }
                //go to nearby city
                else {
                    Pos p;
                    //si ens falta menjar o estem a una ciutat ja controlada per mi
                    if (curr.food < MIN_FOOD or cell(curr.pos).owner == me()) p = find_nearest_city(curr.pos, false);
                    else p = find_nearest_city(curr.pos, true);
                    Dir d = find_direction(curr.pos, p);
                    if (not is_forbidden(p+d, false)) {
                        move(my_warriors[w], d);
                        moved = true;
                    }
                }
            }
            if (not moved) {
                Dir d_rand = random_safe_direction(curr.pos, false);
                move(my_warriors[w], d_rand);
                
            }
        }
    }
    
    void move_cars() {
        for (int c = 0; c < (int)my_cars.size(); c++) {
            Unit curr = unit(my_cars[c]);
            bool moved = false;
            if (can_move(my_cars[c])) {
                if (status(me()) < MAX_STATUS) {
                    //Si no tenim fuel anem a buscar-ne
                    if (curr.food < 15) {
                        Pos near_station = bfs(curr.pos, 1);
                        if (near_station.i != -1 or near_station.j != -1) {
                            Dir d = find_direction(curr.pos, near_station);
                            if (d != None and not is_forbidden(curr.pos+d, true)) {
                                move(my_cars[c], d);
                                moved = true;
                            }
                        }
                    }
                    else {
                        Pos near_enemy = bfs(curr.pos, 0);
                        if (near_enemy.i != -1 or near_enemy.j != -1) {
                            Dir d = find_direction(curr.pos, near_enemy);
                            if (d != None and not is_forbidden(curr.pos+d, true)) {
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
        //Preliminars
        if (round() == 0) all_cities = cities();
        my_warriors = warriors(me());
        my_cars = cars(me());
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

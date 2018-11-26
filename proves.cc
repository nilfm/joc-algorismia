#include "Player.hh"
#include <queue>
#include <set>

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME prova


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
    template<typename T> class Comp {
        bool operator()(T a, T b){
            if (a.i < b.i) return true;
            if (a.i > b.i) return false;
            return a.j < b.j; 
        }
    };
    
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
    
    //TODO: Dijkstra

    void move_warriors() {
        if (round()%4 != me()) return; //no es el meu torn
        for (int w = 0; w < (int)my_warriors.size(); w++) {
            Unit curr = unit(my_warriors[w]);
            bool cont = true;
            for (int i = 0; i < 8 and cont; i++) {
                Dir d = Dir(i);
                Pos new_pos = curr.pos+d;
                if (pos_ok(new_pos) and cell(new_pos).type == City) {
                    move(my_warriors[w], d);
                    cont = false;
                }
                else if (pos_ok(new_pos) and cell_unit(cell(new_pos)) == 3) {
                    move(my_warriors[w], d);
                    cont = false;
                }
            }
            for (int i = 0; i < 8 and cont; i++) {
                Dir d = Dir(i);
                Pos new_pos = curr.pos+d;
                if (pos_ok(new_pos) and cell_unit(cell(new_pos)) == 0) {
                    move(my_warriors[w], d);
                    cont = false;
                }
            }
        }
    }
    
    void move_cars() {
        for (int c = 0; c < (int)my_cars.size(); c++) {
            Unit curr = unit(my_cars[c]);
            bool cont = true;
            for (int i = 0; i < 8 and cont; i++) {
                Dir d = Dir(i);
                Pos new_pos = curr.pos+d;
                if (pos_ok(new_pos) and cell_unit(cell(new_pos)) == 3) {
                    move(my_cars[c], d);
                    cont = false;
                }
            }
            for (int i = 0; i < 8 and cont; i++) {
                Dir d = Dir(i);
                Pos new_pos = curr.pos+d;
                if ((pos_ok(new_pos) and cell_unit(cell(new_pos)) != 4 and (random(1, 10) < 8 and cell(new_pos).type == Road) or (random(1, 10) < 6 and cell(new_pos).type == Station)) or (random(1,10) < 2)) {
                    move(my_cars[c], d);
                    cont = false;
                }
            }
            for (int i = 0; i < 8 and cont; i++) {
                Dir d = Dir(i);
                Pos new_pos = curr.pos+d;
                if (pos_ok(new_pos) and cell_unit(cell(new_pos)) != 4) {
                    move(my_cars[c], d);
                    cont = false;
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

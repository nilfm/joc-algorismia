#include "Player.hh"
#include <queue>

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Nil1


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
    typedef vector<int> VI;
    typedef vector<VI> VVI;

    //TODO: Dijkstra

    void move_warriors() {
        if (round()%4 != me()) return; //no es el meu torn
        
    }
    
    void move_cars() {
        
    } 

    /**
    * Play method, invoked once per each round.
    */
    virtual void play () {
        move_warriors();
        move_cars();
    }

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

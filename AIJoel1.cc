#include "Player.hh"


/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Joel1

// DISCLAIMER: The following Demo player is *not* meant to do anything
// sensible. It is provided just to illustrate how to use the API.
// Please use AINull.cc as a template for your player.

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

  typedef vector<int> VE;


  void move_warriors() {
    if (round()% 4 != me()) return;

    VE W = warriors(me());
    int n = W.size();
    for (int i = 0; i < n; ++i) {
      int id = W[i];
      Dir d = Dir(random(0, 7));
      command(id, d);
    }
  }

  void move_cars() {
    vector<int> C = cars(me());
    for (int id : C) {
      if (can_move(id)) { // This also makes sense.
        command(id, Dir(random(0, 7)));
      }
    }
  }


  /**
   * Play method, invoked once per each round.
   */
  void play () {
    move_warriors();
    move_cars();
  }
};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

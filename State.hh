#ifndef State_hh
#define State_hh


#include "Structs.hh"


/*! \file
 * Contains a class to store the current state of a game.
 */


/**
 * Stores the game state.
 */
class State {

  friend class Info;
  friend class Board;
  friend class Game;
  friend class SecGame;
  friend class Player;

  vector< vector<Cell> > grid_;
  int round_;
  vector<Unit> unit_;
  vector<int> num_cities_;
  vector<int> total_score_;
  vector<double> cpu_status_; // -1 -> dead, 0..1 -> % of cpu time limit
  vector< vector<int> > warriors_;
  vector< vector<int> > cars_;

  /**
   * Returns whether id is a valid unit identifier.
   */
  inline bool unit_ok (int id) const {
    return id >= 0 and id < nb_units();
  }

public:

  /**
   * Returns the current round.
   */
  inline int round () const {
    return round_;
  }

  /**
   * Returns a copy of the cell at p.
   */
  inline Cell cell (Pos p) const {
    if (p.i < 0 or p.i >= (int)grid_.size()
        or p.j < 0 or p.j >= (int)grid_[p.i].size()) {
      cerr << "warning: cell requested for position " << p << endl;
      return Cell();
    }
    return grid_[p.i][p.j];
  }

  /**
   * Returns a copy of the cell at (i, j).
   */
  inline Cell cell (int i, int j) const {
    return cell(Pos(i, j));
  }

  /**
   * Returns the total number of units in the game.
   */
  inline int nb_units () const {
    return unit_.size();
  }

  /**
   * Returns the information of the unit with identifier id.
   */
  inline Unit unit (int id) const {
    if (not unit_ok(id)) {
      cerr << "warning: unit requested for identifier " << id << endl;
      return Unit();
    }
    return unit_[id];
  }

  /**
   * Returns the current number of cities owned by a player.
   */
  inline int num_cities (int pl) const {
    if (pl < 0 or pl >= (int)num_cities_.size()) {
      cerr << "warning: score requested for player " << pl << endl;
      return -1;
    }
    return num_cities_[pl];
  }

  /**
   * Returns the total score of a player.
   */
  inline int total_score (int pl) const {
    if (pl < 0 or pl >= (int)total_score_.size()) {
      cerr << "warning: total score requested for player " << pl << endl;
      return -1;
    }
    return total_score_[pl];
  }

  /**
   * Returns the percentage of cpu time used in the last round, in the
   * range [0.0 - 1.0] or a value lesser than 0 if this player is dead.
   * Note that this is only accessible if secgame() is true.
   */
  inline double status (int pl) const {
    if (pl < 0 or pl >= (int)cpu_status_.size()) {
      cerr << "warning: status requested for player " << pl << endl;
      return -2;
    }
    return cpu_status_[pl];
  }

  /**
   * Returns the ids of all the warriors of a player.
   */
  inline vector<int> warriors (int pl) const {
    if (pl < 0 or pl >= (int)num_cities_.size()) {
      cerr << "warning: warriors requested for player " << pl << endl;
      return vector<int>();
    }
    return warriors_[pl];
  }

  /**
   * Returns the ids of all the cars of a player.
   */
  inline vector<int> cars (int pl) const {
    if (pl < 0 or pl >= (int)num_cities_.size()) {
      cerr << "warning: cars requested for player " << pl << endl;
      return vector<int>();
    }
    return cars_[pl];
  }

  /**
   * Tells if a unit can move at this round.
   */
  inline bool can_move (int id) const {
    if (not unit_ok(id)) return false;
    const Unit& u = unit_[id];
    if (u.player == round()%4) return true;
    if (u.type == Warrior) return false;
    return u.food > 0 and cell(u.pos).type == Road;
  }

};


#endif

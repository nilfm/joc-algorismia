#ifndef Settings_hh
#define Settings_hh


#include "Structs.hh"


/** \file
 * Contains a class to store all the game settings that do not change
 * during a game, except the names of the players.
 */


/**
 * Stores most of the game settings.
 */
class Settings {

  friend class Info;
  friend class Board;
  friend class Game;
  friend class SecGame;
  friend class Player;

  int nb_players_;
  int nb_rounds_;
  int nb_cities_;
  int nb_warriors_;
  int nb_cars_;
  int warriors_health_;
  int cars_fuel_;
  int damage_;
  int rows_;
  int cols_;

  /**
   * Reads the settings from a stream.
   */
  static Settings read_settings (istream& is);

public:

  /**
   * Returns a string with the game name and version.
   */
  inline static string version () {
    return "Mad_Max 1.6";
  }

  /**
   * Returns the number of players in the game.
   */
  inline int nb_players () const {
    return nb_players_;
  }

  /**
   * Returns the number of rounds for the game.
   */
  inline int nb_rounds () const {
    return nb_rounds_;
  }

  /**
   * Returns the number of cities in the grid.
   */
  inline int nb_cities () const {
    return nb_cities_;
  }

  /**
   * Returns the initial number of warriors for every player.
   */
  inline int nb_warriors () const {
    return nb_warriors_;
  }

  /**
   * Returns the initial number of cars for every player.
   */
  inline int nb_cars () const {
    return nb_cars_;
  }

  /**
   * Returns the initial (and maximum) health of the warriors (both in food and water).
   */
  inline int warriors_health () const {
    return warriors_health_;
  }

  /**
   * Returns the initial (and maximum) fuel of the cars.
   */
  inline int cars_fuel () const {
    return cars_fuel_;
  }

  /**
   * Returns the damage inflicted by a warrior attack.
   */
  inline int damage () const {
    return damage_;
  }

  /**
   * Returns the number of rows of the maze of the game.
   */
  inline int rows () const {
    return rows_;
  }

  /**
   * Returns the number of columns of the maze of the game.
   */
  inline int cols () const {
    return cols_;
  }

  /**
   * Returns whether pl is a valid player identifier.
   */
  inline bool player_ok (int pl) const {
    return pl >= 0 and pl < nb_players();
  }

  /**
   * Returns whether (i, j) is a position inside the board.
   */
  inline bool pos_ok (int i, int j) const {
    return i >= 0 and i < rows() and j >= 0 and j < cols();
  }

  /**
   * Returns whether p is a position inside the board.
   */
  inline bool pos_ok (Pos p) const {
    return pos_ok(p.i, p.j);
  }

};


#endif

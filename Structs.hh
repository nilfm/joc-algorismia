#ifndef Structs_hh
#define Structs_hh


#include "Utils.hh"


/** \file
 * Contains the Dir enumeration, the Pos struct,
 * the CellType enumeration, the Cell struct,
 * the UnitType enumeration, the Unit struct,
 * and some useful little functions.
 */


/**
 * Enum to encode directions.
 */
enum Dir {
  Bottom, BR, Right, RT, Top, TL, Left, LB,
  None,
  DirSize
};


/**
 * Returns whether dir is a valid direction.
 */
inline bool dir_ok (Dir dir) {
  return dir >= Bottom and dir <= None;
}


/**
 * Simple struct to handle positions.
 */
struct Pos {

  int i, j;

  /**
   * Default constructor (0, 0).
   */
  inline Pos () : i(0), j(0) { }

  /**
   * Given constructor.
   */
  inline Pos (int i, int j) : i(i), j(j) { }

  /**
   * Print operator.
   */
  inline friend ostream& operator<< (ostream& os, const Pos& p) {
    return os << "(" << p.i << ", " << p.j << ")";
  }

  /**
   * Comparison operator.
   */
  inline friend bool operator== (const Pos& a, const Pos& b) {
    return a.i == b.i and a.j == b.j;
  }

  /**
   * Comparison operator.
   */
  inline friend bool operator!= (const Pos& a, const Pos& b) {
    return not (a == b);
  }

  /**
   * Comparison operator, mostly needed for sorting.
   */
  inline friend bool operator< (const Pos& a, const Pos& b) {
    if (a.i != b.i) return a.i < b.i;
    return a.j < b.j;
  }

  /**
   * Increment operator: moves a position according to a direction.
   */
  inline Pos& operator+= (Dir d) {
    switch (d) {
      case Bottom: ++i;  break;
      case BR: ++i; ++j; break;
      case Right:  ++j;  break;
      case RT: --i; ++j; break;
      case Top:    --i;  break;
      case TL: --i; --j; break;
      case Left:   --j;  break;
      case LB: ++i; --j; break;
      case None:         break;
      default: ; // do nothing
    }
    return *this;
  }

  /**
   * Addition operator: Returns a position by adding a direction.
   */
  inline Pos operator+ (Dir d) const {
    Pos p = *this;
    p += d;
    return p;
  }

  /**
   * Increment operator: moves a position according to another position.
   */
  inline Pos& operator+= (Pos p) {
    this->i += p.i;
    this->j += p.j;
    return *this;
  }

  /**
   * Addition operator: Returns a position by adding another position.
   */
  inline Pos operator+ (Pos p) const {
    Pos p2 = *this;
    p2 += p;
    return p2;
  }

};


/**
 * Defines if a cell is empty or it has any special feature on it.
 */
enum CellType {
  Desert, Road, City, Water, Station, Wall,
  CellTypeSize
};


/**
 * Describes a cell in the board, and its contents.
 */
struct Cell {

  CellType type; // The kind of cell.
  int owner;     // If a city cell, the player that owns it, otherwise -1.
  int id;        // The id of a unit if present, or -1 otherwise.

  /**
   * Default constructor (Desert, -1, -1).
   */
  inline Cell () : type(Desert), owner(-1), id(-1) { }

  /**
   * Given constructor.
   */
  inline Cell (CellType t, int ow, int id)
               : type(t), owner(ow), id(id) { }

};


/**
 * Defines the type of the unit.
 */
enum UnitType {
  Warrior, Car,
  UnitTypeSize
};


/**
 * Returns whether u is a valid UnitType.
 */
inline bool ut_ok (UnitType u) {
  return u == Warrior or u == Car;
}


/**
 * Returns the char corresponding to a unit type.
 */
inline char ut2char (UnitType u) {
  if (u == Warrior) return 'w';
  if (u == Car) return 'c';
  _unreachable();
}


/**
 * Returns the unit type corresponding to a char.
 */
inline UnitType char2ut (char c) {
  if (c == 'w') return Warrior;
  if (c == 'c') return Car;
  _unreachable();
}


/**
 * Describes a unit on the board and its properties.
 */
struct Unit {

  UnitType type; // The kind of unit.
  int id;        // The unique id for this unit during the game.
  int player;    // The player that owns this unit.
  int food;      // For warriors, the current food. For cars, the current fuel.
  int water;     // For warriors, the current water. For cars, nothing.
  Pos pos;       // The position inside the board.

  /**
   * Default constructor (Warrior, -1, -1, 0, 0, (0, 0)).
   */
  inline Unit () : type(Warrior), id(-1), player(-1), food(0), water(0), pos(0, 0) { }

  /**
   * Given constructor.
   */
  inline Unit (UnitType t, int id, int pl, int f, int w, Pos p = Pos(0, 0))
               : type(t), id(id), player(pl), food(f), water(w), pos(p) { }

};


#endif

#ifndef Board_hh
#define Board_hh


#include "Info.hh"
#include "Action.hh"
#include "Random.hh"


/*! \file
 * Contains the Board class, with all the game information,
 * plus the names of the players and a random generator.
 */


/**
 * Manages a random generator and the information of the board.
 */
class Board : public Info, public Random_generator {

  friend class Game;
  friend class SecGame;

  vector<string> names_;
  string generator_;
  vector< vector<Pos> > cells_cities_;

  /**
   * Used by generate random maps.
   */
  map<Pos, Pos> parent_;
  map<Pos, int> area_;
  vector<vector<bool>> seen_;
  vector<vector<Pos>> zone_;
  vector<int> X_, Y_;

  void capture (int id, int pl, vector<bool>& killed);

  void step (int id, Pos p2);

  vector<int> two_different (int pl1, int pl2);

  /**
   * Tries to apply a move. Returns true if it could. Marks killed units.
   */
  bool move (int id, Dir dir, vector<bool>& killed);

  /**
   * Computes the current number of cities owned,
   * and updates the total scores of all players.
   */
  void compute_scores ();

  /**
   * To mark every city at the start of the game.
   */
  void dfs (int i, int j, int owner, vector<vector<bool>>& seen,
            vector<Pos>& cells) const;

  /**
   * Detects and stores all the cities of the board at the start of the game.
   */
  void detect_cities ();

  /**
   * Used by generate_units.
   */
  void new_unit (int& id, int pl, Pos pos, UnitType t);

  /**
   * Generate all the units of the board.
   */
  void generate_units ();

  /**
   * Reads the generator method, and generates or reads the grid.
   */
  void read_generator_and_grid (istream& is);

  /**
   * Generates a board at random.
   */
  void generator (vector<int> param);

  /**
   * Prints some information of the unit.
   */
  inline static void print_unit (Unit u, ostream& os) {
    os << ut2char(u.type) << ' '
       << u.player << ' '
       << u.pos.i << ' '
       << u.pos.j << ' '
       << u.food << ' '
       << u.water << endl;
  }

  /**
   * Used to spawn_units.
   */
  void place (int id, Pos p);

  /**
   * Used to recharge water and fuel.
   */
  bool adjacent (int id, CellType t) const;

  /**
   * Used to spawn units.
   */
  inline bool pos_safe (Pos p) const;

  /**
   * Used by generate random maps.
   */
  bool good_roads (const vector<int>& R) const;
  vector<int> choose_roads (int q);
  Pos repre (Pos p);
  int area (int i, int j);
  static bool before (const vector<Pos>& V1, const vector<Pos>& V2);
  void mark (int i, int j, vector<Pos>& Z);
  Pos choose_one (const set<Pos>& S);
  void make_city (int pl, vector<Pos>& Z);
  void make_water (vector<Pos>& Z);
  void make_wall (Pos ini, int d, set<Pos>& S);
  void make_walls (const vector<Pos>& Z);
  bool possible_station (int i, int j) const;
  int basic_distribution ();


public:

  /**
   * Returns the name of a player.
   */
  inline string name (int player) const {
    assert(player_ok(player));
    return names_[player];
  }

  /**
   * Construct a board by reading information from a stream.
   */
  Board (istream& is, int seed);

  /**
   * Prints the board preamble to a stream.
   */
  void print_preamble (ostream& os) const;

  /**
   * Prints the name players to a stream.
   */
  void print_names (ostream& os) const;

  /**
   * Prints the state of the board to a stream.
   */
  void print_state (ostream& os) const;

  /**
   * Prints the results and the names of the winning players.
   */
  void print_results () const;

  /**
   * Used by next() to spawn dead cars.
   */
  void spawn_cars (const vector<int>& dead_c);

  /**
   * Used by next() to spawn dead warriors.
   */
  void spawn_warriors (const vector<int>& dead_w);

  /**
   * Computes the next board aplying the given actions to the current board.
   * It also prints to os the actual actions performed.
   */
  void next (const vector<Action>& act, ostream& os);

};


#endif

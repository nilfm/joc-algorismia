#include "Player.hh"


void Player::reset (ifstream& is) {
  *(Action*)this = Action();

  read_grid(is);

  string s;
  is >> s >> round_;
  assert(s == "round");
  assert(round_ >= 0 and round_ < nb_rounds());

  is >> s;
  assert(s == "num_cities");
  num_cities_ = vector<int>(nb_players(), 0);
  for (auto& nc : num_cities_) {
    is >> nc;
    assert(nc >= 0);
  }

  is >> s;
  assert(s == "total_score");
  total_score_ = vector<int>(nb_players(), 0);
  for (auto& ts : total_score_) {
    is >> ts;
    assert(ts >= 0);
  }

  is >> s;
  assert(s == "status");
  cpu_status_ = vector<double>(nb_players(), 0);
  for (auto& st : cpu_status_) {
    is >> st;
    assert(st == -1 or (st >= 0 and st <= 1));
  }

  unit_ = vector<Unit>(nb_players()*(nb_warriors() + nb_cars()));

  for (int id = 0; id < nb_units(); ++id) {
    char type;
    int player, i, j, food, water;
    _my_assert(is >> type >> player >> i >> j >> food >> water,
               "Could not read info for unit " + int_to_string(id) + ".");

    assert(type == 'w' or type == 'c');
    assert(player >= 0 and player < nb_players());
    assert(i >= 0 and i < rows());
    assert(j >= 0 and j < cols());
    assert(grid_[i][j].id == -1);
    int t = grid_[i][j].type;
    assert(t == Desert or t == Road or t == City);
    if (type == 'w') {
      assert(food > 0 and food <= warriors_health()
             and water > 0 and water <= warriors_health());
    }
    else {
      assert(food >= 0 and food <= cars_fuel() and water == 0);
      assert(t != City);
    }

    grid_[i][j].id = id;
    unit_[id] = Unit(char2ut(type), id, player, food, water, Pos(i, j));
  }

  update_vectors_by_player();
}

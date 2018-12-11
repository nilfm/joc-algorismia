#include "Board.hh"
#include "Action.hh"


void Board::capture (int id, int pl, vector<bool>& killed) {
  Unit& u = unit_[id];
  assert(u.player != pl);

  u.player = pl;
  if (u.type == Warrior) u.food = u.water = warriors_health();
  else {
    u.food = cars_fuel();
    u.water = 0;
  }
  grid_[u.pos.i][u.pos.j].id = -1;
  killed[id] = true;
}


void Board::step (int id, Pos p2) {
  Unit& u = unit_[id];
  Pos p1 = u.pos;
  Cell& c1 = grid_[p1.i][p1.j];
  Cell& c2 = grid_[p2.i][p2.j];
  c1.id = -1;
  c2.id = id;
  u.pos = p2;
}


vector<int> Board::two_different (int pl1, int pl2) {
  vector<int> select;
  vector<int> perm = random_permutation(nb_players());
  for (int i = 0; (int)select.size() < 2; ++i) {
    int pl = perm[i];
    if (pl != pl1 and pl != pl2) select.push_back(pl);
  }
  return select;
}


// id is a valid unit id, moved by its player, and d is a valid dir != None.
bool Board::move (int id, Dir dir, vector<bool>& killed) {
  Unit& u = unit_[id];
  Pos p1 = u.pos;
  assert(pos_ok(p1));

  Cell& c1 = grid_[p1.i][p1.j];
  assert(c1.type == Desert or c1.type == Road
         or (c1.type == City and u.type == Warrior));

  Pos p2 = p1 + dir;
  if (not pos_ok(p2)) return false;

  Cell& c2 = grid_[p2.i][p2.j];
  if (c2.type != Desert and c2.type != Road
      and (c2.type != City or u.type != Warrior)) return false;

  int id2 = c2.id;
  if (id2 == -1) {
    step(id, p2);
    return true;
  }

  Unit& u2 = unit_[id2];
  vector<int> select = two_different(u.player, u2.player);

  if (u.type == Car) {
    if (u2.type == Car) { // two cars crash (of the same team or not)
      capture(id2, select[0], killed);
      capture(id, select[1], killed);
      return true;
    }

    if (u2.player == u.player) { // run over own warrior
      capture(id2, select[0], killed);
      step(id, p2);
      return true;
    }

    capture(id2, u.player, killed); // run over enemy warrior
    step(id, p2);
    return true;
  }

  if (u2.type == Car) { // suicidal run over
    if (u2.player == u.player) { // own car
      capture(id, select[0], killed);
      return true;
    }

    capture(id, u2.player, killed); // enemy car
    return true;
  }

  // warrior attacks warrior (of the same team or not)
  if (c1.type == City and c2.type == City) { // thunderdome
    if (random(0, u.water + u2.water - 1) < u.water) {
      if (u.player == u2.player) capture(id2, select[0], killed);
      else capture(id2, u.player, killed);
    }
    else {
      if (u.player == u2.player) capture(id, select[0], killed);
      else capture(id, u2.player, killed);
    }
    return true;
  }

  int f = min(u2.food, damage());
  int w = min(u2.water, damage());
  u2.food -= f;
  u2.water -= w;
  u.food += f/2;
  u.food = min(u.food, warriors_health());
  u.water += w/2;
  u.water = min(u.water, warriors_health());
  if (u2.food <= 0 or u2.water <= 0) {
    if (u2.player == u.player) capture(id2, select[0], killed);
    else capture(id2, u.player, killed);
  }
  return true;
}


void Board::compute_scores () {
  num_cities_ = vector<int>(nb_players(), 0);
  for (int i = 0; i < nb_cities(); ++i) {
    int owner = cell(cells_cities_[i][0]).owner;
    vector<int> counter(nb_players(), 0);
    for (int j = 0; j < (int)cells_cities_[i].size(); ++j) {
      Pos pos = cells_cities_[i][j];
      assert(grid_[pos.i][pos.j].owner == owner);
      int id = grid_[pos.i][pos.j].id;
      if (id != -1) {
        Unit u = unit(id);
        assert(u.type == Warrior);
        assert(player_ok(u.player));
        ++counter[u.player];
      }
    }

    int mx = 0;
    for (int c : counter) mx = max(mx, c);
    if (counter[owner] < mx) {
      int q = 0;
      for (int c : counter)
        if (c == mx) ++q;
      if (q == 1) {
        for (int pl = 0; pl < nb_players(); ++pl)
          if (counter[pl] == mx) owner = pl;
        for (int j = 0; j < (int)cells_cities_[i].size(); ++j) {
          Pos pos = cells_cities_[i][j];
          grid_[pos.i][pos.j].owner = owner;
        }
      }
    }
    ++num_cities_[owner];
  }

  for (int pl = 0; pl < nb_players(); ++pl) total_score_[pl] += num_cities_[pl];
}


// ***************************************************************************


void Board::dfs (int i, int j, int owner, vector<vector<bool>>& seen,
                 vector<Pos>& cells) const {
  assert(i >= 0 and i < rows() and j >= 0 and j < cols());
  if (seen[i][j]) return;
  seen[i][j] = true;
  if (grid_[i][j].type == Desert) return;
  assert(grid_[i][j].type == City and grid_[i][j].owner == owner);
  cells.push_back(Pos(i, j));
  dfs(i + 1, j, owner, seen, cells);
  dfs(i - 1, j, owner, seen, cells);
  dfs(i, j + 1, owner, seen, cells);
  dfs(i, j - 1, owner, seen, cells);
}


void Board::detect_cities () {
  cells_cities_ = vector<vector<Pos>>(0);
  vector<vector<bool>> seen(rows(), vector<bool>(cols(), false));
  for (int i = 0; i < rows(); ++i)
    for (int j = 0; j < cols(); ++j)
      if (grid_[i][j].type == City and not seen[i][j]) {
        int owner = grid_[i][j].owner;
        assert(player_ok(owner));
        vector<Pos> cells;
        dfs(i, j, owner, seen, cells);
        cells_cities_.push_back(cells);
      }
  assert((int)cells_cities_.size() == nb_cities());
}


void Board::new_unit (int& id, int pl, Pos pos, UnitType t) {
  unit_[id] =
    (t == Car ? Unit(Car, id, pl, cars_fuel(), 0, pos) :
     Unit(Warrior, id, pl, warriors_health(), warriors_health(), pos));
  grid_[pos.i][pos.j].id = id++;
}


void Board::generate_units () {
  int id = 0;
  vector<vector<Pos>> cells_per_player(nb_players());
  vector<int> segurs(nb_players(), 0);
  for (int i = 0; i < nb_cities(); ++i) {
    assert(not cells_cities_[i].empty());
    int pl = cell(cells_cities_[i][0]).owner;
    assert(player_ok(pl));
    ++segurs[pl];
    int ran = random(0, cells_cities_[i].size() - 1);
    for (int j = 0; j < (int)cells_cities_[i].size(); ++j) {
      Pos pos = cells_cities_[i][j];
      if (j == ran) new_unit(id, pl, pos, Warrior);
      else cells_per_player[pl].push_back(pos);
    }
  }

  for (int pl = 0; pl < nb_players(); ++pl) {
    assert(segurs[pl] == nb_cities()/nb_players());
    int falta = nb_warriors() - nb_cities()/nb_players();
    int num_cells = cells_per_player[pl].size();
    assert(num_cells >= falta);
    vector<int> perm = random_permutation(num_cells);
    for (int i = 0; i < falta; ++i)
      new_unit(id, pl, cells_per_player[pl][perm[i]], Warrior);
  }

  vector<Pos> pos;
  for (int i = 0; i < rows(); ++i) {
    if (grid_[i][0].type == Road) pos.push_back(Pos(i, 0));
    if (grid_[i][cols()-1].type == Road) pos.push_back(Pos(i, cols()-1));
  }
  for (int j = 0; j < cols(); ++j) {
    if (grid_[0][j].type == Road) pos.push_back(Pos(0, j));
    if (grid_[rows()-1][j].type == Road) pos.push_back(Pos(rows()-1, j));
  }
  int num_pos = pos.size();
  assert(num_pos >= nb_players()*nb_cars());

  vector<int> perm2 = random_permutation(num_pos);
  int z = 0;
  for (int pl = 0; pl < nb_players(); ++pl)
    for (int k = 0; k < nb_cars(); ++k)
      new_unit(id, pl, pos[perm2[z++]], Car);
}


// ***************************************************************************


Board::Board (istream& is, int seed) {
  set_random_seed(seed);
  *static_cast<Settings*>(this) = Settings::read_settings(is);
  names_ = vector<string>(nb_players());
  read_generator_and_grid(is);
  round_ = 0;
  num_cities_ = vector<int>(nb_players(), 0);
  total_score_ = vector<int>(nb_players(), 0);
  cpu_status_ = vector<double>(nb_players(), 0);
  unit_ = vector<Unit>(nb_players()*(nb_warriors() + nb_cars()));
  detect_cities();
  generate_units();
  update_vectors_by_player();
  compute_scores();
}


void Board::print_preamble (ostream& os) const {
  os << version() << endl;
  os << "nb_players      " << nb_players() << endl;
  os << "nb_rounds       " << nb_rounds() << endl;
  os << "nb_cities       " << nb_cities() << endl;
  os << "nb_warriors     " << nb_warriors() << endl;
  os << "nb_cars         " << nb_cars() << endl;
  os << "warriors_health " << warriors_health() << endl;
  os << "cars_fuel       " << cars_fuel() << endl;
  os << "damage          " << damage() << endl;
  os << "rows            " << rows() << endl;
  os << "cols            " << cols() << endl;
}


void Board::print_names (ostream& os) const {
  os << "names          ";
  for (int pl = 0; pl < nb_players(); ++pl) os << ' ' << name(pl);
  os << endl;
}


void Board::print_state (ostream& os) const {
  os << endl << endl;

  for (int i = 0; i < rows(); ++i) {
    for (int j = 0; j < cols(); ++j) {
      const Cell& c = grid_[i][j];
      if (c.type == Wall) os << 'X';
      else if (c.type == Road) os << 'R';
      else if (c.type == Station) os << 'S';
      else if (c.type == Water) os << 'W';
      else if (c.owner == -1) os << '.';
      else if (player_ok(c.owner)) os << c.owner;
      else assert(false);
    }
    os << endl;
  }

  os << endl;
  os << "round " << round() << endl;

  os << "num_cities";
  for (auto nc : num_cities_) os << " " << nc;
  os << endl;

  os << "total_score";
  for (auto ts : total_score_) os << " " << ts;
  os << endl;

  os << "status";
  for (auto st : cpu_status_) os << " " << st;
  os << endl;

  for (int id = 0; id < nb_units(); ++id) print_unit(unit(id), os);
  os << endl;
}


void Board::print_results () const {
  int max_score = 0;
  vector<int> v;
  for (int pl = 0; pl < nb_players(); ++pl) {
    cerr << "info: player " << name(pl)
         << " got score " << total_score(pl) << endl;
    if (total_score(pl) > max_score) {
      max_score = total_score(pl);
      v = vector<int>(1, pl);
    }
    else if (total_score(pl) == max_score) v.push_back(pl);
  }

  cerr << "info: player(s)";
  for (int pl : v) cerr << " " << name(pl);
  cerr << " got top score" << endl;
}


// ***************************************************************************


void Board::place (int id, Pos p) {
  unit_[id].pos = p;
  grid_[p.i][p.j].id = id;
}


bool Board::adjacent (int id, CellType t) const {
  Pos q = unit(id).pos;
  for (int d = 0; d < 8; ++d) {
    Pos p = q + Dir(d);
    if (pos_ok(p) and cell(p).type == t) return true;
  }
  return false;
}


bool Board::pos_safe (Pos p) const {
  for (int x = -4; x <= 4; ++x)
    for (int y = -4; y <= 4; ++y) {
      Pos q = p + Pos(x, y);
      if (pos_ok(q) and grid_[q.i][q.j].id != -1) return false;
    }
  return true;
}


void Board::spawn_cars (const vector<int>& dead_c) {
  vector<vector<int>> dist(rows(), vector<int>(cols(), -1));
  queue<Pos> Q;
  for (int i = 0; i < rows(); ++i)
    for (int j = 0; j < cols(); ++j) {
      int id = grid_[i][j].id;
      if (id != -1) {
        dist[i][j] = 0;
        Q.push(Pos(i, j));
      }
    }

  while (not Q.empty()) {
    Pos q = Q.front(); Q.pop();
    for (int d = 0; d < 8; ++d) {
      Pos p = q + Dir(d);
      if (pos_ok(p) and dist[p.i][p.j] == -1) {
        dist[p.i][p.j] = dist[q.i][q.j] + 1;
        Q.push(p);
      }
    }
  }

  int morts = dead_c.size();

  vector<Pos> pos;
  for (int i = 1; i < rows(); ++i) {
    if (grid_[i][0].type == Road and dist[i][0] >= 4) pos.push_back(Pos(i, 0));
    if (grid_[i][cols()-1].type == Road and dist[i][cols()-1] >= 4) pos.push_back(Pos(i, cols()-1));
  }
  for (int j = 1; j < cols(); ++j) {
    if (grid_[0][j].type == Road and dist[0][j] >= 4) pos.push_back(Pos(0, j));
    if (grid_[rows()-1][j].type == Road and dist[rows()-1][j] >= 4) pos.push_back(Pos(rows()-1, j));
  }

  vector<int> perm = random_permutation(morts);
  for (int k = 0; k < morts; ++k) {
    Pos p(-1, -1);
    while (p == Pos(-1, -1) and not pos.empty()) {
      int z = random(0, pos.size() - 1);
      p = pos[z];
      pos[z] = pos.back();
      pos.pop_back();
      if (not pos_safe(p)) p = Pos(-1, -1);
    }

    bool found = (p != Pos(-1, -1));
    for (int m = 1; not found and m < 30; ++m) {
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, m);
        if (grid_[p.i][p.j].type == Road and pos_safe(p)) found = true;
      }
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, 60 - m - 1);
        if (grid_[p.i][p.j].type == Road and pos_safe(p)) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(m, j);
        if (grid_[p.i][p.j].type == Road and pos_safe(p)) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(60 - m - 1, j);
        if (grid_[p.i][p.j].type == Road and pos_safe(p)) found = true;
      }
    }

    for (int m = 0; not found and m < 30; ++m) {
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, m);
        if (grid_[p.i][p.j].type == Road and grid_[p.i][p.j].id == -1) found = true;
      }
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, 60 - m - 1);
        if (grid_[p.i][p.j].type == Road and grid_[p.i][p.j].id == -1) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(m, j);
        if (grid_[p.i][p.j].type == Road and grid_[p.i][p.j].id == -1) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(60 - m - 1, j);
        if (grid_[p.i][p.j].type == Road and grid_[p.i][p.j].id == -1) found = true;
      }
    }

    assert(found);
    place(dead_c[perm[k]], p);
  }
}


void Board::spawn_warriors (const vector<int>& dead_w) {
  vector<vector<int>> dist(rows(), vector<int>(cols(), -1));
  queue<Pos> Q;
  for (int i = 0; i < rows(); ++i)
    for (int j = 0; j < cols(); ++j) {
      int id = grid_[i][j].id;
      if (id != -1) {
        dist[i][j] = 0;
        Q.push(Pos(i, j));
      }
    }

  while (not Q.empty()) {
    Pos q = Q.front(); Q.pop();
    for (int d = 0; d < 8; ++d) {
      Pos p = q + Dir(d);
      if (pos_ok(p) and dist[p.i][p.j] == -1) {
        dist[p.i][p.j] = dist[q.i][q.j] + 1;
        Q.push(p);
      }
    }
  }

  int morts = dead_w.size();

  vector<Pos> pos;
  for (int i = 1; i < rows() - 1; ++i) {
    if (grid_[i][0].type == Desert and dist[i][0] >= 4) pos.push_back(Pos(i, 0));
    if (grid_[i][cols()-1].type == Desert and dist[i][cols()-1] >= 4) pos.push_back(Pos(i, cols()-1));
  }
  for (int j = 1; j < cols() - 1; ++j) {
    if (grid_[0][j].type == Desert and dist[0][j] >= 4) pos.push_back(Pos(0, j));
    if (grid_[rows()-1][j].type == Desert and dist[rows()-1][j] >= 4) pos.push_back(Pos(rows()-1, j));
  }

  vector<int> perm = random_permutation(morts);
  for (int k = 0; k < morts; ++k) {
    Pos p(-1, -1);
    while (p == Pos(-1, -1) and not pos.empty()) {
      int z = random(0, pos.size() - 1);
      p = pos[z];
      pos[z] = pos.back();
      pos.pop_back();
      if (not pos_safe(p)) p = Pos(-1, -1);
    }

    bool found = (p != Pos(-1, -1));
    for (int m = 1; not found and m < 30; ++m) {
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, m);
        if (grid_[p.i][p.j].type == Desert and pos_safe(p)) found = true;
      }
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, 60 - m - 1);
        if (grid_[p.i][p.j].type == Desert and pos_safe(p)) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(m, j);
        if (grid_[p.i][p.j].type == Desert and pos_safe(p)) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(60 - m - 1, j);
        if (grid_[p.i][p.j].type == Desert and pos_safe(p)) found = true;
      }
    }

    for (int m = 0; not found and m < 30; ++m) {
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, m);
        if (grid_[p.i][p.j].type == Desert and grid_[p.i][p.j].id == -1) found = true;
      }
      for (int i = m; not found and i < 60 - m; ++i) {
        p = Pos(i, 60 - m - 1);
        if (grid_[p.i][p.j].type == Desert and grid_[p.i][p.j].id == -1) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(m, j);
        if (grid_[p.i][p.j].type == Desert and grid_[p.i][p.j].id == -1) found = true;
      }
      for (int j = m; not found and j < 60 - m; ++j) {
        p = Pos(60 - m - 1, j);
        if (grid_[p.i][p.j].type == Desert and grid_[p.i][p.j].id == -1) found = true;
      }
    }

    assert(found);
    place(dead_w[perm[k]], p);
  }
}


void Board::next (const vector<Action>& act, ostream& os) {
  int np = nb_players();
  int nu = nb_units();

  // chooses (at most) one movement per unit
  vector<bool> seen(nu, false);
  vector<Movement> v;
  for (int pl = 0; pl < np; ++pl)
    for (const Movement& m : act[pl].v_) {
      int id = m.id;
      Dir dir = m.dir;
      if (not unit_ok(id)) cerr << "warning: id out of range :" << id << endl;
      else {
        Unit u = unit(id);
        if (u.player != pl)
          cerr << "warning: not own unit: " << id << ' ' << u.player
               << ' ' << pl << endl;
        else {
          _my_assert(not seen[id], "More than one command for the same unit.");
          seen[id] = true;
          if (not dir_ok(dir))
            cerr << "warning: direction not valid: " << dir << endl;
          else if (dir != None) {
            if (not can_move(id))
              cerr << "warning: cannot move: " << id << ' ' << pl
                   << ' ' << round() << endl;
            else v.push_back(Movement(id, dir));
          }
        }
      }
    }
  int num = v.size();

  // makes all movements using a random order
  vector<int> perm = random_permutation(num);
  vector<bool> killed(nu, false);
  vector<Movement> actions_done;
  for (int i = 0; i < num; ++i) {
    Movement m = v[perm[i]];
    if (not killed[m.id] and move(m.id, m.dir, killed))
      actions_done.push_back(m);
  }
  os << "movements" << endl;
  Action::print_actions(actions_done, os);

  // reduces health from units that could move (and perhaps kills them)
  for (int id = 0; id < nu; ++id)
    if (not killed[id]) {
      Unit& u = unit_[id];
      assert(ut_ok(u.type));
      if (can_move(id)) {
        if (u.type == Warrior) {
          --u.food;
          --u.water;
          if (u.food == 0 or u.water == 0)
            capture(id, two_different(u.player, u.player)[0], killed);
        }
        else if (u.food > 0) --u.food;
      }
    }

  // spawns units
  vector<int> dead_w, dead_c;
  for (int id = 0; id < nu; ++id)
    if (killed[id]) {
      UnitType t = unit(id).type;
      assert(ut_ok(t));
      (t == Warrior ? dead_w : dead_c).push_back(id);
    }

  spawn_cars(dead_c);

  spawn_warriors(dead_w);

  update_vectors_by_player();

  compute_scores();

  // recharges food
  for (int id = 0; id < nu; ++id)
    if (not killed[id]) {
      Unit& u = unit_[id];
      assert(ut_ok(u.type));
      if (u.type == Warrior and u.player == round()%4
          and cell(u.pos).owner == u.player)
        u.food = warriors_health();
    }

  // recharges water
  for (int id = 0; id < nu; ++id)
    if (not killed[id]) {
      Unit& u = unit_[id];
      assert(ut_ok(u.type));
      if (u.type == Warrior and u.player == round()%4 and adjacent(id, Water))
        u.water = warriors_health();
    }

  // recharges fuel
  for (int id = 0; id < nu; ++id)
    if (not killed[id]) {
      Unit& u = unit_[id];
      assert(ut_ok(u.type));
      if (u.type == Car and can_move(id) and adjacent(id, Station))
        u.food = cars_fuel();
    }

  ++round_;
}


// ***************************************************************************


void Board::read_generator_and_grid (istream& is) {
  is >> generator_;
  if (generator_ == "FIXED") read_grid(is);
  else {
    vector<int> param;
    int x;
    while (is >> x) param.push_back(x);
    if (generator_ == "GENERATOR") generator(param);
    else _my_assert(false, "Unknow grid generator.");
  }
}


bool Board::good_roads (const vector<int>& R) const {
  for (int i = 1; i < (int)R.size(); ++i)
    if (R[i] <= R[i-1] + 4) return false;
  return true;
}


vector<int> Board::choose_roads (int q) {
  int e = random(6, 8);
  int d = random(60 - 8 - 1, 60 - 6 - 1);
  vector<int> P(q - 2);
  do {
    for (int i = 0; i < q - 2; ++i) P[i] = random(e + 5, d - 5);
    sort(P.begin(), P.end());
  } while (not good_roads(P));

  vector<int> R(q);
  R[0] = e;
  for (int i = 0; i < q - 2; ++i) R[i+1] = P[i];
  R[q-1] = d;
  assert(good_roads(R));
  return R;
}


Pos Board::repre (Pos p) {
  return (parent_[p] == p ? p : parent_[p] =repre(parent_[p]));
}


int Board::area (int i, int j) {
  return area_[repre(Pos(i, j))];
}


bool Board::before (const vector<Pos>& V1, const vector<Pos>& V2) {
  return V1.size() > V2.size();
}


void Board::mark (int i, int j, vector<Pos>& Z) {
  if (seen_[i][j]) return;
  seen_[i][j] = true;
  if (grid_[i][j].type != Desert) return;
  bool ok = true;
  for (int d = 0; ok and d < 8; ++d) {
    Pos p = Pos(i, j) + Dir(d);
    if (cell(p).type != Desert) ok = false;
  }
  if (ok) Z.push_back(Pos(i, j));
  mark(i - 1, j, Z);
  mark(i + 1, j, Z);
  mark(i, j - 1, Z);
  mark(i, j + 1, Z);
}


Pos Board::choose_one (const set<Pos>& S) {
  int q = S.size();
  assert(q > 0);
  vector<Pos> V(S.begin(), S.end());
  return V[random(0, q - 1)];
}


void Board::make_city (int pl, vector<Pos>& Z) {
  int q = Z.size();
  assert(q >= 20);
  int j = random(0, q - 1);
  set<Pos> escollits, frontera, altres;
  for (int i = 0; i < q; ++i)
    if (i != j) altres.insert(Z[i]);

  int mida = random(20, min(q, 40));
  Pos ultim = Z[j];
  escollits.insert(ultim);
  while ((int)escollits.size() < mida) {
    for (int d = 0; d < 8; d += 2) {
      Pos p = ultim + Dir(d);
      if (altres.find(p) != altres.end()) {
        altres.erase(p);
        frontera.insert(p);
      }
    }
    ultim = choose_one(frontera);
    escollits.insert(ultim);
    frontera.erase(ultim);
  }
  Z.clear();
  for (Pos p : altres) {
    bool ok = true;
    for (int d = 0; ok and d < 8; ++d)
      if (escollits.find(p + Dir(d)) != escollits.end()) ok = false;
    if (ok) Z.push_back(p);
  }
  for (Pos p : escollits) {
    grid_[p.i][p.j].type = City;
    grid_[p.i][p.j].owner = pl;
  }
}


void Board::make_water (vector<Pos>& Z) {
  int q = Z.size();
  assert(q >= 10);
  int j = random(0, q - 1);
  set<Pos> escollits, frontera, altres;
  for (int i = 0; i < q; ++i)
    if (i != j) altres.insert(Z[i]);

  int mida = random(5, min(q, 15));
  Pos ultim = Z[j];
  escollits.insert(ultim);
  while ((int)escollits.size() < mida) {
    for (int i = -2; i <= 2; ++i)
      for (int j = -2; j <= 2; ++j)
        if (abs(i*j) < 4) {
          int x = ultim.i + i;
          int y = ultim.j + j;
          Pos p(x, y);
          if (altres.find(p) != altres.end()) {
            altres.erase(p);
            frontera.insert(p);
          }
        }
    ultim = choose_one(frontera);
    escollits.insert(ultim);
    frontera.erase(ultim);
  }
  Z.clear();
  for (Pos p : altres) {
    bool ok = true;
    for (int d = 0; ok and d < 8; ++d)
      if (escollits.find(p + Dir(d)) != escollits.end()) ok = false;
    if (ok) Z.push_back(p);
  }
  for (Pos p : escollits) grid_[p.i][p.j].type = Water;
}


void Board::make_wall (Pos ini, int d, set<Pos>& S) {
  int k = -1;
  Pos p = ini;
  while (S.find(p) != S.end()) {
    p += Dir(d);
    ++k;
  }
  int opo = (d + 4)%8;
  p = ini;
  while (S.find(p) != S.end()) {
    p += Dir(opo);
    ++k;
  }
  if (k >= 4) {
    p = ini;
    while (S.find(p) != S.end()) {
      if (random(0, 7)) grid_[p.i][p.j].type = Wall;
      p += Dir(d);
    }
    p = ini;
    while (S.find(p) != S.end()) {
      if (random(0, 7)) grid_[p.i][p.j].type = Wall;
      p += Dir(opo);
    }
  }
}


void Board::make_walls (const vector<Pos>& Z) {
  int q = Z.size();
  if (q == 0) return;
  set<Pos> S(Z.begin(), Z.end());
  make_wall(Z[random(0, q - 1)], 4*random(0, 1), S);
  make_wall(Z[random(0, q - 1)], 2 + 4*random(0, 1), S);
}


inline bool Board::possible_station (int i, int j) const {
  if (grid_[i][j].type != Road) return false;
  if (grid_[i-1][j].type != Road and grid_[i+1][j].type != Road) return false;
  if (grid_[i][j-1].type != Road and grid_[i][j+1].type != Road) return false;
  return true;
}


int Board::basic_distribution () {
  grid_ = vector< vector<Cell> >(60, vector<Cell>(60, char2cell('.')));

  int n = random(5, 7);
  int m = random(5, 7);
  if (n == 5 and m == 5) ++(random(0, 1) ? n : m);
  if (n == 7 and m == 7) --(random(0, 1) ? n : m);
  X_ = choose_roads(n);
  Y_ = choose_roads(m);

  for (int i = 0; i < n; ++i)
    for (int j = 0; j < 60; ++j) grid_[X_[i]][j].type = Road;
  for (int j = 0; j < m; ++j)
    for (int i = 0; i < 60; ++i) grid_[i][Y_[j]].type = Road;

  parent_.clear();
  area_.clear();
  for (int i = 1; i < n; ++i)
    for (int j = 1; j < m; ++j) {
      parent_[Pos(i, j)] = Pos(i, j);
      area_[Pos(i, j)] = (X_[i] - X_[i-1] - 1)*(Y_[j] - Y_[j-1] - 1);
    }

  vector<vector<int>> V(n, vector<int>(m - 1, true));
  vector<vector<int>> H(n - 1, vector<int>(m, true));
  int q = (n - 1)*(m - 1);
  int compo = random(14, min(q, 18));
  while (q > compo) {
    int minim = 1e6, x = -1, y = -1;
    bool hor = false;
    for (int j = 1; j < m; ++j)
      for (int i = 1; i < n - 1; ++i)
        if (H[i][j] and repre(Pos(i, j)) != repre(Pos(i + 1, j))) {
          int a = area(i, j) + area(i + 1, j) + Y_[j] - Y_[j-1] - 1;
          if (a < minim) {
            minim = a;
            hor = true;
            x = i;
            y = j;
          }
        }
    for (int i = 1; i < n; ++i)
      for (int j = 1; j < m - 1; ++j)
        if (V[i][j] and repre(Pos(i, j)) != repre(Pos(i, j + 1))) {
          int a = area(i, j) + area(i, j + 1) + X_[i] - X_[i-1] - 1;
          if (a < minim) {
            minim = a;
            hor = false;
            x = i;
            y = j;
          }
        }

    if (hor) {
      H[x][y] = false;
      Pos r1 = repre(Pos(x, y));
      Pos r2 = repre(Pos(x + 1, y));
      area_[r1] = minim;
      parent_[r2] = r1;
      for (int j = Y_[y-1] + 1; j < Y_[y]; ++j) grid_[X_[x]][j].type = Desert;
    }
    else {
      V[x][y] = false;
      Pos r1 = repre(Pos(x, y));
      Pos r2 = repre(Pos(x, y + 1));
      area_[r1] = minim;
      parent_[r2] = r1;
      for (int i = X_[x-1] + 1; i < X_[x]; ++i) grid_[i][Y_[y]].type = Desert;
    }
    --q;
  }

  seen_ = vector<vector<bool>>(60, vector<bool>(60, false));
  zone_.clear();
  for (int i = 1; i < n; ++i)
    for (int j = 1; j < m; ++j)
      if (repre(Pos(i, j)) == Pos(i, j)) {
        vector<Pos> Z;
        mark(X_[i] - 2, Y_[j] - 2, Z);
        zone_.push_back(Z);
      }
  assert((int)zone_.size() == compo);

  sort(zone_.begin(), zone_.end(), before);

  if (zone_.front().size() > 300 or zone_.back().size() < 10)
    return basic_distribution(); // start again

  return compo;
}


void Board::generator (vector<int> param) {
  int num = param.size();
  _my_assert(num == 0, "GENERATOR requires no parameters.");

  int r = rows();
  int c = cols();
  _my_assert(r == 60 and c == 60, "GENERATOR with unexpected sizes.");

  int compo = basic_distribution();
  vector<vector<Pos>> C, W;
  for (int i = 0; i < 4; ++i) {
    C.push_back(zone_[i]);
    zone_[i].clear();
  }
  for (int i = compo - 1; i >= 0; --i)
    if (not zone_[i].empty() and zone_[i].size() < 20
        and (int)W.size() < compo - 8) {
      W.push_back(zone_[i]);
      zone_[i].clear();
    }
  for (int i = compo - 1; i >= 0; --i)
    if (not zone_[i].empty() and C.size() < 8) {
      C.push_back(zone_[i]);
      zone_[i].clear();
    }
  for (int i = 0; i < compo; ++i)
    if (not zone_[i].empty()) W.push_back(zone_[i]);
  assert(C.size() == 8);
  assert((int)W.size() == compo - 8);

  vector<int> perm = random_permutation(8);
  for (int pl = 0; pl < 4; ++pl)
    for (int i = 0; i < 2; ++i) {
      make_city(pl, C[perm[2*pl+i]]);
      make_walls(C[perm[2*pl+i]]);
    }

  for (int i = 0; i < compo - 8; ++i) {
    make_water(W[i]);
    make_walls(W[i]);
  }

  int n = X_.size();
  int m = Y_.size();
  int r1 = n - random(3, 5);
  vector<int> perm1 = random_permutation(n);
  for (int k = 0; k < r1; ++k) {
    int x = X_[perm1[k]];
    for (int j = 0; j < Y_[0]; ++j) grid_[x][j].type = Desert;
  }

  int r2 = n - random(3, 5);
  vector<int> perm2 = random_permutation(n);
  for (int k = 0; k < r2; ++k) {
    int x = X_[perm2[k]];
    for (int j = Y_[m-1] + 1; j < 60; ++j) grid_[x][j].type = Desert;
  }

  int r3 = m - random(3, 5);
  vector<int> perm3 = random_permutation(m);
  for (int k = 0; k < r3; ++k) {
    int y = Y_[perm3[k]];
    for (int i = 0; i < X_[0]; ++i) grid_[i][y].type = Desert;
  }

  int r4 = m - random(3, 5);
  vector<int> perm4 = random_permutation(m);
  for (int k = 0; k < r4; ++k) {
    int y = Y_[perm4[k]];
    for (int i = X_[n-1] + 1; i < 60; ++i) grid_[i][y].type = Desert;
  }

  vector<Pos> station;
  for (int i = 1; i < 59; ++i)
    for (int j = 1; j < 59; ++j)
      if (possible_station(i, j)) station.push_back(Pos(i, j));
  int ns = station.size();
  assert(ns >= 6);
  int num_stations = random(6, min(8, ns));
  vector<int> perm5 = random_permutation(ns);
  for (int i = 0; i < num_stations; ++i) {
    Pos p = station[perm5[i]];
    grid_[p.i][p.j].type = Station;
  }
}


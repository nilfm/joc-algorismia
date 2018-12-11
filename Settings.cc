#include "Settings.hh"


Settings Settings::read_settings (istream& is) {
  Settings r;
  string s, v;

  // Version, compared part by part.
  istringstream vs(version());
  while (!vs.eof()) {
    is >> s;
    vs >> v;
    assert(s == v);
  };

  is >> s >> r.nb_players_;
  assert(s == "nb_players");
  assert(r.nb_players_ == 4);

  is >> s >> r.nb_rounds_;
  assert(s == "nb_rounds");
  assert(r.nb_rounds_ >= 1);

  is >> s >> r.nb_cities_;
  assert(s == "nb_cities");
  assert(r.nb_cities_ >= 0 and r.nb_cities_%4 == 0);

  is >> s >> r.nb_warriors_;
  assert(s == "nb_warriors");
  assert(r.nb_warriors_ >= 2);

  is >> s >> r.nb_cars_;
  assert(s == "nb_cars");
  assert(r.nb_cars_ >= 1);

  is >> s >> r.warriors_health_;
  assert(s == "warriors_health");
  assert(r.warriors_health_ >= 1);

  is >> s >> r.cars_fuel_;
  assert(s == "cars_fuel");
  assert(r.cars_fuel_ >= 1);

  is >> s >> r.damage_;
  assert(s == "damage");
  assert(r.damage_ >= 1);

  is >> s >> r.rows_;
  assert(s == "rows");
  assert(r.rows_ >= 40);

  is >> s >> r.cols_;
  assert(s == "cols");
  assert(r.cols_ >= 40);

  return r;
}

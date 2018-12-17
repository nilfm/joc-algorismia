#include "Player.hh"

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME rafah


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

	typedef vector<int> vi;
	typedef vector<vi> mi;
	typedef vector<Pos> vp;
	typedef vector<vp> mp;
	typedef pair<int, int> pi;
	typedef vector<pi> vpi;
	typedef vector<vpi> mpi;
	typedef vector<Dir> vd;
	typedef set<Dir> sd;
	typedef set<Pos> sp;
	typedef set<int> si;
	typedef pair<int, Pos> pip;
	typedef pair<pi, Pos> ppip;
	
	int n = 60;
	Pos cm;
	int occup;
	
	mp cities;
	mi city;
	vi act = vi(80, -2);
	
	sd V_all = {Bottom, BR, Right, RT, Top, TL, Left, LB, None};
	sd V_no_null = {Bottom, BR, Right, RT, Top, TL, Left, LB, None};
	
	mi w_d = mi(60, vi(60, -1));
	mi f_d = mi(60, vi(60, -1));
	mi s_d = mi(60, vi(60, -1));
	mi r_d = mi(60, vi(60, -1));
	mi cm_d = mi(60, vi(60, -1));
	mi car_pond = mi(60, vi(60, -1));
	mi warrior_pond = mi(60, vi(60, -1));
	mi danger;
	
	mpi w_f_d = mpi(60, vpi(60, pi(-1, -1)));    //first: food.   second: water.
	mpi f_w_d = mpi(60, vpi(60, pi(-1, -1)));    //first: water.  second: food.
	
	/**
	 * Play method, invoked once per each round.
	 */
	
	void move_warriors(sp& P_occuped){
		if (round() == 0){
			cm = mass_center();
			occup = 15;
			cities = city_pos();
			city = city_num();
			
			vi W = warriors(me());
			for (int id : W){
				Pos p = unit(id).pos;
				act[id] = city[p.i][p.j];
			}
			
			distance_to(Water, w_d, 1, 1, 1);
			distance_to(City, f_d, 1, 1, 1);
			distance_to(Road, r_d, 1, 1, -1);
			distance_to_cm(cm, cm_d, 1, 4, -1);
			adj_to_station(s_d, 1, 4, -1);
			
			locura(f_w_d, w_d);
			locura2(w_f_d, f_d);
			
			pond_cars(car_pond, r_d);
			pond_warriors(warrior_pond, r_d, f_d, w_d);
		}
		
		if (round() == 100) occup = 4;
	
		if (round()% 4 != me()) return;
		
		vi occuped(8, 0);
		danger = attacked_cells();
		
		vi W = warriors(me());
		for (int id : W){
			// Declarations
			Unit u = unit(id);
			Pos p = u.pos;
			Cell P = cell(p);
			int i = p.i;
			int j = p.j;
			int wwf = w_f_d[i][j].second;
			int fwf = w_f_d[i][j].first;
			int wfw = f_w_d[i][j].first;
			int ffw = f_w_d[i][j].second;
			
			// Type of action
			if (P.type == City){ 
				act[id] = city[i][j];
				if (u.water > wwf + 5 and occuped[act[id]] < occup){
					occuped[act[id]]++;
				}
				else{
					act[id] = -2;
				}
			}
			else if (P.type != City and act[id] >= 0){
				act[id] = -2;
			}
			if (act[id] == -2 and u.water > 30){
				act[id] = -1;
				if (!random(0, 2)) act[id] = -3;
			}

			// My base set of directions.
			sd Vi = ok_dir(p, V_all);
			Vi = pos_dir(p, Vi);
			Vi = not_occuped(p, Vi, P_occuped);
			sd V = danger0(p, Vi, danger);
			
			if (V.size() == 0) V = danger1(p, Vi, danger);
			if (V.size() == 0) V = danger2(p, Vi, danger);
			if (V.size() == 0){
				command (id, None);
				continue;
			}
			
			// The directions that optimize important things
			
			Dir d = None;
			int eps = 5;
			
			if (act[id] >= 0){
				// I should attack an enemy warrior inside a city
				Dir d_city_attack = city_warrior_dir(p, V);
				if (d_city_attack != None){
					Unit enemy = unit(cell(p + d_city_attack).id);
					if (enemy.water <= u.water - 10){
						command (id, d = d_city_attack);
						continue;
					}
				}
				
				// I remain in the city
				for (Dir dir : V){
					if (cell(p+dir).type == City){
						d = dir;
						break;
					}
				}
				command(id, d);
				if (is_empty(p+d)) P_occuped.insert(p+d);
				continue;
			}
			
			if (is_adj_to_enemy_car(p)){
				for (Dir dir : V){
					if (is_empty(p+dir) and !is_adj_to_road(p+dir) and cell(p+dir).type != Road){
						d = dir;
						break;
					}
				}
				if (d != None){
					command(id, d);
					P_occuped.insert(p+d);
					continue;
				}
			}
			
			if (act[id] == -3){
				// I should attack an enemy warrior
				Dir d_attack = warrior_dir(p, V);
				if (d_attack != None){
					Unit enemy = unit(cell(p + d_attack).id);
					if (min(enemy.food, enemy.water) <= min(u.food, u.water) + 6){
						command(id, d = d_attack);
						if (is_empty(p+d)) P_occuped.insert(p+d);
						continue;
					}
				}
				
				// I am ready to conquer
				if (u.water > wfw and u.food > search_city(p, d, V, me(), 30)){
					command(id, d);
					if (is_empty(p+d)) P_occuped.insert(p+d);
					continue;
				}
				
				act[id] = -1;
			}
			
			if (act[id] == -2){
				if ((u.water < w_f_d[i][j].second or u.food < w_f_d[i][j].first) and (u.water < f_w_d[i][j].first or u.food < f_w_d[i][j].second)){
					act[id] = -1;
				}
				else{
					command(id, d = opt_dir_2(p, V, w_f_d));
					if (is_empty(p+d)) P_occuped.insert(p+d);
					continue;
				}
			}
			
			if (act[id] == -1){
				// I am already dead
				if ((u.water < w_f_d[i][j].second or u.food < w_f_d[i][j].first) and (u.water < f_w_d[i][j].first or u.food < f_w_d[i][j].second)){
					//do something desperate
					Dir d_city_attack = city_warrior_dir(p, V);
					if (d_city_attack != None){
						Unit enemy = unit(cell(p + d_city_attack).id);
						if (enemy.water <= u.water + 5){
							command (id, d = d_city_attack);
							if (is_empty(p+d)) P_occuped.insert(p+d);
							continue;
						}
					}
					command(id, d = opt_dir(p, V, f_d));
					if (is_empty(p+d)) P_occuped.insert(p+d);
					continue;
				}
				
				// I should attack an enemy warrior
				Dir d_attack = warrior_dir(p, V);
				if (d_attack != None){
					Unit enemy = unit(cell(p + d_attack).id);
					if (min(enemy.food, enemy.water) <= min(u.food, u.water) + 3){
						command(id, d = d_attack);
						if (is_empty(p+d)) P_occuped.insert(p+d);
						continue;
					}
				}
				
				// I am thirsty
				if (u.water < wwf + eps and u.water >= wwf and u.food >= fwf){
					command(id, d = opt_dir_2(p, V, w_f_d));
					if (is_empty(p+d)) P_occuped.insert(p+d);
					continue;
				}

				// I am hungry
				if (u.food < fwf + eps and u.food >= fwf and u.water >= wwf and u.water < 40 - wwf){
					command(id, d = opt_dir_2(p, V, w_f_d));
					if (is_empty(p+d)) P_occuped.insert(p+d);
					continue;
				}
				
				// I am really hungry
				if (u.food < fwf + eps and u.food >= ffw){
					command(id, d = opt_dir_2(p, V, f_w_d));
					if (is_empty(p+d)) P_occuped.insert(p+d);
					continue;
				}
				
				// I don't know what to do
				else{
					command(id, d = opt_dir(p, V, f_d));
					if (is_empty(p+d)) P_occuped.insert(p+d);
				}
			}
			
		}
	}
	
	void move_cars(sp& P_occuped){
		
		vi C = cars(me());
		for (int id : C){
			if (can_move(id)){
				// Declarations
				Unit u = unit(id);
				Pos p = u.pos;
				int i = p.i;
				int j = p.j;
				
				// My base set of directions
				sd V = V_all;
				V = ok_dir(p, V);
				V = not_occuped(p, V, P_occuped);
				V = car_empty_dir(p, V);
				V = no_death_dir(p, V);
				
				Dir d_station = None;
				Dir d_enemy = None;
				Dir d_attack = None;		
				
				int enemy_dist;
				
				Dir d;
				
				if (u.food < s_d[i][j] + 8) command(id, d = opt_dir(p, V, s_d));
				else if (u.food >= 15 and (enemy_dist = dijkstra_enemy_warrior(p, d_enemy, V, 35, car_pond)) != -1) command(id, d = d_enemy);
				else if (u.food < 15 and (enemy_dist = 4*search_enemy_unit(p, Warrior, d_enemy, V, 10)) != -1) command(id, d = d_enemy);
				else if (V.size() > 0) command(id, d = opt_dir(p, V, cm_d));
				else command(id, d = None);
				
				P_occuped.insert(p+d);
			}
		}
	} 

	virtual void play () {
		sp P;
		move_cars(P);
		move_warriors(P);
		
		//if (round ()%4 == me()){
		//	for (int i = 0; i < 60; i++){
		//		for (int j = 0; j < 60; j++) cerr << danger[i][j] << (danger[i][j] < 10 and danger[i][j] >= 0 ? "  " : " ");
		//		cerr << endl;
		//	}
		//}
	}
	
	/*
	* Functions
	*/
		
	//Return true if the cell is an obstacle for a warrior
	bool is_obs (Pos p){
		Cell P = cell(p);
		return (P.type == Wall or P.type == Water or P.type == Station);
	}
	
	//Return true if the cell is an obstacle for a car
	bool is_car_obs (Pos p){
		Cell P = cell(p);
		return (not (P.type == Road or P.type == Desert));
	}
	
	//Return true if a warrior can move to this cell
	bool is_empty (Pos p){
		Cell P = cell(p);
		return P.id == -1 and !is_obs(p);
	}
	
	//Return if moving to this cell makes sense
	bool is_reachable (Pos p){
		Cell P = cell(p);
		if (P.id != -1){
			return unit(P.id).player != me();
		}
		return !is_obs(p);
	}
	
	//Return true if a car can move to this cell
	bool is_car_empty (Pos p){
		Cell P = cell(p);
		return ((P.id == -1 and !is_car_obs(p)) or (P.id != -1 and unit(P.id).player != me())) ;
	}

	//Return true if a cell have a car
	bool have_a_car (Pos p){
		Cell P = cell(p);
		if (P.id != -1) return unit(P.id).type == Car;
		return false;
	}
	
	//Return true if a cell have a warrior
	bool have_a_warrior (Pos p){
		Cell P = cell(p);
		if (P.id != -1) return unit(P.id).type == Warrior;
		return false;
	}
	
	//Return true if a cell have an enemy unit
	bool have_enemy (Pos p){
		Cell P = cell(p);
		if (P.id != -1) return unit(P.id).player != me();
		return false;
	}
	
	//Return true if a cell have an enemy car
	bool have_enemy_car (Pos p){
		Cell P = cell(p);
		if (have_a_car(p)) return unit(P.id).player != me();
		return false;
	}
	
	//Return true if a cell have an enemy warrior
	bool have_enemy_warrior (Pos p){
		Cell P = cell(p);
		if (have_a_warrior(p)) return unit(P.id).player != me();
		return false;
	}

	//Return true if a cell is adjacent to a cell with an enemy car
	bool is_adj_to_enemy_car (Pos p){
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd) and have_enemy_car(pd)) return true;
		}
		return false;
	}
	
	//Return true if a cell is adjacent to a cell with an enemy warrior
	bool is_adj_to_enemy_warrior (Pos p){
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd)){
				if (have_enemy_warrior(pd)) return true;
			}
		}
		return false;
	}
	
	//Return true if a cell is adjacent to a cell with an enemy unit
	bool is_adj_to_enemy (Pos p){
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd)){
				Cell P = cell(pd);
				if (P.id != -1){
					Unit u = unit(P.id);
					if (u.player != me()) {
						return true;
					}
				}
			}
		}
		return false;
	}
	
	//Return true if a cell is adjacent to a water cell
	bool is_adj_to_water (Pos p){
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd) and cell(pd).type == Water) return true;
		}
		return false;
	}
	
	//Return true if a cell is adjacent to a station cell
	bool is_adj_to_station (Pos p){
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd) and cell(pd).type == Station) return true;
		}
		return false;
	}
	
	//Return true if a cell is adjacent to a road cell
	bool is_adj_to_road (Pos p){
		if (cell(p).type == Road) return false;
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd) and cell(pd).type == Road) return true;
		}
		return false;
	}
	
	//Select the direction that takes to a cell inside the map
	sd ok_dir(Pos p, sd& V){
		sd V_ok;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd)) V_ok.insert(d);
		}
		return V_ok;
	}
	
	//Select the directions those which are possible to reach
	sd pos_dir (Pos p, sd& V){
		sd V_pos;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd) and is_reachable(pd)) V_pos.insert(d);
		}
		return V_pos;
	}
	
	//Select the directions those which are possible to reach for a car
	sd pos_car_dir (Pos p, sd& V){
		sd V_pos;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd) and !is_car_obs(pd)) V_pos.insert(d);
		}
		return V_pos;
	}
	
	//Select from the directions those which are empty
	sd empty_dir (Pos p, sd& V){
		sd V_empty;
		for (Dir d : V){
			Pos pd = p + d;
			if ((pos_ok(pd) and is_empty(pd)) or d == None) V_empty.insert(d);
		}
		return V_empty;
	}
	
	//Select from the directions those which are empty for a car
	sd car_empty_dir (Pos p, sd& V){
		sd V_empty;
		for (Dir d : V){
			Pos pd = p + d;
			if ((pos_ok(pd) and is_car_empty(pd)) or d == None) V_empty.insert(d);
		}
		return V_empty;
	}
	
	//Select from the possible directions those which don't take the unit to an instant death
	sd no_instant_death_dir (Pos p, sd& V){
		sd V_alive;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd) and (!have_a_car(pd) or d == None)) V_alive.insert(d);
		}
		return V_alive;
	}
	
	//Select from the possible directions those which don't take the unit to a secure death
	sd no_death_dir (Pos p, sd& V){
		sd V_alive;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd) and !have_enemy_car(pd) and (cell(pd).type == City or !is_adj_to_enemy_car(pd))) V_alive.insert(d);
		}
		return V_alive;
	}
	
	//Select from the possible directions those which take the unit to a cell where can't be attacked
	sd no_problem_dir (Pos p, sd& V){ //If I want to use it for cars I have to modify it first with (or d == None)
		sd V_free;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd) and !have_a_car(pd) and !is_adj_to_enemy(pd)) V_free.insert(d);
		}
		return V_free;
	}
	
	//Show which directions have a enemy unit
	sd enemy_dir (Pos p){
		sd V;
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd) and have_enemy(pd)) V.insert(d);
		}
		return V;
	}
	
	//Show which directions have a enemy unit
	sd no_city_enemy_warrior_dir (Pos p){
		sd V;
		for (Dir d : V_no_null){
			Pos pd = p + d;
			if (pos_ok(pd) and have_enemy_warrior(pd) and cell(pd).type != City) V.insert(d);
		}
		return V;
	}
	
	sd danger0 (Pos p, sd& V, mi& danger){
		sd V0;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd)){
				if (cell(pd).id == -1 and danger[pd.i][pd.j] == 0) V0.insert(d);
				else if (cell(pd).id != -1 and danger[p.i][p.j] == 0) V0.insert(d);
			}
		}
		return V0;
	}
	
	sd danger1 (Pos p, sd& V, mi& danger){
		sd V1;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd)){
				if (cell(pd).id == -1 and danger[pd.i][pd.j] <= 1) V1.insert(d);
				else if (cell(pd).id != -1 and danger[p.i][p.j] <= 1) V1.insert(d);
			}
		}
		return V1;
	}
	
	sd danger2 (Pos p, sd& V, mi& danger){
		sd V2;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd)){
				if (cell(pd).id == -1 and danger[pd.i][pd.j] <= 2) V2.insert(d);
				else if (cell(pd).id != -1 and danger[p.i][p.j] <= 2) V2.insert(d);
			}
		}
		return V2;
	}
	
	//Move a warrior in a random direction
	void move_random_warrior (int id){
		vi v = random_permutation(8);
		for (int i = 0; i < 8; ++i){
			Pos p = unit(id).pos + Dir(v[i]);
			if (pos_ok(p) and is_empty(p)) {
				command(id, Dir(v[i]));
				return;
			}
		}
	}
	
	//Move a car in a random direction
	void move_random_car (int id){
		vi v = random_permutation(8);
		for (int i = 0; i < 8; ++i){
			Pos p = unit(id).pos + Dir(v[i]);
			if (pos_ok(p) and is_car_empty(p)) {
				command(id, Dir(v[i]));
				return;
			}
		}
	}
	
	//Select a direction with an enemy warrior
	Dir warrior_dir(Pos p, sd V){
		for (Dir d : V){
			Pos pd = p + d;
				if (pos_ok(pd)){
				Cell P = cell(pd);
				if (P.id != -1){
					Unit u = unit(P.id);
					if (u.type == Warrior and u.player != me()){
						if (cell(pd).type != City) return d;
					}
				}
			}
		}
		return None;
	}
	
	//Select a direction with an enemy warrior
	Dir city_warrior_dir(Pos p, sd V){
		for (Dir d : V){
			Pos pd = p + d;
				if (pos_ok(pd)){
				Cell P = cell(pd);
				if (P.id != -1){
					Unit u = unit(P.id);
					if (u.type == Warrior and u.player != me()){
						if (cell(pd).type == City) return d;
					}
				}
			}
		}
		return None;
	}
	
	//Select a random direcion from a direction set
	Dir random_dir(Pos p, sd V){
		if(V.size() == 0) return None;
		int rnd = random(0, V.size()-1);
		int i = 0;
		for (Dir d : V){
			if (i == rnd) return d;
			i++;
		}
		return None;
	}
	
	//Select the optimal direction to go for water, food or fuel
	Dir opt_dir(Pos p, sd V, mi m_d){
		Dir dir = None;
		int dist = -1;
		for (Dir d : V){
			Pos pd = p+d;
			int i = pd.i;
			int j = pd.j;
			if ((dist == -1 or dist > m_d[i][j]) and m_d[i][j] > -1){
				dist = m_d[i][j];
				dir = d;
			}
		}
		return dir;
	}
	
	//Select the optimal direction to go for water or food passing through food or water
	Dir opt_dir_2(Pos p, sd V, mpi m){
		Dir dir = None;
		pi dist(-1, -1);
		for (Dir d : V){
			Pos pd = p+d;
			int i = pd.i;
			int j = pd.j;
			if (dist == pi(-1,-1) or dist > m[i][j]){
				dist = m[i][j];
				dir = d;
			}
		}
		return dir;
	}
	
	//Dir bored_car (Pos p, sd V, mi car_pond){

	//Select those directions which are not already occuped
	sd not_occuped(Pos p, sd V, sp P_occuped){
		sd V_pos;
		for (Dir d : V){
			Pos pd = p + d;
			if (pos_ok(pd) and !P_occuped.count(pd)) V_pos.insert(d);
		}
		return V_pos;
	}
		
	//Switch directions into chars			
	char d2c (Dir d) {
		switch (d) {
		  case Bottom: return 'b';
		  case BR:     return 'w';
		  case Right:  return 'r';
		  case RT:     return 'x';
		  case Top:    return 't';
		  case TL:     return 'y';
		  case Left:   return 'l';
		  case LB:     return 'z';
		  case None:   return 'n';
		  default:     assert(false);
		}
	}
	
	//Return the distance and direction to the closest enemy unit of type U
	int search_enemy_unit(Pos P, UnitType U, Dir& D, sd V, int it){
		queue<Pos> q;
		set<Pos> s;
		map<Pos, int> mdist;
		map<Pos, Dir> mdir;
		
		s.insert(P);
		mdist[P] = 0;
		mdir[P] = None;
		
		for (Dir d : V){
			if (d != None){
				Pos pd = P+d;
				if (pos_ok(pd)){
					s.insert(pd);
					q.push(pd);
					mdist[pd] = 1;
					mdir[pd] = d;
				}
			}
		}
		
		while(!q.empty()){
			Pos p = q.front();
			q.pop();
			
			if (mdist[p] > it) return -1;
			
			if (cell(p).id != -1){
				Unit u = unit(cell(p).id);
				if (u.type == U){
					if (u.player != me()){
						D = mdir[p];
						return mdist[p];
					}
				}
			}
			
			for(Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					if (!s.count(pd) and !is_car_obs(pd)){
						s.insert(pd);
						q.push(pd);
						mdist[pd] = mdist[p]+1;
						mdir[pd] = mdir[p];
					}
				}
			}
		}
		return -1;
	}
	
	//Dijkstra algorithm to search an enemy car
	bool have_close_enemy_car (Pos P){
		priority_queue<pip> q;
		
		set<Pos> s;
		s.insert(P);
		
		map<Pos, int> dist;
		dist[P] = 0;
		map<Pos, Dir> dir;
		dir[P] = None;
		
		for (Dir d : V_no_null){
			Pos pd = P+d;
			if (pos_ok(pd)){
				int c = car_pond[pd.i][pd.j];
				if (c >= 0){
					dist[pd] = c;
					dir[pd] = d;
					q.push(pip(-c,pd));
				}
			}
		}
		
		while (!q.empty()){
			Pos p = q.top().second;
			q.pop();
			if (s.count(p)) continue;
			s.insert(p);
			
			if (dist[p] > 3) return false;
			
			if (cell(p).id != -1){
				Unit u = unit(cell(p).id);
				if (u.type == Car and u.player != me()){
					return true;
				}
			}
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					int c = 4;
					if (cell(pd).type == Road) c = 1;
					
					if (c > 0 and (dist[pd] == 0 or dist[p]+c < dist[pd])){
						dist[pd] = dist[p]+c;;
						dir[pd] = dir[p];
						q.push(pip(-dist[pd],pd));
					}
				}
			}
		}
		return false;
	}
	
	//Return the distance and diretion to the closest city that doesn't belong to "notown".
	int search_city (Pos P, Dir& D, sd V, int notown, int it){
		queue<Pos> q;
		set<Pos> s;
		map<Pos, int> mdist;
		map<Pos, Dir> mdir;
		
		s.insert(P);
		mdist[P] = 0;
		mdir[P] = None;
		
		for (Dir d : V){
			if (d != None){
				Pos pd = P+d;
				if (pos_ok(pd)){
					if (is_empty(pd)){
						s.insert(pd);
						q.push(pd);
						mdist[pd] = 1;
						mdir[pd] = d;
					}
				}
			}
		}
		
		while(!q.empty()){
			Pos p = q.front();
			q.pop();
			
			if (mdist[p] > it) return -1;
			
			if (cell(p).type == City){
				if (cell(p).owner != notown){
					D = mdir[p];
					return mdist[p];
				}
			}
			
			for(Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					if (!s.count(pd) and !is_obs(pd)){
						s.insert(pd);
						q.push(pd);
						mdist[pd] = mdist[p]+1;
						mdir[pd] = mdir[p];
					}
				}
			}
		}
		return -1;
	}
	
	//Return the distance and direction to the closest cell adjacent to a Water cell
	int search_station(Pos P, Dir& D, sd V, int it){
		queue<Pos> q;
		set<Pos> s;
		map<Pos, int> mdist;
		map<Pos, Dir> mdir;
		
		s.insert(P);
		mdist[P] = 0;
		mdir[P] = None;
		
		for (Dir d : V){
			if (d != None){
				Pos pd = P+d;
				if (pos_ok(pd)){
					if (is_empty(pd)){
						s.insert(pd);
						q.push(pd);
						mdist[pd] = 1;
						mdir[pd] = d;
					}
				}
			}
		}
		
		while(!q.empty()){
			Pos p = q.front();
			q.pop();
			
			if (mdist[p] > it) return -1;
			
			for(Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					if (cell(pd).type == Station){
						D = mdir[p];
						return mdist[p];
					}
					
					if (!s.count(pd) and !is_car_obs(pd)){
						s.insert(pd);
						q.push(pd);
						mdist[pd] = mdist[p]+1;
						mdir[pd] = mdir[p];
					}
				}
			}
		}
		return -1;
	}
	
	//Return the distance and diretion to the closest city that doesn't belong to "notown".
	int search_road (Pos P, Dir& D, sd V, int it){
		queue<Pos> q;
		set<Pos> s;
		map<Pos, int> mdist;
		map<Pos, Dir> mdir;
		
		s.insert(P);
		mdist[P] = 0;
		mdir[P] = None;
		
		for (Dir d : V){
			if (d != None){
				Pos pd = P+d;
				if (pos_ok(pd)){
					if (is_empty(pd)){
						s.insert(pd);
						q.push(pd);
						mdist[pd] = 1;
						mdir[pd] = d;
					}
				}
			}
		}
		
		while(!q.empty()){
			Pos p = q.front();
			q.pop();
			
			if (mdist[p] > it) return -1;
			
			if (cell(p).type == Road){
				D = mdir[p];
				return mdist[p];
			}
			
			for(Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					if (!s.count(pd) and !is_obs(pd)){
						s.insert(pd);
						q.push(pd);
						mdist[pd] = mdist[p]+1;
						mdir[pd] = mdir[p];
					}
				}
			}
		}
		return -1;
	}
	
	//Return the minimum of two integers, supposing that -1 is larger than any positive number
	int min_mod(int n, int m){
		if (n == -1) return m;
		if (m == -1) return n;
		if (n > m) return m;
		return n;
	}
	
	//Dijkstra algorithm to search an enemy warrior
	int dijkstra_enemy_warrior(Pos P, Dir& D, sd V, int it, mi car_pond){
		priority_queue<pip> q;
		
		set<Pos> s;
		s.insert(P);
		
		map<Pos, int> dist;
		dist[P] = 0;
		map<Pos, Dir> dir;
		dir[P] = None;
		
		for (Dir d : V){
			Pos pd = P+d;
			if (pos_ok(pd)){
				int c = car_pond[pd.i][pd.j];
				if (c >= 0){
					dist[pd] = c;
					dir[pd] = d;
					q.push(pip(-c,pd));
				}
			}
		}
		
		while (!q.empty()){
			Pos p = q.top().second;
			q.pop();
			if (s.count(p)) continue;
			s.insert(p);
			
			if (cell(p).id != -1){
				Unit u = unit(cell(p).id);
				if (u.type == Warrior and u.player != me() and cell(p).type != City){
					D = dir[p];
					return dist[p];
				}
			}
			
			if (dist[p] > it) return -1;
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					int c = car_pond[pd.i][pd.j];
					
					if (c > 0 and (dist[pd] == 0 or dist[p]+c < dist[pd])){
						dist[pd] = dist[p]+c;;
						dir[pd] = dir[p];
						q.push(pip(-dist[pd],pd));
					}
				}
			}
		}
		return -1;
	}
	
	//Dijkstra algorithm to search a station
	int dijkstra_station(Pos P, Dir& D, sd V, int it){
		priority_queue<pip> q;
		
		set<Pos> s;
		s.insert(P);
		
		map<Pos, int> dist;
		dist[P] = 0;
		map<Pos, Dir> dir;
		dir[P] = None;
		
		for (Dir d : V){
			Pos pd = P+d;
			if (pos_ok(pd)){
				if (cell(pd).type == Road){
					dist[pd] = 1;;
					dir[pd] = d;
					q.push(pip(-1,pd));
				}
				if (cell(pd).type == Desert){
					dist[pd] = 4;;
					dir[pd] = d;
					q.push(pip(-4,pd));
				}
			}
		}
		
		while (!q.empty()){
			Pos p = q.top().second;
			q.pop();
			if (s.count(p)) continue;
			s.insert(p);
			
			if (dist[p] > it) return -1;
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					if (cell(pd).type == Station){
						D = dir[p];
						return dist[p];
					}
					
					int c;
					if (cell(pd).type == Road) c = 1;
					else if (cell(pd).type == Desert) c = 4;
					
					if (dist[pd] == 0 or dist[p]+c < dist[pd]){
						dist[pd] = dist[p]+c;;
						dir[pd] = dir[p];
						q.push(pip(-dist[pd],pd));
					}
				}
			}
		}
		cerr << endl << "PROBLEM" << endl << endl;
		return -1;
	}
	
	//Compute the distance from each cell of the map to the nearest cell of type CT
	void distance_to(CellType CT, mi& m_d, int road_pond, int desert_pond, int city_pond){
		mi v(60, vi(60, 0));
		
		priority_queue<pip> q;
		for (int i = 0; i < 60; i++){
			for (int j = 0; j < 60; j++){
				Pos p(i,j);
				if (cell(p).type == CT){
					m_d[i][j] = 0;
					q.push(pip(0, p));
				}
			}
		}
		
		while (!q.empty()){
			Pos p = q.top().second;
			int i = p.i;
			int j = p.j;
			q.pop();
			
			if (v[i][j]) continue;
			v[i][j] = 1;
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					int id = pd.i;
					int jd = pd.j;
					
					if (cell(pd).type == Road){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + road_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
					else if (cell(pd).type == Desert){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + desert_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
					else if (city_pond != -1 and cell(pd).type == City){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + city_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
				}
			}
		}
	}
	
	//Compute the distance from each cell of the map to the nearest cell of type CT
	void adj_to_station(mi& m_d, int road_pond, int desert_pond, int city_pond){
		mi v(60, vi(60, 0));
		
		priority_queue<pip> q;
		for (int i = 0; i < 60; i++){
			for (int j = 0; j < 60; j++){
				Pos p(i,j);
				if (is_adj_to_station(p) and cell(p).type != Station){
					m_d[i][j] = 0;
					q.push(pip(0, p));
				}
			}
		}
		
		while (!q.empty()){
			Pos p = q.top().second;
			int i = p.i;
			int j = p.j;
			q.pop();
			
			if (v[i][j]) continue;
			v[i][j] = 1;
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					int id = pd.i;
					int jd = pd.j;
					
					if (cell(pd).type == Road){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + road_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
					else if (cell(pd).type == Desert){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + desert_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
					else if (city_pond != -1 and cell(pd).type == City){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + city_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
				}
			}
		}
	}

	//Fill the matrix ->f->w
	void locura(mpi& f_w_d, mi& w_d){
		priority_queue<ppip> q;
		for (int i = 0; i < 60; i++){
			for (int j = 0; j < 60; j++){
				Pos p(i,j);
				if (cell(p).type == City){
					f_w_d[i][j].first = w_d[i][j]-1;
					f_w_d[i][j].second = 0;
					q.push(ppip(pi(-w_d[i][j], 0), p));
				}
			}
		}
		
		while (!q.empty()){
			Pos p = q.top().second;
			int i = p.i; int j = p.j;
			q.pop();
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd) and !is_obs(pd)){
					int id = pd.i; int jd = pd.j;
					
					if (f_w_d[id][jd].first == -1){
						f_w_d[id][jd].first = f_w_d[i][j].first + 1;
						f_w_d[id][jd].second = f_w_d[i][j].second + 1;
						
						q.push(ppip(pi(-f_w_d[id][jd].first, -f_w_d[id][jd].second), pd));
					}
				}
			}
		}
	}
	
	//Fill the matrix ->w->f
	void locura2(mpi& f_w_d, mi& w_d){
		priority_queue<ppip> q;
		for (int i = 0; i < 60; i++){
			for (int j = 0; j < 60; j++){
				Pos p(i,j);
				if (!is_obs(p) and is_adj_to_water(p)){
					f_w_d[i][j].first = w_d[i][j];
					f_w_d[i][j].second = 0;
					q.push(ppip(pi(-w_d[i][j], 0), p));
				}
			}
		}
		
		while (!q.empty()){
			Pos p = q.top().second;
			int i = p.i; int j = p.j;
			q.pop();
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd) and !is_obs(pd)){
					int id = pd.i; int jd = pd.j;
					
					if (f_w_d[id][jd].first == -1){
						f_w_d[id][jd].first = f_w_d[i][j].first + 1;
						f_w_d[id][jd].second = f_w_d[i][j].second + 1;
						
						q.push(ppip(pi(-f_w_d[id][jd].first, -f_w_d[id][jd].second), pd));
					}
				}
			}
		}
	}

	//Assign values to each cell depending on priority for a car
	void pond_cars(mi& car_pond, mi& r_d){
		for (int i = 0; i < 60; ++i){
			for (int j = 0; j < 60; ++j){
				Pos p(i,j);
				if (r_d[i][j] == -1) car_pond[i][j] = -1;
				else if (r_d[i][j] == 0) car_pond[i][j] = 1;
				else if (r_d[i][j] == 1) car_pond[i][j] = 4;
				else car_pond[i][j] = r_d[i][j]*4;
				
				if (i < 5) car_pond[i][j] += (5-i)*2;
				if (j < 5) car_pond[i][j] += (5-j)*2;
				if (i > 54) car_pond[i][j] += (i - 54)*2;
				if (j > 54) car_pond[i][j] += (j - 54)*2;
			}
		}
	}
	
	//Assign values to each cell depending on priority for a warrior
	void pond_warriors(mi& warrior_pond, mi& r_d, mi& f_d, mi& w_d){
		for (int i = 0; i < 60; ++i){
			for (int j = 0; j < 60; ++j){
				Pos p(i,j);
				
				warrior_pond[i][j] += 1 + f_d[i][j];
				
				if (r_d[i][j] == -1) car_pond[i][j] = -1;
				else if (r_d[i][j] == 0) warrior_pond[i][j] *= 3;
				else if (r_d[i][j] == 1) warrior_pond[i][j] *= 2;
				
				if (cell(p).type == City) warrior_pond[i][j] = 0;
			}
		}
	}

	Pos mass_center() {
		int icm, jcm;
		icm = jcm = 0;
		int s = 0;
		for (int i = 0; i < 60; ++i){
			for (int j = 0; j < 60; ++j){
				if (cell(i,j).type == City){
					icm += i;
					jcm += j;
					s++;
				}
			}
		}
		icm = icm/s;
		jcm = jcm/s;
		return Pos(icm, jcm);
	}

	//Compute the distance from each cell of the map to the nearest cell of type CT
	void distance_to_cm(Pos cm, mi& m_d, int road_pond, int desert_pond, int city_pond){
		mi v(60, vi(60, 0));
		
		priority_queue<pip> q;
		m_d[cm.i][cm.j] = 0;
		q.push(pip(0, cm));

		
		while (!q.empty()){
			Pos p = q.top().second;
			int i = p.i;
			int j = p.j;
			q.pop();
			
			if (v[i][j]) continue;
			v[i][j] = 1;
			
			for (Dir d : V_no_null){
				Pos pd = p+d;
				if (pos_ok(pd)){
					int id = pd.i;
					int jd = pd.j;
					
					if (cell(pd).type == Road){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + road_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
					else if (cell(pd).type == Desert){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + desert_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
					else if (city_pond != -1 and cell(pd).type == City){
						m_d[id][jd] = min_mod(m_d[id][jd], m_d[i][j] + city_pond);
						q.push(pip(-m_d[id][jd], pd));
					}
				}
			}
		}
	}

	//Function cities
	mp city_pos(){
		mp cities(8, vp(0));
		mi visited(60, vi(60, 0));
		int city = 0;
		for (int i = 0; i < 60; ++i){
			for (int j = 0; j < 60; ++j){
				Pos p(i,j);
				if (cell(p).type == City and (not visited[i][j])){
					cities[city].push_back(p);
					visited[i][j] = 1;
					queue<Pos> q;
					q.push(p);
					
					while (!q.empty()){
						Pos p = q.front(); q.pop();
						for (Dir d : V_no_null){
							Pos pd = p+d;
							if (pos_ok(pd) and cell(pd).type == City and (not visited[pd.i][pd.j])){
								cities[city].push_back(pd);
								visited[pd.i][pd.j] = 1;
								q.push(pd);
							}
						}
					}
					city++;
				}
			}
		}
		return cities;
	}

	//Matrix city
	mi city_num(){
		mi city(60, vi(60, -1));
		mi visited(60, vi(60, 0));
		int c = 0;
		for (int i = 0; i < 60; ++i){
			for (int j = 0; j < 60; ++j){
				Pos p(i,j);
				if (cell(p).type == City and (not visited[i][j])){
					city[i][j] = c;
					visited[i][j] = 1;
					queue<Pos> q;
					q.push(p);
					
					while (!q.empty()){
						Pos p = q.front(); q.pop();
						for (Dir d : V_no_null){
							Pos pd = p+d;
							if (pos_ok(pd) and cell(pd).type == City and (not visited[pd.i][pd.j])){
								city[pd.i][pd.j] = c;
								visited[pd.i][pd.j] = 1;
								q.push(pd);
							}
						}
					}
					c++;
				}
			}
		}
		return city;
	}

	//Attacked cells
	mi attacked_cells(){
		mi danger(60, vi(60, 0));
		
		for (int i = 0; i < 60; ++i){
			for (int j = 0; j < 60; ++j){
				Pos p(i,j);
				int id = cell(p).id;
				if (id != -1 and unit(id).type == Car and unit(id).player != me()){
					danger[i][j] += 10;
					queue<pip> q;
					sp v;
					q.push(pip(4, p));
					v.insert(p);
					
					while (!q.empty()){
						Pos p = q.front().second; 
						int c = q.front().first;
						q.pop();
						for (Dir d : V_no_null){
							Pos pd = p+d;
							if (pos_ok(pd) and !v.count(pd)){
								if (cell(pd).type == Road){
									danger[pd.i][pd.j] += c;
									v.insert(pd);
									if (c > 1) q.push(pip(c-1, pd));
								}
								else if (cell(pd).type == Desert){
									danger[pd.i][pd.j] += c;
									v.insert(pd);
								}
							}
						}
					}
				}
			}
		}
		return danger;
	}

};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);

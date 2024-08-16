string mapgen;

void wallin() {
  for(int y=0; y<size_y; y++) {
    board[y][0].be_blocked();
    board[y][size_x-1].be_blocked();
    }
  for(int x=0; x<size_x; x++) {
    board[0][x].be_blocked();
    board[size_y-1][x].be_blocked();
    }
  }

void create_point_barriers(int prob) {
  mapgen += "Point obstacles (probability " + to_string(prob) + "%)<br/>";
  for(int y=0; y<size_y; y++) 
  for(int x=0; x<size_x; x++) 
    if(hrand(100) < prob) board[y][x].be_blocked();
  }

void create_point_barriers_removable(int prob) {
  mapgen += "Point obstacles (probability " + to_string(prob) + "%)<br/>";
  for(int y=0; y<size_y; y++) 
  for(int x=0; x<size_x; x++) 
    if(hrand(100) < prob) board[y][x].be_brem();
  }

void create_line_barriers(int qx, int qy) {
  if(qx) mapgen += "Horizontal line obstacles (" + to_string(qx) + ")<br/>";
  for(int i=0; i<qx; i++) {
    int x1 = hrange(0, size_x-1);
    int x2 = hrange(0, size_x-1);
    int y = hrange(2, size_y-3);
    if(x2 < x1) swap(x1, x2);
    for(int x=x1; x<=x2; x++) board[y][x].be_brem();
    }
  if(qy) mapgen += "Vertical line obstacles (" + to_string(qy) + ")<br/>";
  for(int i=0; i<qy; i++) {
    int y1 = hrange(0, size_y-1);
    int y2 = hrange(0, size_y-1);
    int x = hrange(2, size_x-3);
    if(y2 < y1) swap(y1, y2);
    for(int y=y1; y<=y2; y++) board[y][x].be_brem();
    }
  }

void fill_all() {
  for(int y=0; y<size_y; y++) 
  for(int x=0; x<size_x; x++) {
    board[y][x].be_blocked();
    if(x && y && x < size_x-1 && y < size_y-1)
      board[y][x].be_brem();
    }
  }

void create_rogue(int pb) {

  fill_all();

  int xrooms = size_x / (10 + hrand(10));
  int yrooms = size_y / (10 + hrand(10));
  
  int x1[20][20], y1[20][20], x2[20][20], y2[20][20];

  mapgen += "Roguelike (" + to_string(xrooms) + "x" + to_string(yrooms) + " rooms)<br/>";

  for(int y=0; y<yrooms; y++) 
  for(int x=0; x<xrooms; x++) {
    int lx = x * size_x / xrooms + 2; int hx = (x+1) * size_x / xrooms - 3;
    int ly = y * size_y / yrooms + 2; int hy = (y+1) * size_y / yrooms - 3;
    
    x1[y][x] = lx + hrand((hx - lx) / 2 - 1);
    x2[y][x] = hx - hrand((hx - lx) / 2 - 1);
    y1[y][x] = ly + hrand((hy - ly) / 2 - 1);
    y2[y][x] = hy - hrand((hy - ly) / 2 - 1);

    for(int by=y1[y][x]; by<=y2[y][x]; by++) for(int bx=x1[y][x]; bx<=x2[y][x]; bx++) board[by][bx].be_free();
    }
  
  if(pb) create_point_barriers(pb);

  int d = hrand(100);
  if(d < 5) create_point_barriers(5);
  else if(d < 10) create_point_barriers(1);
  else if(d < 12) create_point_barriers(10);

  for(int y=0; y<yrooms; y++) 
  for(int x=0; x<xrooms-1; x++) {
    int sx = x2[y][x]+1; int tx = x1[y][x+1]-1;
    int sy = hrange(y1[y][x], y2[y][x]);
    int ty = hrange(y1[y][x+1], y2[y][x+1]);
    int swx = hrange(sx+1, tx-1);
    for(int ax=x1[y][x]; ax<=swx; ax++) board[sy][ax].be_free();
    for(int ax=swx; ax<=x2[y][x+1]; ax++) board[ty][ax].be_free();
    if(sy > ty) swap(sy, ty); for(int y=sy; y<=ty; y++) board[y][swx].be_free();
    }

  for(int y=0; y<yrooms-1; y++) 
  for(int x=0; x<xrooms; x++) {
    int sy = y2[y][x]+1; int ty = y1[y+1][x]-1;
    int sx = hrange(x1[y][x], x2[y][x]);
    int tx = hrange(x1[y+1][x], x2[y+1][x]);
    int swy = hrange(sy+1, ty-1);
    for(int ay=y1[y][x]; ay<=swy; ay++) board[ay][sx].be_free();
    for(int ay=swy; ay<=y2[y+1][x]; ay++) board[ay][tx].be_free();
    if(sx > tx) swap(sx, tx); for(int x=sx; x<=tx; x++) board[swy][x].be_free();
    }

  }

coord ufind(coord x) {
  if(bat(x).fudata == x) return x;
  return bat(x).fudata = ufind(bat(x).fudata);
  }

void funion(coord x, coord y) {
  bat(ufind(x)).fudata = ufind(y);
  }

bool have_dq = false;
double dsq[128][128];

// should be good enough approx
double randgauss() {
  return hrand(1000) + hrand(1000) - hrand(1000) - hrand(1000);
  }

// larger param -- focus on bigger frequencies

double dsqparam;

void gen_diamondsquare(double param) {
  have_dq = true; dsqparam = param;
  dsq[0][0] = 0;
  for(int step=64; step >= 1; step >>= 1) {
    for(int ph=0; ph<2; ph++) 
    for(int x=0; x<128; x += step)
    for(int y=0; y<128; y += step) {
      auto mix = [&] (int y1, int x1, int y2, int x2, int y3, int x3, int y4, int x4, int var) {
        dsq[y][x] = dsq[y1&127][x1&127] + dsq[y2&127][x2&127] + dsq[y3&127][x3&127] + dsq[y4&127][x4&127];
        dsq[y][x] /= 4;
        dsq[y][x] += pow(var, param) * randgauss();
        };
      if(ph == 0 && (x&step) && (y & step))
        mix(y-step, x-step, y-step, x+step, y+step, x-step, y+step, x+step, step*step*2);
      if(ph == 1 && ((x&step) ^ (y & step)))
        mix(y-step, x, y+step, x, y, x-step, y, x+step, step*step);
      }
    }
  }

// variant 0: remove always
// variant 1: remove only if produces new connections
// variant 2: remove only if produces new connections OR extends regions
// (with prob, may still remove even if condition not satisfied)

void connect_fu(int variant, int prob = 0) {
  
  if(variant == 0) mapgen += "Remove walls until connected";
  if(variant == 1) mapgen += "Remove walls until connected, but only if they connect something";
  if(variant == 2) mapgen += "Remove walls until connected, but only if they connect something or extend regions";
  if(prob)  mapgen += " (or with probability "+to_string(prob)+"%)";
  mapgen += "<br/>";
  if(have_dq) {
    stringstream dp; dp << dsqparam;
    mapgen += "removal order based on diamond-square (parameter " + dp.str() + ")<br/>";
    }

  bat(source).be_free();
  bat(target).be_free();

  vector<coord> to_remove;

  for(int y=0; y<size_y; y++)
  for(int x=0; x<size_x; x++) board[y][x].fudata = coord(x, y);

  for(int y=0; y<size_y; y++)
  for(int x=0; x<size_x; x++) {
    coord at(x, y);
    if(!board[y][x].blocked) {
      for(auto d: dirs) if(!bat(at + d).blocked) funion(at, at + d);
      }
    else if(board[y][x].removable) {
      to_remove.push_back(at);
      swap(to_remove.back(), to_remove[hrand(to_remove.size())]);
      }
    }

  if(have_dq)
    sort(to_remove.begin(), to_remove.end(), [] (coord a, coord b) {
      return dsq[a.y][a.x] < dsq[b.y][b.x];
      });
  
  for(coord at: to_remove) {
    if(ufind(source) == ufind(target)) break;
    set<coord> ex;
    if(variant && hrand(100) > prob) {
      int qty = 0;
      for(auto d: dirs) if(!bat(at + d).blocked) { qty++; ex.insert(ufind(at+d)); }
      if(variant == 1 && ex.size() == 1) continue;
      if(variant == 2 && (ex.size() == 1 && qty > 1)) continue;
      }
    bat(at).blocked = false;
    for(auto d: dirs) if(!bat(at + d).blocked) funion(at, at + d);
    }

  for(int y=0; y<size_y; y++)
  for(int x=0; x<size_x; x++) if(!board[y][x].blocked && ufind(coord(x,y)) != ufind(source)) {
    board[y][x].blocked = true;
    }
  }

void maze() {
  mapgen += "basic grid layout<br/>";
  for(int y=1; y<size_y-1; y++)
  for(int x=1; x<size_x-1; x++) {
    if((y^x) & 1) board[y][x].be_brem();
    else if(x&1) board[y][x].be_blocked();
    }
  }

void nothing() {}

void endcorners() {
  mapgen += "Create small obstacles close to the exit<br/>";
  if(source.x < target.x) board[target.y][target.x-1].be_blocked();
  if(source.x > target.x) board[target.y][target.x+1].be_blocked();
  if(source.y < target.y) board[target.y-1][target.x].be_blocked();
  if(source.y > target.y) board[target.y+1][target.x].be_blocked();
  }

void startcorners() {
  mapgen += "Create small obstacles close to the start<br/>";
  if(source.x < target.x) board[source.y][source.x+1].be_blocked();
  if(source.x > target.x) board[source.y][source.x-1].be_blocked();
  if(source.y < target.y) board[source.y+1][source.x].be_blocked();
  if(source.y > target.y) board[source.y-1][source.x].be_blocked();
  }

std::function<void()> after;

void gen_board() {
  restart:
  have_dq = false;
  size_x = 40 + hrand(41);
  size_y = 20 + hrand(31);
  lsize = min(1000 / size_x, 600 / size_y);

  board.clear();
  board.resize(size_y);
  for(int y=0; y<size_y; y++) board[y].resize(size_x);
  after = nothing;
  bool ignore_blocks = false;

  mapgen = "";
  wallin();

  int algo = hrand(90);
  printf("trying algorithm %d\n", algo);

  if(algo < 10) {
    after = [] { if(hrand(100) < 50) startcorners(); if(hrand(100) < 50) endcorners(); };
    }
  else if(algo < 20) {
    create_point_barriers(hrpick({1, 2, 5, 10, 20}));
    after = [] { if(hrand(100) < 50) startcorners(); if(hrand(100) < 50) endcorners(); };
    }
  else if(algo < 30) {
    create_rogue(hrpick({0, 0, 0, hrpick({1, 2, 5, 10, 20})}));
    }
  else if(algo < 40) {
    create_point_barriers(hrpick({1, 2, 5, 10, 20}));
    create_line_barriers(hrpick({0, 0, 1, 2, 5, 10, 15, 20}), 0);
    create_line_barriers(0, hrpick({0, 0, 1, 2, 5, 10, 15, 20}));
    wallin();
    after = [] { if(hrand(100) < 50) startcorners(); if(hrand(100) < 50) endcorners(); connect_fu(1, 0); };
    }
  else if(algo < 50) {
    maze(); 
    after = [] { connect_fu(1, hrpick({0, 0, 0, 0, 5, 10, 20, 50})); };
    }
  else if(algo < 60) {
    fill_all(); ignore_blocks = true;
    after = [] { connect_fu(0, 0); };
    }
  else if(algo < 70) {
    fill_all(); ignore_blocks = true;
    after = [] { connect_fu(2, 0); };
    }
  else if(algo < 80) {
    fill_all(); ignore_blocks = true;
    gen_diamondsquare(hrpickf({0.1, 0.25, 0.5, 1}));
    if(hrand(100) < 20) create_point_barriers(hrpick({1, 2, 5, 10, 20}));
    after = [] { connect_fu(0, 0); };
    }
  else if(algo < 90) {
    maze();
    gen_diamondsquare(hrpickf({0.1, 0.25, 0.5, 1}));
    after = [] { connect_fu(1, 0); };
    }

  for(int att=0;; att++) {
    source.x = 1 + hrand(size_x-2);
    source.y = 1 + hrand(size_y-2);
    if(ignore_blocks || !bat(source).blocked)  break;
    if(att > 1000) { printf("failed to generate start\n"); goto restart; }
    }
  
  for(int att=0;; att++) {
    target.x = 1 + hrand(size_x-2);
    target.y = 1 + hrand(size_y-2);
    if((ignore_blocks || !bat(target).blocked) && dist_actual(source, target) >= (size_x+size_y)/2)  break;
    if(att > 1000) { printf("failed to generate end\n"); goto restart; }
    }

  after();
  }


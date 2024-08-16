#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include "visutils.h"
#include <emscripten/fetch.h>
#include <random>

int lsize = 40;

bool game_running;
bool is_daily;
int daily;

int score, curscore, attempts;

bool current_solved;

void draw_board();

int size_x, size_y;

int my_guess, answer;

struct coord {
  int x, y;
  coord(int x, int y) : x(x), y(y) {}
  coord() {}
  coord operator + (const coord& b) const { return coord(x+b.x, y+b.y); }
  coord operator - (const coord& b) const { return coord(x-b.x, y-b.y); }
  coord operator - () const { return coord(-x, -y); }
  bool operator < (const coord& b) const { return tie(x,y) < tie(b.x, b.y); }
  bool operator == (const coord& b) const { return tie(x,y) == tie(b.x, b.y); }
  bool operator != (const coord& b) const { return tie(x,y) != tie(b.x, b.y); }
  coord mirror() { return coord(y, x); }
  };

array<coord, 4> dirs = { coord(1,0), coord(0,1), coord(-1,0), coord(0,-1) };

struct exploreitem {
  int age;
  int steps;
  coord where;
  int lastmove;
  int bends;
  int lastbend;
  };

struct tile {
  bool blocked;
  bool onpath;
  bool bfs_visited;
  bool removable;
  int explore_index;
  exploreitem ei;
  coord fudata;
  void be_free() { blocked = removable = false; }
  void be_blocked() { blocked = true; removable = false; }
  void be_brem() { blocked = true; removable = true; }
  tile() { blocked = false; bfs_visited = false; explore_index = 0; onpath = false; }
  };

vector<vector<tile>> board;

coord source, target;

int gameseed;

std::mt19937 game_rng;

int hrand(int i, std::mt19937& which = game_rng) {
  unsigned d = which() - which.min();
  long long m = (long long) (which.max() - which.min()) + 1;
  m /= i;
  d /= m;
  if(d < (unsigned) i) return d;
  return hrand(i);
  }

int hrange(int i, int j, std::mt19937& which = game_rng) {
  return i + hrand(j - i + 1, which);
  }

int hrpick(vector<int> vals, std::mt19937& which = game_rng) {
  return vals[hrand(vals.size())];
  }

double hrpickf(vector<double> vals, std::mt19937& which = game_rng) {
  return vals[hrand(vals.size())];
  }

tile& bat(coord c) { return board[c.y][c.x]; }

double dist_max(coord a, coord b) {
  return max(abs(a.x-b.x), abs(a.y-b.y));
  }
double dist_euc(coord a, coord b) {
  return hypot(abs(a.x-b.x), abs(a.y-b.y));
  }
double dist_actual(coord a, coord b) {
  return abs(a.x-b.x) + abs(a.y-b.y);
  }

int distance_mode = 0;
double factor = 1;

double heuristic(const exploreitem& e) {
  double d;
  if(distance_mode == 0) d = dist_actual(target, e.where);
  if(distance_mode == 1) d = dist_euc(target, e.where);
  if(distance_mode == 2) d = dist_max(target, e.where);
  return e.steps + d * factor;
  }

vector<int> tiebreakers, tiebreaker_applied;

vector<string> tbnames = {"heuristic + steps", "steps", "turns", "Euclidean distance", "Manhattan distance", "max distance", 
  "age", "absolute dir (E=0 S=1 W=2 N=3)", "relative dir (left=1 right=2 forward=3 started=4)"
  };

double apply(int index, const exploreitem& e) {
  if(index >= tiebreakers.size()) {
    tiebreakers.push_back(2 + hrand(14));
    tiebreaker_applied.push_back(0);
    }

  int tb = tiebreakers[index];
  int sign = (tb & 1) ? -1 : +1;
  tb /= 2;

  if(tb == 0) return heuristic(e);
  if(tb == 1) return e.steps;
  if(tb == 2) return e.bends;
  if(tb == 3) return dist_euc(target, e.where);
  if(tb == 4) return dist_actual(target, e.where);
  if(tb == 5) return dist_max(target, e.where);
  if(tb == 6) return -e.age;
  if(tb == 7) return e.lastmove;
  if(tb == 8) return e.lastbend;

  return 0;
  }

int comparisons;

bool operator < (const exploreitem& e1, const exploreitem& e2) {
  comparisons++;
  for(int i=0;; i++) {
    auto h1 = apply(i, e1);
    auto h2 = apply(i, e2);
    if(h1 != h2) { tiebreaker_applied[i]++; return h1 > h2; }
    }
  }

std::priority_queue<exploreitem> explores;

int explore_count;

bool astar() {
  explores = {};
  exploreitem ei; ei.steps = 0; ei.where = source; ei.lastmove = -1; ei.bends = 0;
  explores.push(ei);

  explore_count = 0;

  comparisons = 0;

  int steps = 0;
  while(true) {
    // steps++; if(steps == 10) return true;
    if(explores.empty()) return false;
    ei = explores.top(); explores.pop();
    tile& at = bat(ei.where);
    if(at.explore_index) continue;
    at.explore_index = ++explore_count;
    at.ei = ei;
    if(ei.where == target) {
      auto t = target;
      while(t != source) { bat(t).onpath = true; t = t - dirs[bat(t).ei.lastmove]; }
      return true;
      }
    for(int d=0; d<4; d++) {
      auto ei2 = ei;
      ei2.where = ei.where + dirs[d];
      if(bat(ei2.where).blocked || bat(ei2.where).explore_index) continue;
      ei2.age = explore_count;
      ei2.steps++;
      ei2.lastmove = d;
      if(ei.lastmove >= 0 && ((ei.lastmove + d) % 2)) ei2.bends++;
      if(ei2.lastbend == -1) ei2.lastbend = 4;
      else if(ei2.lastbend == ei.lastmove) ei2.lastbend = 3;
      else if(ei2.lastbend == (ei.lastmove + 1) % 4) ei2.lastbend = 2;
      else if(ei2.lastbend == (ei.lastmove + 3) % 4) ei2.lastbend = 1;
      explores.push(ei2);
      }
    }
  }

int bfs_steps, bfs_distance;

bool bfs() {
  std::queue<coord> q;
  std::queue<int> len;
  bfs_steps = 0;
  auto visit = [&] (coord at, int d) { if(bat(at).bfs_visited || bat(at).blocked) return; bat(at).bfs_visited = true; q.push(at); len.push(d); };
  visit(source, 0);
  while(!q.empty()) {
    bfs_steps++;
    auto at = q.front(); q.pop();
    bfs_distance = len.front(); len.pop();
    if(at == target) return true;
    for(int d=0; d<4; d++) visit(at + dirs[d], bfs_distance + 1);
    }
  bfs_steps = -1; return false;
  }

#include "mapgen.cpp"

int freetiles;

int ask_about;

void gen_puzzle() {
  restart:

  gen_board();
  tiebreaker_applied = tiebreakers = {0};
  factor = 1; distance_mode = 0;
  int h = hrand(5);
  if(h == 3) distance_mode = 1;
  if(h == 4) distance_mode = 2;
  int h1 = hrand(10);
  if(h1 == 8) factor = 1.5;
  if(h1 == 9) factor = 2;
  if(h1 == 7) factor = 0.75;
  if(h1 == 6) factor = 0.5;
  if(h1 == 5) factor = 0;

  if(!astar()) {} // goto restart;
  bfs();
  current_solved = false;

  freetiles = 0;
  for(int y=0; y<size_y; y++) for(int x=0; x<size_x; x++) if(!board[y][x].blocked) freetiles++;

  ask_about = 0;
  if(attempts >= 5 && hrand(100) < 25) ask_about = 1;
  }

void render_tile(pic& p, pic& p2, int x, int y) {
  coord co(x, y);
  style bblock(0xFFFFFFFF, 0xFFC0C0C0, 1);
  auto& t = board[y][x];

  if(t.blocked) {
    path pa(bblock);
    pa.add(vec(x*lsize, y*lsize));
    pa.add(vec(x*lsize, y*lsize+lsize));
    pa.add(vec(x*lsize+lsize, y*lsize+lsize));
    pa.add(vec(x*lsize+lsize, y*lsize));
    pa.add(vec(x*lsize, y*lsize));
    pa.cycled = true;
    p += pa;
    }

  if(current_solved && t.explore_index) {
    color col = 0xFF000000;
    int q = t.explore_index * 255 / explore_count;
    col += (q << 0);
    col += ((255 - q) << 8);
    style bblock(0xFFC0C0C0, col, 1);
    path pa(bblock);
    pa.add(vec(x*lsize, y*lsize));
    pa.add(vec(x*lsize, y*lsize+lsize));
    pa.add(vec(x*lsize+lsize, y*lsize+lsize));
    pa.add(vec(x*lsize+lsize, y*lsize));
    pa.add(vec(x*lsize, y*lsize));
    pa.cycled = true;
    p += pa;

    if(t.ei.lastmove >= 0 && t.onpath) {
      // if(part(col, 0) < 128) part(col, 0) = 128;
      // if(part(col, 1) < 128) part(col, 1) = 128;
      style bline(0xFFFFFFFF, 0, 7);
      path pa1(bline);
      pa1.add(vec(x*lsize + lsize/2, y*lsize + lsize/2));
      pa1.add(vec(x*lsize + lsize/2 - lsize * dirs[t.ei.lastmove].x, y*lsize + lsize/2 - lsize * dirs[t.ei.lastmove].y));
      p2 += pa1;
      }

    if(t.ei.lastmove >= 0) {
      // if(part(col, 0) < 128) part(col, 0) = 128;
      // if(part(col, 1) < 128) part(col, 1) = 128;
      style bline(t.onpath ? 0xFF000000 : 0xFFFFFFFF, 0, 3);
      path pa1(bline);
      pa1.add(vec(x*lsize + lsize/2, y*lsize + lsize/2));
      pa1.add(vec(x*lsize + lsize/2 - lsize * dirs[t.ei.lastmove].x, y*lsize + lsize/2 - lsize * dirs[t.ei.lastmove].y));
      p2 += pa1;
      }
    }

  style bgold(0, 0xFFFFD500, 0);

  if(co == source) {
    font ff = makefont("DejaVuSans-Bold.ttf", ";font-family:'DejaVu Sans';font-weight:bold");
    text t1(bgold, ff, vec(x*lsize+lsize*.5, y*lsize+lsize*.35), center, lsize * .9, "@");
    p += t1;
    }

  if(co == target) {
    font ff = makefont("DejaVuSans-Bold.ttf", ";font-family:'DejaVu Sans';font-weight:bold");
    text t1(bgold, ff, vec(x*lsize+lsize*.5, y*lsize+lsize*.35), center, lsize * .9, ">");
    p += t1;
    }

  }

void draw_board() {
  pic p;

  style blines(0xFFC0C0C0, 0, 1);

  for(int y=0; y<=size_y; y++) {
    path pa(blines);
    pa.add(vec(0, y*lsize));
    pa.add(vec(size_x*lsize, y*lsize));
    p += pa;
    }
    
  for(int x=0; x<=size_x; x++) {
    path pa(blines);
    pa.add(vec(x*lsize, 0));
    pa.add(vec(x*lsize, size_y*lsize));
    p += pa;
    }

  pic p2;
    
  for(int y=0; y<size_y; y++)
  for(int x=0; x<size_x; x++) {
    render_tile(p, p2, x, y);
    }

  p += p2;
  stringstream ss;

  ss << "<center>\n";

  ss << SVG_to_string(p);

  ss << "</center><br/><br/>";

  ss << "<div style=\"float:left;width:30%\">&nbsp;</div>";
  ss << "<div style=\"float:left;width:20%\">";

  ss << "map " << size_x << "x" << size_y << " (" << freetiles << " free tiles)<br/>";
  ss << "Manhattan distance: " << dist_actual(source, target) << "</br></br>";

  ss << "<b>Procgen algorithm:</b><br/>";
  ss << mapgen;

  ss << "</div><div style=\"float:left;width:20%\">";
  ss << "<b>Heuristics:</b><br/>";
  if(factor != 0) {
    if(distance_mode == 0) ss << "Manhattan distance<br/>";
    if(distance_mode == 1) ss << "Euclidean distance<br/>";
    if(distance_mode == 2) ss << "max distance<br/>";
    }
  if(factor != 1) {
    if(factor > 1) ss << "<font color='red'>";

    ss << "(factor " << factor << ")<br/>";
    if(factor > 1) ss << " <b>(inadmissible!)</b></font>";
    }

  ss << "<br/><b>Tiebreakers:</b><br/>";
  int id = 0;
  for(auto tb: tiebreakers) {
    int qty = tiebreaker_applied[id++];
    if(!qty) continue;
    bool flip = (tb & 1);
    tb /= 2;
    if(flip) ss << "<tt>-</tt> ";
    else ss << "<tt>+</tt> ";
    ss << tbnames[tb];
    if(current_solved) ss << " (x" << qty << ")";
    ss << "<br/>";
    }

  ss << "<br/><br/>";

  ss << "</div>";

  ss << "<div style=\"float:left;width:20%\">";

  if(ask_about == 0)
    ss << "<b>How many tiles visited by A*?</b><br/>";
  else
    ss << "<b>How many comparisons does A* do?</b><br/>";

  if(!current_solved) {
    ss << "<input id=\"guess\" length=10 type=text/><br/>";
    add_button(ss, "g = document.getElementById(\"guess\").value; if(g == \"\") alert(\"please enter value\"); else guess(g)", "guess!");
    if(attempts) ss << "<br/><br/>You got " << score << " points in " << attempts << " attempts so far<br/>";
    }
  else {
    ss << "<b>Your guess: " << my_guess << " correct answer: " << answer << " (" << curscore << " points)</b><br/>";
    if(attempts) ss << "You got " << score << " points in " << attempts << " attempts<br/>";
    if(daily && attempts == 10)
      add_button(ss, "view_new_game()", "daily finished!");
    add_button(ss, "moveon()", "next!");

    ss << "<br/><br/>";
    ss << "A* visits " << explore_count << " tiles<br/>";
    ss << "A* performs " << comparisons << " comparisons<br/>";
    ss << "BFS visits " << bfs_steps << " tiles<br/>";
    ss << "shortest path: " << bfs_distance << " tiles<br/>";
    int asol = bat(target).ei.steps;
    if(asol != bfs_distance)
      ss << "A* found " << asol << " due to inadmissible heuristics<br/>";
    }

  ss << "<br/><br/><a onclick='view_help()'>view help / new game</a>";
  ss << "</div></div>";

  set_value("output", ss.str());
  }

char secleft[64];

void score_guess(int my_guess) {
  answer = 0;
  if(ask_about == 0) answer = explore_count;
  if(ask_about == 1) answer = comparisons;
  double d;
  if(my_guess < answer) d = my_guess * 1. / answer;
  else d = answer * 1. / my_guess;
  d = pow(d, 1.5); 
  curscore = int(d * 100);
  score += curscore; attempts++;
  }

extern "C" {

void view_new_game() {
  stringstream ss;

  ss << "<div style=\"float:left;width:30%\">&nbsp;</div>";
  ss << "<div style=\"float:left;width:40%\">";

  if(attempts) {
    ss << "You got " << score << " points in " << attempts << " attempts<br/>";
    ss << "<a onclick='back_to_game()'>back to game!</a><br/><br/>";
    }

  ss << "In this game, you are given a map with obstacles, and your task is to guess how efficient the A* algorithm will be at finding the ";
  ss << "shortest path from the entrance ('@') to the exit ('>').";

  ss << "<h2>What is A*?</h2>";

  ss
    << "Let us start with Breadth First Search (BFS). In this algorithm, we proceed in waves: find all locations next to the source, "
    << "then next to these locations, etc., until we reach the target. It is obvious that we will find the shortest path to the target this way, "
    << "and also easy to implement on the computer: all the locations we reach are simply put in a queue, the location we explore next is always "
    << "the one which was reached the earliest.<br/><br/>";

  ss
    << "In A*, instead, we instead try to more promising directions first. While in BFS, the order is generally based on (the number of steps), "
    << "in A* it is (the number of steps so far + heuristics), where 'heuristics' is some approximation of the the number of steps we still need to take "
    << "(for example, the number of steps to the exit, not taking obstacles into account). If the heuristics is 'admissible' (not greater than the actual length of path to "
    << "the exit) it is also guaranteed to find the shortest path. (See <a href=\"https://www.redblobgames.com/pathfinding/a-star/introduction.html\">Red Blob Games</a> for more details.)<br/><br/>";

  ss
    << "<h2>Glossary</h2>"
    << "<b>Manhattan distance</b> is the number of steps if you can go over obstacles but only North, East, West, and South (not diagonally).<br/>"
    << "<b>Euclidean distance</b> is the distance along the diagonal (computed from the Pythogarean Theorem).<br/>"
    << "<b>Max distance</b> is the maximum of the difference in X coordinates, and the difference in the Y coordinates.<br/>"
    << "<b>Factor</b> means that the distance is multiplied by something before being added to the number of steps. Values greater than 1 "
    << "are not admissible and thus the path found may be not the shortest (but hopefully still relatively short and found faster).<br/>"
    << "<b>Tiebreaker</b> is a rule used for ordering reached locations when (steps+heuristics) has the same value. Some ideas are good, some are very bad. '+' means smaller value explored first, '-' means larger value explored first. (The numbers by heuristics correspond to the number of times a rule was successful at tie-breaking.)<br/>"
    << "<b>Comparisons</b> is the number of comparisons used by the A* algorithm. We use a simple implementation here based on the 'priority queue' "
    << "from C++, i.e., a binary heap. Entering a new location to the priority queue, or removing the current best candidate from it, generally "
    << "requires a number of comparisons proportional to the logarithm of number of choices currently on the priority queue.<br/>"
    << "<b>Diamond-square</b> is an algorithm used to generate landscapes. Smaller parameter = more focus on local features than global.<br/>"
    << "<br/>";

  ss << "<h2>Commentary</h2>";
  ss << "A* seems to be the goto algorithm used by game developers for pathfinding purposes. But it rarely seems that good (even if the heuristics and tiebreakers are chosen reasonably). ";
  ss << "Maybe BFS (with its simplicity and good performance tue to the avoidance of priority queues), or some algorithm that takes into account the fact that you are usually pathfinding many times, could be more appropriate for your game?";

  if(game_running) {
    ss << "<a onclick='back_to_game()'>back to game!</a><br/><br/>";
    }

  ss << "<br/><br/>";

  add_button(ss, "restart(document.getElementById(\"seed\").value)", "start a free game");

  ss << " seed: <input id=\"seed\" length=10 type=text/> | ";


  string sl = secleft;

  add_button(ss, "restart(\"daily\")", "start a daily game #" + to_string(daily) + " (" + sl + " left)");
  ss << "<br/><br/>";

  ss << "</div></div>";
  set_value("output", ss.str());
  }

void view_help() {
  view_new_game();
  /* stringstream ss;

  ss << "<div style=\"float:left;width:30%\">&nbsp;</div>";
  ss << "<div style=\"float:left;width:40%\">"; 
  ss << "No rules!</br>";
  ss << "<br/><a onclick='back_to_game()'>back to game!</a>";
  ss << "</div>"; 
  ss << "</div>"; 

  set_value("output", ss.str()); */
  }


void restart(const char *s) {

  is_daily = false;
  string s0 = s;

  if(s0 == "daily") is_daily = true;

  if(is_daily) gameseed = daily;
  else if(!s[0]) gameseed = time(NULL);
  else gameseed = atoi(s);
  game_rng.seed(gameseed);

  score = 0; attempts = 0;
  
  gen_puzzle();
  draw_board();
  }

void moveon() {
  if(current_solved) { gen_puzzle(); draw_board(); }
  }

void guess(const char *s) {
  my_guess = atoi(s);
  current_solved = true;
  score_guess(my_guess);
  draw_board();
  }

void back_to_game() {
  draw_board();
  }

}

void check_daily_time() {
  time_t t = time(NULL);
  struct tm *res = localtime(&t);
  int seconds_into = res->tm_hour * 3600 + res->tm_min * 60 + res->tm_sec;
  res->tm_hour = 0;
  res->tm_min = 0;
  res->tm_sec = 0;
  time_t t1 = timegm(res);
  daily = t1 / 3600 / 24 - 19843;
  int seconds_left = 3600*24 - seconds_into;
  sprintf(secleft, "%02d:%02d:%02d", seconds_left / 3600, (seconds_left / 60) % 60, seconds_left % 60);
  daily -= 107;
  }

int init(bool _is_mobile) {
  check_daily_time();
  view_new_game();
  return 0;
  }

extern "C" {
  void start(bool mobile) { gameseed = time(NULL); init(mobile); }
  }
                                  
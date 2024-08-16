
#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <stdio.h>
#include <math.h>
#include <complex>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

using namespace std;

typedef long double ld;
#ifndef Size
#define Size(x) (int((x).size()))
#endif

const char *fontfile = "VeraBd.ttf";

// colors (color)
// just as in SDL, color has the following format: 0xAARRGGBB
//==============

typedef unsigned int color;
typedef unsigned char colorpart;

ld cta(color col) {
  col >>= 24;
  col &= 0xFF;
  return col / 255.0;
  }

bool isfilled(color col) { return col & 0xFF000000; }

int gltc(color col) {
  return ((col >> 8) & 0xFFFFFF) | ((col << 24) & 0xFF000000);
  // (col & 0xFFFFFF) << 8 + (Uint32(col & 0xFF000000)) >> 24;
  }

// mix 2 colors (no alpha)
color mixcolor(color col, color ncol) {
  return ((col&0xFEFEFEFE) + (ncol&0xFEFEFEFE)) >> 1;
  }

// i-th part of a color (0=B, 1=G, 2=R, 3=A), can write to
colorpart& part(color& col, int i) {
  colorpart* c = (colorpart*) &col;
  return c[i];
  }

colorpart mixpart(colorpart a, colorpart b, colorpart c) {
  return (a*(255-c)+b*c+127)/255;
  }

void alphablend(color& col, color ncol, colorpart nv) {
  int s3 = part(ncol, 3);
  for(int i=0; i<3; i++) {
    part(col, i) = 
      (part(col, i) * (255-s3) + part(ncol, i) * s3 + 128) >> 8;
    }
  part(col, 3) = nv;
  }

// 2d vector
//===========

struct xform;

typedef complex<ld> cld;

struct vec {
  ld x, y;
  vec(ld _x, ld _y) : x(_x), y(_y) { }
  vec(cld c) : x(real(c)), y(imag(c)) { }
  vec() : x(0), y(0) {}
  virtual void tform(const xform& x);
  };

cld asconj(vec v) { return cld (v.x, v.y); }

vec& operator += (vec& a, const vec b) { a.x += b.x; a.y += b.y; return a; }
vec& operator -= (vec& a, const vec b) { a.x -= b.x; a.y -= b.y; return a; }

// coordinatewise multiplication and division
vec& operator *= (vec& a, const vec b) { a.x *= b.x; a.y *= b.y; return a; }
vec& operator *= (vec& a, ld scalar) { a.x *= scalar; a.y *= scalar; return a; }
vec& operator /= (vec& a, const vec b) { a.x /= b.x; a.y /= b.y; return a; }
vec& operator /= (vec& a, ld scalar) { a.x /= scalar; a.y /= scalar; return a; }

vec  operator +  (vec a, const vec b) { return a+=b; }
vec  operator -  (vec a, const vec b) { return a-=b; }
vec  operator *  (vec a, const vec b) { return a*=b; }
vec  operator /  (vec a, const vec b) { return a/=b; }

vec  operator *  (vec a, ld scalar)   { return a*=scalar; }
vec  operator *  (ld scalar, vec a)   { return a*=scalar; }
vec  operator /  (vec a, ld scalar)   { return a/=scalar; }

vec operator -   (vec a) { return vec(-a.x, -a.y); }


// cross product
ld   operator ^  (const vec a, const vec b) { return a.x*b.y - a.y*b.x; }

// scalar product
ld   operator |  (const vec a, const vec b) { return a.x*b.x + a.y*b.y; }

ld len(const vec& v) { return sqrt(v|v); }

// complex multiplication
vec  operator &  (const vec a, const vec b) { return vec(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x); }

vec vmin(vec a, vec b) { return vec(min(a.x, b.x),  min(a.y, b.y)); }
vec vmax(vec a, vec b) { return vec(max(a.x, b.x),  max(a.y, b.y)); }

// rectangles
//============

struct rec {
  vec c1, c2;
  rec(const vec& _1, const vec& _2) : c1(_1), c2(_2) { }
  rec(const vec& _1, double s) : c1(_1-vec(s,s)), c2(_1+vec(s,s)) { }
  rec operator | (const rec& r2) { return rec(vmin(c1,r2.c1), vmax(c2, r2.c2)); }
  rec operator & (const rec& r2) { return rec(vmax(c1,r2.c1), vmin(c2, r2.c2)); }
  };

vec inrect(rec r, vec v) {
  return r.c1 + (r.c2-r.c1) * v;
  }

static const rec emptyrec = rec(vec(1e100,1e100), vec(-1e100,-1e100));

bool inrec(const vec& v, const rec &r) {
  return v.x >= r.c1.x && v.y >= r.c1.y && v.x <= r.c2.x && v.y <= r.c2.y;
  }

// transformations
//=================

struct xform {
  ld mxx, mxy, myx, myy, tx, ty;
  xform(ld _mxx, ld _mxy, ld _myx, ld _myy, ld _tx, ld _ty) : 
    mxx(_mxx), mxy(_mxy), myx(_myx), myy(_myy), tx(_tx), ty(_ty) {}
  xform(cld a, cld b) {
    mxx = real(a);
    myy = real(a);
    myx = imag(a);
    mxy = -imag(a);
    tx = real(b);
    ty = imag(b);
    }
  };

ld det(const xform& x) { return x.mxx*x.myy - x.mxy * x.myx; }

ld scalefactor(const xform& x) { return sqrt((float) abs(det(x))); }

xform xshift(const vec& _s) { return xform(1,0,0,1, _s.x, _s.y); }

xform xscale(ld m) { return xform(m,0,0,m, 0,0); }

xform xscale(const vec& _m) { return xform(_m.x,0,0,_m.y, 0,0); }

xform xspin(ld alpha) { 
  ld s = sin(alpha), c = cos(alpha);
  return xform(c, s, -s, c, 0, 0); 
  }

void vec::tform(const xform& a) {
  (*this) = vec(a.tx + a.mxx * x + a.myx * y, a.ty + a.mxy * x + a.myy * y);
  }

xform operator * (const xform &a, const xform &b) {

  return xform(
    a.mxx*b.mxx+a.myx*b.mxy,
    a.mxy*b.mxx+a.myy*b.mxy,
    a.mxx*b.myx+a.myx*b.myy,
    a.mxy*b.myx+a.myy*b.myy,  
    a.tx + a.mxx*b.tx + a.myx*b.ty, 
    a.ty + a.mxy*b.tx + a.myy*b.ty
    );
  }

vec operator * (const xform &a, vec b) { b.tform(a); return b; }

// vec operator * (const xform& x, const vec a) { return (a & x.amul) + x.shift; }

// styles
//========

struct style {
  int stroke, fill;
  ld width;
  string extra;
  style(int st, int fi, ld wi) : stroke(st), fill(fi), width(wi) {}
  };

char* stylestr(style &s) {
  static char buf[600];
  sprintf(buf, "style=\"stroke:#%06x;stroke-opacity:%.3Lf;stroke-width:%Lfpx;fill:#%06x;fill-opacity:%.3Lf%s\"",
    s.stroke & 0xFFFFFF, cta(s.stroke),
    s.width,
    s.fill & 0xFFFFFF, cta(s.fill),
    s.extra.c_str()
    );
  return buf;
  }

// pictures
//==========

struct picture;
typedef std::shared_ptr<picture> pic;

template<class T> std::shared_ptr<T> uclone(const T* x) { return std::make_shared<T> (*x); }

typedef std::function<void(style&)> stylefun;

struct picture {
  virtual void drawSvg(FILE *f) { printf("not implemented\n"); }
  virtual void tform(const xform& x) { printf("not implemented\n"); }
  virtual pic clone() const { return uclone<picture>(this); }
  virtual ~picture() {}
  virtual void onstyle(stylefun s) {}
  virtual rec bbox() const { return emptyrec; }
  };

struct line : picture {
  style b;
  vec v1, v2;
  line(style _b, vec _v1, vec _v2) : b(_b), v1(_v1), v2(_v2) { }
  void drawSvg(FILE *f) { 
    fprintf(f, "<line x1='%Lf' y1='%Lf' x2='%Lf' y2='%Lf' %s/>\n",
      v1.x, v1.y, v2.x, v2.y, stylestr(b)
      );
    }
  virtual void onstyle(stylefun s) { s(b); }
  virtual void tform(const xform& x) { v1.tform(x); v2.tform(x); }
  virtual pic clone() const { return uclone<line>(this); }

  virtual rec bbox() const { return rec(v1, b.width/2) | rec(v2, b.width/2); }
  };

static const vec center = vec(.5, .5);
static const vec rcenter = vec(1, .5);
static const vec lcenter = vec(0, .5);
static const vec tcenter = vec(.5, 0);
static const vec bcenter = vec(.5, 1);
static const vec topleft = vec(0, 0);
static const vec topright = vec(1, 0);
static const vec botleft = vec(0, 1);
static const vec botright = vec(1, 1);

struct fontdata {
  string svgname;
  string filename;
  };
typedef shared_ptr<fontdata> font; 
font makefont() { return make_shared<fontdata> (); }
font makefont(string fn, string sn) { auto ret = makefont(); ret->svgname = sn; ret->filename = fn; return ret;}

struct text : picture {
  style b;
  font ff;
  vec v;
  vec anchor;
  string onclick;
  ld size;
  string txt;
  text(style _b, font _f, vec _v, vec _a, ld _s, const string& _txt) : 
    b(_b), ff(_f), v(_v), anchor(_a), size(_s), txt(_txt) { }
  void drawSvg(FILE *f) { 
    fprintf(f, "<text ");
    if(onclick != "") fprintf(f, "%s ", onclick.c_str());
    fprintf(f, "x='%Lf' y='%Lf' font-size='%Lf' ", v.x, v.y + size*(1-anchor.y), size);
    style b2 = b;
    if(ff->svgname != "" && ff->svgname != "latex") 
      b2.extra += ff->svgname;
    if(anchor.x == 0) fprintf(f, "text-anchor='start'");
    if(anchor.x == .5) fprintf(f, "text-anchor='middle'");
    if(anchor.x == 1) fprintf(f, "text-anchor='end'");
    fprintf(f, " %s>", stylestr(b2));
    if(ff->svgname == "latex")
      fprintf(f, "\\myfont{%lf}{%s}", double(size), txt.c_str());
    else
      fprintf(f, "%s", txt.c_str());
    fprintf(f, "</text>\n");
    }
  virtual void onstyle(stylefun s) { s(b); }
  virtual void tform(const xform& x) { v.tform(x); size *= scalefactor(x); }
  virtual pic clone() const { return uclone<text>(this); }
  };

struct path : picture {
  style b;
  vector<vec> lst;
  bool cycled;
  string onclick;
  path(style _b) : b(_b)  { }
  path(style _b, const vector<vec>& _l, bool _cycled = false) : b(_b), lst(_l), cycled(_cycled) { }
  void add(vec v) { lst.push_back(v); }
  void drawSvg(FILE *f) { 
    if(Size(lst) < 2) return;
    for(int i=0; i<Size(lst); i++) {
      if(i == 0) {
        fprintf(f, "<path ");
        if(onclick != "") fprintf(f, "%s ", onclick.c_str());
        fprintf(f, " d=\"M ");
        }
      else
        fprintf(f, " L ");
      fprintf(f, "%Lf %Lf", lst[i].x, lst[i].y);
      }
    
    if(cycled) fprintf(f, " Z");
    
    fprintf(f, "\" %s/>\n", stylestr(b));
    }
  virtual void onstyle(stylefun s) { s(b); }
  void tform(const xform& x) { 
    for(int i=0; i<Size(lst); i++) lst[i].tform(x);
    }
  virtual pic clone() const { return uclone<path>(this); }
  virtual rec bbox() const { 
    rec res = emptyrec;
    for(vec v: lst) res = res | rec(v, b.width/2);
    return res;
    }
  };

path rectopath(rec r, style s) {
  path p(s);
  p.add(inrect(r, vec(0,0)));
  p.add(inrect(r, vec(1,0)));
  p.add(inrect(r, vec(1,1)));
  p.add(inrect(r, vec(0,1)));
  p.cycled = true;
  return p;
  }

struct circle : picture {
  style b;
  vec center;
  ld radius;
  circle(style _b, vec _c, ld _r) : b(_b), center(_c), radius(_r)  { }
  void drawSvg(FILE *f) { 
    fprintf(f, "<circle cx='%Lf' cy='%Lf' r='%Lf' %s/>\n",
      center.x, center.y, radius, stylestr(b));
    }
  virtual pic clone() const { return uclone<circle>(this); }
  virtual void tform(const xform& x) { 
    center.tform(x); radius *= scalefactor(x); 
    }
  virtual void onstyle(stylefun s) { s(b); }
  virtual rec bbox() const { return rec(center, radius + b.width/2); }
  };

struct picturegroup : picture {
  vector<pic> v;
  void addPtr(pic p) { v.push_back(p); }
  void add(const picture& p) { v.push_back(p.clone()); }
  void add(pic p) { v.push_back(p->clone()); }
  pic clone() const {
    auto g = std::make_shared<picturegroup> ();
    for(int i=0; i<Size(v); i++) g->v.push_back(v[i]->clone());
    return g;
    }
  void drawSvg(FILE *f) {
    for(int i=0; i<Size(v); i++) v[i]->drawSvg(f);
    }
  virtual void tform(const xform& x) { 
    for(int i=0; i<Size(v); i++) v[i]->tform(x);
    }
  virtual void onstyle(stylefun s) { 
    for(int i=0; i<Size(v); i++) v[i]->onstyle(s);
    }
  virtual rec bbox() const { 
    rec res = emptyrec;
    for(pic p: v) res = res | p->bbox();
    return res;
    }
  };

struct svg {
  FILE *f;
  svg(string fname, ld width, ld height) {
    f = fopen(fname.c_str(), "wt");
    if(!f) throw 1;
    fprintf(f, "<svg width=\"%Lf\" height=\"%Lf\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns=\"http://www.w3.org/2000/svg\">\n", width, height);
    }
  
  ~svg() { 
    fprintf(f, "</svg>\n");
    fclose(f); 
    }
  
  svg& operator += (pic p) { p->drawSvg(f); return *this; }
  svg& operator += (picture& p) { p.drawSvg(f); return *this; }
  };

pic& operator += (pic& addto, pic addwhat) {
  if(addwhat == NULL) ;
  else if(addto == NULL) { 
    addto = addwhat->clone(); 
    }
  else {
    auto d = dynamic_pointer_cast<picturegroup> (addto);
    if(d) {
      d->add(addwhat);
      }
    else { 
      auto v = std::make_shared<picturegroup> (); 
      v->add(addto);
      v->add(addwhat);
      addto = v;
      }
    }
  return addto;
  }

pic& operator += (pic& addto, const picture& addwhat) {
  return (addto += addwhat.clone());
  }

pic operator * (const xform& xf, pic what) {
  pic w1 = what->clone(); w1->tform(xf);
  return w1;
  }

pic operator * (const xform& xf, const picture& what) {
  pic w1 = what.clone(); w1->tform(xf);
  return w1;
  }

void drawSvg(string fname, int width, int height, pic p) {
  svg s(fname, width, height);
  s += p;
  }

struct normalizer {
  pic p;
  normalizer& operator += (pic p1) { p += p1; return *this; }
  normalizer& operator += (const picture &p1) { p += p1; return *this; }

  virtual ~normalizer() {}
  };

struct svg_normalizer : normalizer {
  string fname;
  svg_normalizer(const string& s) : fname(s) {}
  ~svg_normalizer() {  
    if(!p) return;
    rec r = p->bbox();
    r.c1.x -= 2;
    r.c1.y -= 2;
    r.c2.x += 2;
    r.c2.y += 2;
    p->tform(xshift(-r.c1));
      
    svg s(fname, r.c2.x-r.c1.x, r.c2.y-r.c1.y);
    s += p;
    }
  };

void addcrosses(const line& l1, const line& l2, vector<vec>& v) {
  auto w1 = l1.v2 - l1.v1;
  auto w2 = l2.v2 - l2.v1;
  auto z1 = l1.v1 ^ l1.v2;
  auto z2 = l2.v1 ^ l2.v2;
  double dd = w1 ^ w2;
  if(!dd) return;
  v.push_back(vec(z2*w1.x-z1*w2.x, z2*w1.y-z1*w2.y) / dd);
  }

xform be01(line& l) {
  return xform(cld(1) / asconj(l.v2 - l.v1), cld(0)) * xshift(-l.v1);
  }

void addcrosses(const line &l, const circle &c, vector<vec>& vres) {
  cld v1 = asconj(l.v1), v2 = asconj(l.v2), v = asconj(c.center);
  ld r = c.radius;
  v2 -= v1; v -= v1;
  v /= v2; r /= abs(v2);
  if(abs(r) >= abs(imag(v))) {
    cld ve1(real(v) + sqrt(r*r - imag(v) * imag(v)));
    vres.push_back(ve1 * v2 + v1);
    cld ve2(real(v) - sqrt(r*r - imag(v) * imag(v)));
    vres.push_back(ve2 * v2 + v1);
    }
  }

void addcrosses(const circle& c1, const circle &c2, vector<vec>& vres) {
  cld cc1 = asconj(c1.center), cc2 = asconj(c2.center);
  cc2 -= cc1;
  ld d = abs(cc2);
  ld r1 = abs(c1.radius / d);
  ld r2 = abs(c2.radius / d);
  
  // O(0,r1) and O(1,r2)
  
  if(r1+r2 < 1) return;
  if(abs(r2-r1) > 1) return;
  
  ld x = (r1*r1 - r2*r2 + 1)/2;
  ld y = sqrt(r1*r1 - x*x);
  vres.push_back(cld(x,y) * cc2 + cc1);
  vres.push_back(cld(x,-y) * cc2 + cc1);  
  }

void addcrosses(pic p1, pic p2, vector<vec>& v) {
  auto g1 = dynamic_pointer_cast<picturegroup> (p1);
  if(g1) { for(auto p: g1->v) addcrosses(p, p2, v); return; }
  auto g2 = dynamic_pointer_cast<picturegroup> (p2);
  if(g2) { for(auto p: g2->v) addcrosses(p1, p, v); return; }
  auto l1 = dynamic_pointer_cast<line> (p1);
  auto c1 = dynamic_pointer_cast<circle> (p1);
  auto l2 = dynamic_pointer_cast<line> (p2);
  auto c2 = dynamic_pointer_cast<circle> (p2);
  if(l1 && l2) addcrosses(*l1, *l2, v);
  else if(l1 && c2) addcrosses(*l1, *c2, v);
  else if(c1 && l2) addcrosses(*l2, *c1, v);
  else if(c1 && c2) addcrosses(*c1, *c2, v);
  }
  
template<class T, class U> vector<vec> crosses(const T& p1, const U& p2) {
  vector<vec> result;
  addcrosses(p1, p2, result);
  return result;
  }

#endif


#include "graph2.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

int digits;

inline string its(int i) {
  char buf[16];
  sprintf(buf, "%*d", digits, i);
  return buf;
  }

inline string sts(string s0) {
  char buf[16];
  sprintf(buf, "%*s", digits, s0.c_str());
  return buf;
  }

string s0 = "";

inline string maymark(string s, bool marked) {
  if(!marked) return s;
  return "<font color='red'><b>" + s + "</b></font>";
  }

#ifdef EMSCRIPTEN
void set_value(string name, string s) {
  EM_ASM_({
    var name = UTF8ToString($0, $1);
    var value = UTF8ToString($2, $3);
    document.getElementById(name).innerHTML = value;
    }, name.c_str(), int(name.size()),
    s.c_str(), int(s.size())
    );
  }
#else
void set_value(string name, string s) {
  printf("output on %s:\n%s\n", name.c_str(), s.c_str());
  }
#endif

#ifdef EMSCRIPTEN
void set_edit_value(string name, string s) {
  EM_ASM_({
    var name = UTF8ToString($0, $1);
    var value = UTF8ToString($2, $3);
    document.getElementById(name).value = value;
    }, name.c_str(), int(name.size()),
    s.c_str(), int(s.size())
    );
  }
#else
void set_edit_value(string name, string s) {
  printf("value on %s:\n%s\n", name.c_str(), s.c_str());
  }
#endif

void add_button(ostream& of, string action, string text) {
  of << "<button type='button' onclick='" << action << "'/>" << text << "</button>";
  }

inline void outputHTML(string s) {
#ifdef EMSCRIPTEN
  set_value("result", s);
#else
  ofstream of("output.html");
  of << s;
  printf("Output written to 'output.html'\n");
#endif
  }

inline string SVG_to_string(pic& p) {
  {svg_normalizer s("output.svg"); s += p;}

  string output = "&nbsp;";

  FILE *hello = fopen("output.svg", "rt");
  int c;
  while((c = fgetc(hello)) >= 0) output += c;
  fclose(hello);    

  return output;
  }

inline void outputSVG(pic& p) {
#ifdef EMSCRIPTEN
  outputHTML(SVG_to_string(p));
#else
  {svg_normalizer s("output.svg"); s += p;}
  printf("Output written to 'output.svg'\n");
#endif
  }

all: astar.js

astar.js: astar.cpp mapgen.cpp visutils.h graph2.h
	em++ -O2 -std=c++11 astar.cpp -o astar.js \
          -s EXPORTED_FUNCTIONS="['_start', '_view_help', '_back_to_game', '_view_new_game', '_restart', '_guess', '_moveon']" \
          -s EXTRA_EXPORTED_RUNTIME_METHODS='["FS","ccall"]' -sALLOW_MEMORY_GROWTH \
          -sFETCH

test: astar.js
	cp index.html ~/public_html/astar
	cp astar.wasm ~/public_html/astar
	cp astar.js ~/public_html/astar

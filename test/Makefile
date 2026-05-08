all:
	swig -c++ -lua -I.. ../sqlitelib.i
	g++ -std=c++14 -fPIC -shared \
	    $(shell pkg-config --cflags --libs lua sqlite3) \
	    -I.. ../sqlitelib_wrap.cxx -o sqlitelib.so

clean:
	rm -f sqlitelib_wrap.cxx sqlitelib.so

.PHONY: all clean

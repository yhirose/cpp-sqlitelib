
CC = clang++
CCFLAGS = -std=c++1y -stdlib=libc++ -g -DGTEST_USE_OWN_TR1_TUPLE

all : test
	./test

test : test.cc ../sqlitelib.h
	$(CC) -o test $(CCFLAGS) -I.. -I../sqlite3  -I. test.cc gtest/gtest-all.cc gtest/gtest_main.cc -lsqlite3

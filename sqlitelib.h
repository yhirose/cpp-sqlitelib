//
//  sqlitelib.h
//
//  Copyright (c) 2013 Yuji Hirose. All rights reserved.
//  The Boost Software License 1.0
//

#ifndef _CPPSQLITELIB_HTTPSLIB_H_
#define _CPPSQLITELIB_HTTPSLIB_H_

#include <sqlite3.h>
#include <cstdlib>
#include <cassert>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

namespace sqlitelib {

typedef int Int;
typedef double Float;
typedef std::string Text;
typedef std::vector<char> Blob;

class Statement
{
public:
	Statement(sqlite3* db, const char* query)
		: stmt(nullptr)
	{
		verify(sqlite3_prepare(db, query, strlen(query), &stmt, nullptr));
	}

	Statement(Statement&& rhs)
		: stmt(rhs.stmt)
	{
		rhs.stmt = nullptr;
	}

	~Statement()
	{
	    verify(sqlite3_finalize(stmt));
	}

	template <typename T1>
	Statement& bind(T1 val1)
	{
		verify(sqlite3_reset(stmt));
		bind_value(val1, 1);
		return *this;
	}

	template <typename T1, typename T2>
	Statement& bind(T1 val1, T2 val2)
	{
		verify(sqlite3_reset(stmt));
		bind_value(val1, 1);
		bind_value(val2, 2);
		return *this;
	}

	template <typename T>
	T execute_value()
	{
		T ret;

		enumrate_rows([&]() {
			ret = get_value<T>(0);
			return true;
		});

		return ret;
	}

	template <typename T1>
	std::vector<T1> execute()
	{
		std::vector<T1> ret;

		enumrate_rows([&]() {
			ret.push_back(get_value<T1>(0));
			return false;
		});

		return ret;
	}

	template <typename T1, typename T2>
	std::vector<std::tuple<T1, T2>> execute()
	{
		std::vector<std::tuple<T1, T2>> ret;

		enumrate_rows([&]() {
			ret.push_back(std::make_tuple(
				get_value<T1>(0),
				get_value<T2>(1)));
			return false;
		});

		return ret;
	}

private:
	Statement(const Statement& rhs);
	Statement& operator=(const Statement& rhs);

	sqlite3_stmt* stmt;

	template <typename T>
	T get_value(int col) {}

	template <typename T>
	void bind_value(T val, int col)	{}

	template <typename T>
	void enumrate_rows(T func)
	{
		int rc;
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
			if (func()) {
				rc = SQLITE_DONE;
				break;
			}
		}
		verify(rc, SQLITE_DONE);
	}

	void verify(int rc, int expected = SQLITE_OK) const
	{
		if (rc != expected) {
			throw;
		}
	}
};

class Sqlite
{
public:
	Sqlite(const char* path)
	   : db_(nullptr)
	{
		auto rc = sqlite3_open(path, &db_);
		if (rc) {
			sqlite3_close(db_);
			db_ = nullptr;
		}
	}

	~Sqlite()
	{
		if (db_) {
			sqlite3_close(db_);
		}
	}

	bool is_open() const
	{
		return db_ != nullptr;
	}

	Statement prepare(const char* query) const
	{
		return Statement(db_, query);
	}

private:
    Sqlite();
    Sqlite(const Sqlite&);
    Sqlite& operator=(const Sqlite&);

    sqlite3* db_;
};

template <>
int Statement::get_value<int>(int col)
{
    return sqlite3_column_int(stmt, col);
}

template <>
double Statement::get_value<double>(int col)
{
    return sqlite3_column_double(stmt, col);
}

template <>
std::string Statement::get_value<std::string>(int col)
{
    auto val = std::string(sqlite3_column_bytes(stmt, col), 0);
    memcpy(&val[0], sqlite3_column_text(stmt, col), val.size());
    return val;
}

template <>
std::vector<char> Statement::get_value<std::vector<char>>(int col)
{
    auto val = std::vector<char>(sqlite3_column_bytes(stmt, col));
    memcpy(&val[0], sqlite3_column_blob(stmt, col), val.size());
    return val;
}

template <>
void Statement::bind_value<int>(int val, int col)
{
    verify(sqlite3_bind_int(stmt, col, val));
}

template <>
void Statement::bind_value<double>(double val, int col)
{
    verify(sqlite3_bind_double(stmt, col, val));
}

template <>
void Statement::bind_value<std::string>(std::string val, int col)
{
    verify(sqlite3_bind_text(stmt, col, val.data(), val.size(), SQLITE_TRANSIENT));
}

template <>
void Statement::bind_value<const char*>(const char* val, int col)
{
    verify(sqlite3_bind_text(stmt, col, val, strlen(val), SQLITE_TRANSIENT));
}

}

#endif

// vim: et ts=4 sw=4 cin cino={1s ff=unix

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

class Value
{
public:
	Value();
	Value(int val);
	Value(double val);
	Value(const std::string& val);
	Value(sqlite3_stmt* stmt, int col);

	int type;

	// TODO: use variant type to reduce space
	int               integer;
	double            floating_point;
	std::string       text;
	std::vector<char> blob;

private:
	void init();
	static std::unordered_map<std::string, int> types_;
};

class Sqlite
{
public:
    Sqlite(const char* path);
    Sqlite(const std::string& path);

    //Sqlite() = delete;
    //Sqlite(const Sqlite&) = delete;
    //Sqlite& operator=(const Sqlite&) = delete;

    ~Sqlite();

    bool is_open() const;

	// Varient type version
	Value execute_value(const char* query);

	std::vector<std::vector<Value>> execute(const char* query);

	// Static type version
	template <typename T>
	T execute_value(const char* query);

	template <typename T1>
	std::vector<T1> execute(const char* query);

	template <typename T1, typename T2>
	std::vector<std::tuple<T1, T2>> execute(const char* query);

	template <typename T1, typename T2, typename T3>
	std::vector<std::tuple<T1, T2, T3>> execute(const char* query);

	template <typename T1, typename T2, typename T3, typename T4>
	std::vector<std::tuple<T1, T2, T3, T4>> execute(const char* query);

    // Bind values to place holders
	template <typename T1>
	Sqlite& bind(T1 val1);

	template <typename T1, typename T2>
	Sqlite& bind(T1 val1, T2 val2);

	template <typename T1, typename T2, typename T3>
	Sqlite& bind(T1 val1, T2 val2, T3 val3);

	template <typename T1, typename T2, typename T3, typename T4>
	Sqlite& bind(T1 val1, T2 val2, T3 val3, T4 val4);

private:
    sqlite3*           db_;
	std::vector<Value> bound_values_;

	template <typename T>
	void enumrate_rows(const char* query, T func);

	void bind_values(sqlite3_stmt* stmt);

    void verify(int rc, int expected = SQLITE_OK) const;
};

// Helper
namespace {

template <typename T>
T get_value(sqlite3_stmt* stmt, int col) {}

template <>
int get_value<int>(sqlite3_stmt* stmt, int col)
{
	return sqlite3_column_int(stmt, col);
}

template <>
double get_value<double>(sqlite3_stmt* stmt, int col)
{
	return sqlite3_column_double(stmt, col);
}

template <>
std::string get_value<std::string>(sqlite3_stmt* stmt, int col)
{
	auto val = std::string(sqlite3_column_bytes(stmt, col), 0);
	memcpy(&val[0], sqlite3_column_text(stmt, col), val.size());
	return val;
}

template <>
std::vector<char> get_value<std::vector<char>>(sqlite3_stmt* stmt, int col)
{
    auto val = std::vector<char>(sqlite3_column_bytes(stmt, col));
    memcpy(&val[0], sqlite3_column_blob(stmt, col), val.size());
    return val;
}

}

// Implementation
std::unordered_map<std::string, int> Value::types_;

inline Value::Value() : type(SQLITE_NULL)
{
	init();
}

inline Value::Value(int val)
	: type(SQLITE_INTEGER)
	, integer(val)
{
	init();
}

inline Value::Value(double val)
	: type(SQLITE_FLOAT)
	, floating_point(val)
{
	init();
}

inline Value::Value(const std::string& val)
	: type(SQLITE_TEXT)
	, text(val)
{
	init();
}

inline Value::Value(sqlite3_stmt* stmt, int col)
	: type(SQLITE_NULL)
{
	init();

	auto decltype_text = sqlite3_column_decltype(stmt, col);
	type = types_[decltype_text];

	switch (type) {
		case SQLITE_INTEGER:
			integer = sqlite3_column_int(stmt, col);
			break;
		case SQLITE_FLOAT:
			floating_point = sqlite3_column_double(stmt, col);
			break;
		case SQLITE_TEXT: {
			text.assign(
				(char*)sqlite3_column_text(stmt, col),
				sqlite3_column_bytes(stmt, col));
			break;
		}
		case SQLITE_BLOB: {
			auto p = (char*)sqlite3_column_blob(stmt, col);
			blob.assign(p, p + sqlite3_column_bytes(stmt, col));
			break;
		}
	}
}

inline void Value::init()
{
	if (types_.empty()) {
		types_["BLOB"] = SQLITE_BLOB;
		types_["INTEGER"] = SQLITE_INTEGER;
		types_["FLOAT"] = SQLITE_FLOAT;
		types_["TEXT"] = SQLITE_TEXT;
	}
}

inline Sqlite::Sqlite(const char* path)
   : db_(nullptr)
{
    auto rc = sqlite3_open(path, &db_);
    if (rc) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

inline Sqlite::~Sqlite()
{
    if (db_) {
        sqlite3_close(db_);
    }
}

inline bool Sqlite::is_open() const
{
    return db_ != nullptr;
}

template <typename T1>
inline Sqlite& Sqlite::bind(T1 val1)
{
	bound_values_.push_back(Value(val1));
	return *this;
}

template <typename T1, typename T2>
inline Sqlite& Sqlite::bind(T1 val1, T2 val2)
{
	bound_values_.push_back(Value(val1));
	bound_values_.push_back(Value(val2));
	return *this;
}

template <typename T1, typename T2, typename T3>
inline Sqlite& Sqlite::bind(T1 val1, T2 val2, T3 val3)
{
	bound_values_.push_back(Value(val1));
	bound_values_.push_back(Value(val2));
	bound_values_.push_back(Value(val3));
	return *this;
}

template <typename T1, typename T2, typename T3, typename T4>
inline Sqlite& Sqlite::bind(T1 val1, T2 val2, T3 val3, T4 val4)
{
	bound_values_.push_back(Value(val1));
	bound_values_.push_back(Value(val2));
	bound_values_.push_back(Value(val3));
	bound_values_.push_back(Value(val4));
	return *this;
}

inline Value Sqlite::execute_value(const char* query)
{
	Value ret;

	enumrate_rows(query, [&](sqlite3_stmt* stmt) {
		ret = Value(stmt, 0);
		return true;
	});

    return ret;
}

inline std::vector<std::vector<Value>> Sqlite::execute(const char* query)
{
	std::vector<std::vector<Value>> ret;

	enumrate_rows(query, [&](sqlite3_stmt* stmt) {
		ret.push_back(std::vector<Value>());
		auto& last = ret[ret.size() - 1];

		auto count = sqlite3_column_count(stmt);
		for (int i = 0; i < count; i++) {
			last.push_back(Value(stmt, i));
		}

		return false;
	});

    return ret;
}

template <typename T>
inline T Sqlite::execute_value(const char* query)
{
	T ret;

	enumrate_rows(query, [&](sqlite3_stmt* stmt) {
		ret = get_value<T>(stmt, 0);
		return true;
	});

    return ret;
}

template <typename T1>
inline std::vector<T1> Sqlite::execute(const char* query)
{
	std::vector<T1> ret;

	enumrate_rows(query, [&](sqlite3_stmt* stmt) {
		ret.push_back(get_value<T1>(stmt, 0));
		return false;
	});

    return ret;
}

template <typename T1, typename T2>
inline std::vector<std::tuple<T1, T2>> Sqlite::execute(const char* query)
{
	std::vector<std::tuple<T1, T2>> ret;

	enumrate_rows(query, [&](sqlite3_stmt* stmt) {
		ret.push_back(std::make_tuple(
			get_value<T1>(stmt, 0),
			get_value<T2>(stmt, 1)));
		return false;
	});

    return ret;
}

template <typename T1, typename T2, typename T3>
inline std::vector<std::tuple<T1, T2, T3>> Sqlite::execute(const char* query)
{
	std::vector<std::tuple<T1, T2, T3>> ret;

	enumrate_rows(query, [&](sqlite3_stmt* stmt) {
		ret.push_back(std::make_tuple(
			get_value<T1>(stmt, 0),
			get_value<T2>(stmt, 1),
			get_value<T2>(stmt, 2)));
		return false;
	});

    return ret;
}

template <typename T1, typename T2, typename T3, typename T4>
inline std::vector<std::tuple<T1, T2, T3, T4>> Sqlite::execute(const char* query)
{
	std::vector<std::tuple<T1, T2, T3, T4>> ret;

	enumrate_rows(query, [&](sqlite3_stmt* stmt) {
		ret.push_back(std::make_tuple(
			get_value<T1>(stmt, 0),
			get_value<T2>(stmt, 1),
			get_value<T2>(stmt, 2)));
		return false;
	});

    return ret;
}

template <typename T>
inline void Sqlite::enumrate_rows(const char* query, T func)
{
    assert(is_open());

    sqlite3_stmt* stmt = nullptr;
    verify(sqlite3_prepare(db_, query, strlen(query), &stmt, nullptr));

    verify(sqlite3_reset(stmt));
    bind_values(stmt);

	int rc;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		if (func(stmt)) {
			rc = SQLITE_DONE;
			break;
		}
	}
	verify(rc, SQLITE_DONE);

    verify(sqlite3_finalize(stmt));

	bound_values_.clear();
}

inline void Sqlite::bind_values(sqlite3_stmt* stmt)
{
	for (size_t i = 0; i < bound_values_.size(); i++) {
		const auto& val = bound_values_[i];
        auto id = i + 1;

		switch (val.type) {
		case SQLITE_INTEGER:
			verify(sqlite3_bind_int(stmt, id, val.integer));
			break;
		case SQLITE_FLOAT:
			verify(sqlite3_bind_double(stmt, id, val.floating_point));
			break;
		case SQLITE_TEXT:
			verify(sqlite3_bind_text(stmt, id, val.text.data(), val.text.size(), SQLITE_TRANSIENT));
			break;
        default:
            throw;
		}
	}
}

inline void Sqlite::verify(int rc, int expected) const
{
    if (rc != expected) {
        throw;
    }
}

}

#endif

// vim: et ts=4 sw=4 cin cino={1s ff=unix

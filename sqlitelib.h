//
//  sqlitelib.h
//
//  Copyright (c) 2013 Yuji Hirose. All rights reserved.
//  The Boost Software License 1.0
//

#ifndef _CPPSQLITELIB_HTTPSLIB_H_
#define _CPPSQLITELIB_HTTPSLIB_H_

#include <sqlite3.h>
#include <vector>
#include <tuple>
#include <type_traits>

namespace sqlitelib {

typedef int Int;
typedef double Float;
typedef std::string Text;
typedef std::vector<char> Blob;

namespace {

extern void* enabler;

inline void verify(int rc, int expected = SQLITE_OK)
{
    if (rc != expected) {
        throw;
    }
}

template <typename T>
T get_column_value(sqlite3_stmt* stmt, int col) {}

template <>
int get_column_value<int>(sqlite3_stmt* stmt, int col)
{
    return sqlite3_column_int(stmt, col);
}

template <>
double get_column_value<double>(sqlite3_stmt* stmt, int col)
{
    return sqlite3_column_double(stmt, col);
}

template <>
std::string get_column_value<std::string>(sqlite3_stmt* stmt, int col)
{
    auto val = std::string(sqlite3_column_bytes(stmt, col), 0);
    memcpy(&val[0], sqlite3_column_text(stmt, col), val.size());
    return val;
}

template <>
std::vector<char> get_column_value<std::vector<char>>(sqlite3_stmt* stmt, int col)
{
    auto val = std::vector<char>(sqlite3_column_bytes(stmt, col));
    memcpy(&val[0], sqlite3_column_blob(stmt, col), val.size());
    return val;
}

template <int N, typename T, typename... Rest>
struct ColumnValues;

template <int N, typename T, typename... Rest>
struct ColumnValues {
    static std::tuple<T, Rest...> get(sqlite3_stmt* stmt, int col)
    {
        return std::tuple_cat(
            std::make_tuple(get_column_value<T>(stmt, col)),
            ColumnValues<N - 1, Rest...>::get(stmt, col + 1));
    }
};

template <typename T>
struct ColumnValues<1, T> {
    static std::tuple<T> get(sqlite3_stmt* stmt, int col)
    {
        return std::make_tuple(get_column_value<T>(stmt, col));
    }
};

template <typename Arg>
void bind_value(sqlite3_stmt* stmt, int col, Arg val) {}

template <>
void bind_value<int>(sqlite3_stmt* stmt, int col, int val)
{
    verify(sqlite3_bind_int(stmt, col, val));
}

template <>
void bind_value<double>(sqlite3_stmt* stmt, int col, double val)
{
    verify(sqlite3_bind_double(stmt, col, val));
}

template <>
void bind_value<std::string>(sqlite3_stmt* stmt, int col, std::string val)
{
    verify(sqlite3_bind_text(stmt, col, val.data(), val.size(), SQLITE_TRANSIENT));
}

template <>
void bind_value<const char*>(sqlite3_stmt* stmt, int col, const char* val)
{
    verify(sqlite3_bind_text(stmt, col, val, strlen(val), SQLITE_TRANSIENT));
}

};

template <typename T, typename... Rest>
class Statement
{
public:
    Statement(sqlite3* db, const char* query)
        : stmt_(nullptr)
    {
        verify(sqlite3_prepare(db, query, strlen(query), &stmt_, nullptr));
    }

    Statement(Statement&& rhs)
        : stmt_(rhs.stmt_)
    {
        rhs.stmt_ = nullptr;
    }

    ~Statement()
    {
        verify(sqlite3_finalize(stmt_));
    }

    template <typename... Args>
    T execute_value(const Args&... args)
    {
        bind(args...);
        T ret;
        enumrate_rows([&]() {
            ret = get_column_value<T>(stmt_, 0);
            return true;
        });
        return ret;
    }

    template <
        int RestSize = sizeof...(Rest),
        typename std::enable_if<(RestSize == 0)>::type*& = enabler,
        typename... Args>
    std::vector<T> execute(const Args&... args)
    {
        bind(args...);
        std::vector<T> ret;
        enumrate_rows([&]() {
            ret.push_back(get_column_value<T>(stmt_, 0));
            return false;
        });
        return ret;
    }

    template <
        int RestSize = sizeof...(Rest),
        typename std::enable_if<(RestSize != 0)>::type*& = enabler,
        typename... Args>
    std::vector<std::tuple<T, Rest...>> execute(const Args&... args)
    {
        bind(args...);
        std::vector<std::tuple<T, Rest...>> ret;
        enumrate_rows([&]() {
            ret.push_back(ColumnValues<1 + sizeof...(Rest), T, Rest...>::get(stmt_, 0));
            return false;
        });
        return ret;
    }

private:
    Statement(const Statement& rhs);
    Statement& operator=(const Statement& rhs);

    sqlite3_stmt* stmt_;

    inline void bind_values(int col) {}

    template <typename Arg, typename... ArgRest>
    void bind_values(int col, const Arg& val, const ArgRest&... rest)
    {
        bind_value(stmt_, col, val);
        bind_values(col + 1, rest...);
    }

    template <typename... Args>
    void bind(const Args&... args)
    {
        verify(sqlite3_reset(stmt_));
        bind_values(1, args...);
    }

    template <typename Func>
    void enumrate_rows(Func func)
    {
        int rc;
        while ((rc = sqlite3_step(stmt_)) == SQLITE_ROW) {
            if (func()) {
                rc = SQLITE_DONE;
                break;
            }
        }
        verify(rc, SQLITE_DONE);
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

    template <typename... Types>
    Statement<Types...> prepare(const char* query) const
    {
        return Statement<Types...>(db_, query);
    }

private:
    Sqlite();
    Sqlite(const Sqlite&);
    Sqlite& operator=(const Sqlite&);

    sqlite3* db_;
};

}

#endif

// vim: et ts=4 sw=4 cin cino={1s ff=unix

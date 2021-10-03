//
//  sqlitelib.h
//
//  Copyright (c) 2013 Yuji Hirose. All rights reserved.
//  The Boost Software License 1.0
//

#ifndef _CPPSQLITELIB_HTTPSLIB_H_
#define _CPPSQLITELIB_HTTPSLIB_H_

#include <sqlite3.h>

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace sqlitelib {

namespace {

void* enabler;

inline void verify(int rc, int expected = SQLITE_OK) {
  if (rc != expected) {
    throw std::exception();
  }
}

template <typename T>
T get_column_value(sqlite3_stmt* stmt, int col) {}

template <>
int get_column_value<int>(sqlite3_stmt* stmt, int col) {
  return sqlite3_column_int(stmt, col);
}

template <>
double get_column_value<double>(sqlite3_stmt* stmt, int col) {
  return sqlite3_column_double(stmt, col);
}

template <>
std::string get_column_value<std::string>(sqlite3_stmt* stmt, int col) {
  auto val = std::string(sqlite3_column_bytes(stmt, col), 0);
  memcpy(&val[0], sqlite3_column_text(stmt, col), val.size());
  return val;
}

template <>
std::vector<char> get_column_value<std::vector<char>>(sqlite3_stmt* stmt,
                                                      int col) {
  auto val = std::vector<char>(sqlite3_column_bytes(stmt, col));
  memcpy(&val[0], sqlite3_column_blob(stmt, col), val.size());
  return val;
}

template <int N, typename T, typename... Rest>
struct ColumnValues;

template <int N, typename T, typename... Rest>
struct ColumnValues {
  static std::tuple<T, Rest...> get(sqlite3_stmt* stmt, int col) {
    return std::tuple_cat(std::make_tuple(get_column_value<T>(stmt, col)),
                          ColumnValues<N - 1, Rest...>::get(stmt, col + 1));
  }
};

template <typename T>
struct ColumnValues<1, T> {
  static std::tuple<T> get(sqlite3_stmt* stmt, int col) {
    return std::make_tuple(get_column_value<T>(stmt, col));
  }
};

template <typename Arg>
void bind_value(sqlite3_stmt* stmt, int col, Arg val) {}

template <>
void bind_value<int>(sqlite3_stmt* stmt, int col, int val) {
  verify(sqlite3_bind_int(stmt, col, val));
}

template <>
void bind_value<double>(sqlite3_stmt* stmt, int col, double val) {
  verify(sqlite3_bind_double(stmt, col, val));
}

template <>
void bind_value<std::string>(sqlite3_stmt* stmt, int col, std::string val) {
  verify(sqlite3_bind_text(stmt, col, val.data(), static_cast<int>(val.size()),
                           SQLITE_TRANSIENT));
}

template <>
void bind_value<const char*>(sqlite3_stmt* stmt, int col, const char* val) {
  verify(sqlite3_bind_text(stmt, col, val, static_cast<int>(strlen(val)),
                           SQLITE_TRANSIENT));
}

template <>
void bind_value<std::vector<char>>(sqlite3_stmt* stmt, int col,
                                   std::vector<char> val) {
  verify(sqlite3_bind_blob(stmt, col, val.data(), static_cast<int>(val.size()),
                           SQLITE_TRANSIENT));
}

template <bool isRestEmpty, typename T, typename... Rest>
struct ValueType;

template <typename T, typename... Rest>
struct ValueType<true, T, Rest...> {
  typedef T type;
};

template <typename T, typename... Rest>
struct ValueType<false, T, Rest...> {
  typedef std::tuple<T, Rest...> type;
};

};  // namespace

template <typename T, typename... Rest>
class Iterator {
 public:
  typedef std::forward_iterator_tag iterator_category;
  typedef typename ValueType<!sizeof...(Rest), T, Rest...>::type value_type;
  typedef std::ptrdiff_t difference_type;
  typedef value_type* pointer;
  typedef value_type& reference;

  Iterator() : stmt_(nullptr), id_(-1) {}

  Iterator(sqlite3_stmt* stmt) : stmt_(stmt), id_(-1) { operator++(); }

  template <int RestSize = sizeof...(Rest),
            typename std::enable_if<(RestSize == 0)>::type*& = enabler>
  value_type operator*() const {
    return get_column_value<T>(stmt_, 0);
  }

  template <int RestSize = sizeof...(Rest),
            typename std::enable_if<(RestSize != 0)>::type*& = enabler>
  value_type operator*() const {
    return ColumnValues<1 + sizeof...(Rest), T, Rest...>::get(stmt_, 0);
  }

  Iterator& operator++() {
    if (stmt_) {
      auto rc = sqlite3_step(stmt_);
      if (rc == SQLITE_ROW) {
        ++id_;
      } else if (rc == SQLITE_DONE) {
        id_ = -1;
      } else {
        throw std::exception();  // TODO:
      }
    } else {
      throw std::exception();  // TODO:
    }
    return *this;
  }

  bool operator==(const Iterator& rhs) { return id_ == rhs.id_; }

  bool operator!=(const Iterator& rhs) { return !operator==(rhs); }

 private:
  sqlite3_stmt* stmt_;
  int id_;
};

inline void sqlite3_stmt_deleter(sqlite3_stmt* stmt) {
  verify(sqlite3_finalize(stmt));
};

template <typename T, typename... Rest>
class Cursor {
 public:
  Cursor() = delete;
  Cursor(const Cursor&) = delete;
  Cursor& operator=(const Cursor&) = delete;

  Cursor(Cursor&& rhs) : stmt_(rhs.stmt_) {}

  Cursor(std::shared_ptr<sqlite3_stmt> stmt) : stmt_(stmt) {}

  Iterator<T, Rest...> begin() { return Iterator<T, Rest...>(stmt_.get()); }

  Iterator<T, Rest...> end() { return Iterator<T, Rest...>(); }

 private:
  std::shared_ptr<sqlite3_stmt> stmt_;
};

template <typename T, typename... Rest>
class Statement {
 public:
  Statement(sqlite3* db, const char* query)
      : stmt_(new_sqlite3_stmt(db, query), sqlite3_stmt_deleter) {}

  Statement(Statement&& rhs) : stmt_(rhs.stmt_) { rhs.stmt_ = nullptr; }

  Statement() = delete;
  Statement(Statement& rhs) = default;
  ~Statement() = default;

  template <typename... Args>
  Statement<T, Rest...>& bind(const Args&... args) {
    verify(sqlite3_reset(stmt_.get()));
    bind_values(1, args...);
    return *this;
  }

  template <
      typename U = T,
      typename std::enable_if<std::is_same<U, void>::value>::type*& = enabler,
      typename... Args>
  void execute(const Args&... args) {
    bind(args...);
    verify(sqlite3_step(stmt_.get()), SQLITE_DONE);
  }

  template <
      typename U = T,
      typename std::enable_if<!std::is_same<U, void>::value>::type*& = enabler,
      typename V = typename ValueType<!sizeof...(Rest), T, Rest...>::type,
      typename... Args>
  std::vector<V> execute(const Args&... args) {
    std::vector<V> ret;
    for (const auto& x : execute_cursor(args...)) {
      ret.push_back(x);
    }
    return ret;
  }

  template <typename... Args>
  T execute_value(const Args&... args) {
    auto cursor = execute_cursor(args...);
    return *cursor.begin();
  }

  template <typename... Args>
  Cursor<T, Rest...> execute_cursor(const Args&... args) {
    bind(args...);
    return Cursor<T, Rest...>(stmt_);
  }

 private:
  Statement& operator=(const Statement& rhs);

  sqlite3_stmt* new_sqlite3_stmt(sqlite3* db, const char* query) {
    sqlite3_stmt* p = nullptr;
    verify(sqlite3_prepare(db, query, static_cast<int>(strlen(query)), &p,
                           nullptr));
    return p;
  }

  void bind_values(int col) {}

  template <typename Arg, typename... ArgRest>
  void bind_values(int col, const Arg& val, const ArgRest&... rest) {
    bind_value(stmt_.get(), col, val);
    bind_values(col + 1, rest...);
  }

  std::shared_ptr<sqlite3_stmt> stmt_;
};

class Sqlite {
 public:
  Sqlite() = delete;
  Sqlite(const Sqlite&) = delete;
  Sqlite& operator=(const Sqlite&) = delete;

  Sqlite(const char* path) : db_(nullptr) {
    auto rc = sqlite3_open(path, &db_);
    if (rc) {
      sqlite3_close(db_);
      db_ = nullptr;
    }
  }

  Sqlite(Sqlite&& rhs) : db_(rhs.db_) {}

  ~Sqlite() {
    if (db_) {
      sqlite3_close(db_);
    }
  }

  bool is_open() const { return db_ != nullptr; }

  Statement<void> prepare(const char* query) const {
    return Statement<void>(db_, query);
  }

  template <typename... Types>
  Statement<Types...> prepare(const char* query) const {
    return Statement<Types...>(db_, query);
  }

  template <typename... Args>
  void execute(const char* query, const Args&... args) {
    prepare<void>(query).execute(args...);
  }

  template <
      typename T, typename... Rest,
      typename std::enable_if<!std::is_same<T, void>::value>::type*& = enabler,
      typename... Args>
  std::vector<typename ValueType<!sizeof...(Rest), T, Rest...>::type> execute(
      const char* query, const Args&... args) {
    return prepare<T, Rest...>(query).execute(args...);
  }

  template <typename T, typename... Args>
  T execute_value(const char* query, const Args&... args) {
    return prepare<T>(query).execute_value(args...);
  }

  template <typename T, typename... Rest, typename... Args>
  Cursor<T, Rest...> execute_cursor(const char* query, const Args&... args) {
    return prepare<T, Rest...>(query).execute_cursor(args...);
  }

 private:
  sqlite3* db_;
};

}  // namespace sqlitelib

#endif

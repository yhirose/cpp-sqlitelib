%module sqlitelib

%{
#include "sqlitelib.h"
#include <sqlite3.h>
using namespace sqlitelib;
template<typename Tag, typename Tag::type M>
struct Rob { friend typename Tag::type get(Tag) { return M; } };
struct SqliteDb { typedef sqlite3* sqlitelib::Sqlite::*type; friend type get(SqliteDb); };
template struct Rob<SqliteDb, &sqlitelib::Sqlite::db_>;
%}

%include <std_string.i>
%include <std_vector.i>
%include <exception.i>

#pragma SWIG nowarn=302,509

%exception {
  try { $action }
  catch (const std::exception& e) {
    luaL_error(L, "%s", e.what()); return SWIG_ERROR;
  }
}

%template(VectorString) std::vector<std::string>;

namespace sqlitelib {

class Sqlite {
public:
  Sqlite(const char* path);
  ~Sqlite();
  bool is_open() const;
};

}

%extend sqlitelib::Sqlite {
  void execute(const char* q) { self->execute(q); }
  void execute(const char* q, const std::vector<std::string>& p) {
    sqlite3_stmt* s;
    sqlite3_prepare_v2(self->*get(SqliteDb()), q, -1, &s, nullptr);
    for (int i = 0; i < (int)p.size(); i++)
      sqlite3_bind_text(s, i+1, p[i].data(), p[i].size(), SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_finalize(s);
  } 
  std::vector<std::string> execute_string(const char* q) {
    return self->execute<std::string>(q);
  }
  std::vector<std::string> execute_string(const char* q, const std::vector<std::string>& p) {
    sqlite3_stmt* s;
    sqlite3_prepare_v2(self->*get(SqliteDb()), q, -1, &s, nullptr);
    for (int i = 0; i < (int)p.size(); i++)
      sqlite3_bind_text(s, i+1, p[i].data(), p[i].size(), SQLITE_TRANSIENT);
    std::vector<std::string> out;
    while (sqlite3_step(s) == SQLITE_ROW)
      out.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(s, 0)),
                       sqlite3_column_bytes(s, 0));
    sqlite3_finalize(s);
    return out;
  }
}

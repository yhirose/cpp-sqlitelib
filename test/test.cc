#define CATCH_CONFIG_MAIN
#include <sqlitelib.h>

#include "catch.hpp"

using namespace std;
using namespace sqlitelib;

TEST_CASE("Sqlite Test", "[general]") {
  Sqlite db("./test.db");
  REQUIRE(db.is_open());

  db.prepare(R"(
    CREATE TABLE IF NOT EXISTS people (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT,
      age INTEGER,
      data BLOB
    )
  )")
      .execute();

  auto stmt =
      db.prepare("INSERT INTO people (name, age, data) VALUES (?, ?, ?)");

  stmt.execute("john", 10, vector<char>({'A', 'B', 'C', 'D'}));
  stmt.execute("paul", 20, vector<char>({'E', 'B', 'G', 'H'}));
  stmt.execute("mark", 15, vector<char>({'I', 'J', 'K', 'L'}));
  stmt.execute("luke", 25, vector<char>({'M', 'N', 'O', 'P'}));

  vector<pair<string, int>> data{
      {"john", 10},
      {"paul", 20},
      {"mark", 15},
      {"luke", 25},
  };

  SECTION("ExecuteInt") {
    auto sql = "SELECT age FROM people WHERE name='john'";
    auto val = db.prepare<int>(sql).execute_value();
    REQUIRE(val == 10);
  }

  SECTION("ExecuteText") {
    auto sql = "SELECT name FROM people WHERE name='john'";
    auto val = db.prepare<string>(sql).execute_value();
    REQUIRE(val == "john");
  }

  SECTION("ExecuteBlob") {
    auto sql = "SELECT data FROM people WHERE name='john'";
    auto val = db.prepare<std::vector<char>>(sql).execute_value();
    REQUIRE(val.size() == 4);
    REQUIRE(val[0] == 'A');
    REQUIRE(val[3] == 'D');
  }

  SECTION("ExecuteIntAndText") {
    auto sql = "SELECT age, name FROM people";

    auto rows = db.prepare<int, string>(sql).execute();
    REQUIRE(rows.size() == 4);

    auto [age, name] = rows[3];
    REQUIRE(age == 25);
    REQUIRE(name == "luke");
  }

  SECTION("Bind") {
    {
      auto sql = "SELECT name FROM people WHERE age > ?";
      auto stmt = db.prepare<string>(sql);

      {
        auto rows = stmt.execute(10);
        REQUIRE(rows.size() == 3);
        REQUIRE(rows[0] == "paul");
      }

      {
        auto rows = stmt.bind(10).execute();
        REQUIRE(rows.size() == 3);
        REQUIRE(rows[0] == "paul");
      }
    }

    {
      auto sql = "SELECT age FROM people WHERE name LIKE ?";
      auto val = db.prepare<int>(sql).execute_value("jo%");
      REQUIRE(val == 10);
    }

    {
      auto sql = "SELECT id FROM people WHERE name=? AND age=?";
      auto val = db.prepare<int>(sql).execute_value("john", 10);
      REQUIRE(val == 1);
    }
  }

  SECTION("ReusePreparedStatement") {
    {
      auto stmt = db.prepare<string>("SELECT name FROM people WHERE age>?");
      auto rows = stmt.execute(10);
      REQUIRE(rows.size() == 3);
      REQUIRE(rows[0] == "paul");

      rows = stmt.execute(20);
      REQUIRE(rows.size() == 1);
      REQUIRE(rows[0] == "luke");
    }
  }

  SECTION("CreateTable") {
    db.prepare(R"(
      CREATE TABLE IF NOT EXISTS test (key TEXT PRIMARY KEY, value INTEGER);
    )")
        .execute();

    db.prepare("INSERT INTO test (key, value) VALUES ('zero', 0);").execute();
    db.prepare("INSERT INTO test (key, value) VALUES ('one', 1);").execute();

    auto stmt = db.prepare("INSERT INTO test (key, value) VALUES (?, ?);");
    stmt.execute("two", 2);
    stmt.execute("three", 3);

    auto rows =
        db.prepare<string, int>("SELECT key, value FROM test").execute();
    REQUIRE(rows.size() == 4);
    REQUIRE(get<0>(rows[1]) == "one");
    REQUIRE(get<1>(rows[3]) == 3);

    db.prepare("DROP TABLE IF EXISTS test;").execute();
  }

  SECTION("Iterator") {
    auto sql = "SELECT name, age FROM people";
    auto stmt = db.prepare<string, int>(sql);

    {
      auto itData = data.begin();
      auto cursor = stmt.execute_cursor();
      for (auto it = cursor.begin(); it != cursor.end(); ++it) {
        REQUIRE(itData->first == get<0>(*it));
        REQUIRE(itData->second == get<1>(*it));
        ++itData;
      }
    }

    {
      auto itData = data.begin();
      for (const auto& [name, age] : stmt.execute_cursor()) {
        REQUIRE(itData->first == name);
        REQUIRE(itData->second == age);
        ++itData;
      }
    }
  }

  SECTION("IteratorSingleColumn") {
    auto sql = "SELECT name FROM people";
    auto stmt = db.prepare<string>(sql);

    auto itData = data.begin();
    for (const auto& x : stmt.execute_cursor()) {
      REQUIRE(itData->first == x);
      ++itData;
    }
  }

  SECTION("Count") {
    auto sql = "SELECT COUNT(*) FROM people";
    auto val = db.prepare<int>(sql).execute_value();
    REQUIRE(val == 4);
  }

  SECTION("FlatAPI") {
    auto val = db.execute_value<int>("SELECT COUNT(*) FROM people");
    REQUIRE(val == 4);

    auto rows = db.execute<string>("SELECT name FROM people WHERE age > ?", 10);
    REQUIRE(rows.size() == 3);
    REQUIRE(rows[0] == "paul");

    auto rows2 = db.execute<int, string>("SELECT age, name FROM people");
    REQUIRE(rows2.size() == 4);

    db.execute("DROP TABLE IF EXISTS test;");
  }

  SECTION("FlatAPI - Iterator") {
    {
      auto itData = data.begin();
      auto cursor =
          db.execute_cursor<string, int>("SELECT name, age FROM people");
      for (auto it = cursor.begin(); it != cursor.end(); ++it) {
        REQUIRE(itData->first == get<0>(*it));
        REQUIRE(itData->second == get<1>(*it));
        ++itData;
      }
    }

    {
      auto itData = data.begin();
      for (const auto& [name, age] :
           db.execute_cursor<string, int>("SELECT name, age FROM people")) {
        REQUIRE(itData->first == name);
        REQUIRE(itData->second == age);
        ++itData;
      }
    }
  }

  SECTION("FlatAPI - IteratorSingleColumn") {
    auto rng = db.execute_cursor<string>("SELECT name FROM people");
    auto itData = data.begin();
    for (const auto& x : rng) {
      REQUIRE(itData->first == x);
      ++itData;
    }
  }

  db.prepare("DROP TABLE IF EXISTS people").execute();
}


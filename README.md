cpp-sqlitelib
=============

A single file C++ header-only SQLite wrapper library

## Open database

```cpp
#include <sqlitelib.h>
using namespace sqlitelib;
auto db = Sqlite("./test.db");
```

## Create table

```cpp
db.execute(R"(
  CREATE TABLE IF NOT EXISTS people (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT,
    age INTEGER,
    data BLOB
  )
)");
```

## Drop table

```cpp
db.execute("DROP TABLE IF EXISTS people");
```

## Insert records

```cpp
auto stmt = db.prepare("INSERT INTO people (name, age, data) VALUES (?, ?, ?)");
stmt.execute("john", 10, vector<char>({ 'A', 'B', 'C', 'D' }));
stmt.execute("paul", 20, vector<char>({ 'E', 'B', 'G', 'H' }));
stmt.execute("mark", 15, vector<char>({ 'I', 'J', 'K', 'L' }));
stmt.execute("luke", 25, vector<char>({ 'M', 'N', 'O', 'P' }));
```

## Select a record (single colum)

```cpp
auto val = db.execute_value<int>("SELECT age FROM people WHERE name='john'");
val; // 10
```

## Select records (multiple columns)

```cpp
auto rows = db.execute<int, std::string>("SELECT age, name FROM people");
rows.size(); // 4

auto [age, name] = rows[3]; // age: 25, name: luke
```

## Bind #1

```cpp
auto stmt = db.prepare<std::string>("SELECT name FROM people WHERE age > ?");

auto rows = stmt.execute(10);
rows.size(); // 3
rows[0];     // paul

auto rows = stmt.bind(10).execute();
rows.size(); // 3
rows[0];     // paul
```

## Bind #2

```cpp
auto val = db.execute_value<int>("SELECT id FROM people WHERE name=? AND age=?", "john", 10);
val; // 1
```

## Cursor (multiple columns)

```cpp
auto stmt = db.prepare<std::string, int>("SELECT name, age FROM people");

auto cursor = stmt.execute_cursor();
for (auto it = cursor.begin(); it != cursor.end(); ++it) {
  get<0>(*it);
  get<1>(*it);
  ++it;
}

// With C++17 structured binding
for (const auto& [name, age] : stmt.execute_cursor()) {
  ;
}
```

## Cursor (single column)

```cpp
auto stmt = db.prepare<std::string>("SELECT name FROM people");

for (const auto& x: stmt.execute_cursor()) {
  ;
}
```

## Count

```cpp
auto val = db.execute_value<int>("SELECT COUNT(*) FROM people");
val; // 4
```

## Flat API

```cpp
for (const auto& [name, age] :
     db.execute_cursor<string, int>("SELECT name, age FROM people")) {
  ;
}

for (const auto& x: db.execute_cursor<std::string>("SELECT name FROM people")) {
  ;
}
```

License
-------

MIT license (© 2021 Yuji Hirose)

lua-sqlitelib
=============

A Lua SQLite library via SWIG bindings over [cpp-sqlitelib](https://github.com/yhirose/cpp-sqlitelib).

## Open database

```lua
local sqlitelib = require("sqlitelib")
 
local db = sqlitelib.Sqlite(":memory:")
-- or a file:
local db = sqlitelib.Sqlite("./test.db")
```

## Create table

```lua
db:execute([[
  CREATE TABLE IF NOT EXISTS people (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT,
    age INTEGER
  )
]])
```

## Drop table

```lua
db:execute("DROP TABLE IF EXISTS people")
```


## Insert records

```lua
db:execute("INSERT INTO people (name, age) VALUES ('fish', 10)")
```

## Select a records

```lua
local names = db:execute_string("SELECT name FROM people")
for i = 0, names:size() - 1 do
	print(names[i])
end
```

## Check connection

```lua
print(db:is_open()) -- true
```

License
-------

MIT license (© 2021 Yuji Hirose, © 2026 rycamosun)

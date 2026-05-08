local sqlitelib = require("sqlitelib")

local db = sqlitelib.Sqlite(":memory:")

db:execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, score REAL)")
db:execute("INSERT INTO users VALUES (1, 'Ry', 9.5)")

local e = sqlitelib.VectorString()
e:push_back("Eh")
db:execute("INSERT INTO users VALUES (2, ?, 7.0)", e)

local names = db:execute_string("SELECT name FROM users")
for i = 0, names:size() - 1 do
	print(names[i])
end

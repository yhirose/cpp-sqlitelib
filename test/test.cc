
#include <gtest/gtest.h>
#include <sqlitelib.h>

using namespace std;
using namespace sqlitelib;

class SqliteTest : public ::testing::Test
{
protected:
    SqliteTest() : db_("./test.db") {}

    virtual void SetUp()
    {
        EXPECT_EQ(true, db_.is_open());

        db_.prepare(
            "CREATE TABLE IF NOT EXISTS people ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    name TEXT,"
            "    age INTEGER,"
            "    data BLOB"
            ")"
            ).execute();

        auto stmt = db_.prepare("INSERT INTO people (name, age, data) VALUES (?, ?, ?)");
        stmt.execute("john", 10, vector<char>({ 'A', 'B', 'C', 'D' }));
        stmt.execute("paul", 20, vector<char>({ 'E', 'B', 'G', 'H' }));
        stmt.execute("mark", 15, vector<char>({ 'I', 'J', 'K', 'L' }));
        stmt.execute("luke", 25, vector<char>({ 'M', 'N', 'O', 'P' }));
    }

    virtual void TearDown()
    {
        db_.prepare("DROP TABLE IF EXISTS people").execute();
    }

    Sqlite db_;

	std::vector<std::pair<std::string, int>> data_ = {
		{ "john", 10 },
		{ "paul", 20 },
		{ "mark", 15 },
		{ "luke", 25 },
	};
};

TEST_F(SqliteTest, ExecuteInt)
{
    auto sql = "SELECT age FROM people WHERE name='john'";
    auto val = db_.prepare<Int>(sql).execute_value();
    EXPECT_EQ(10, val);
}

TEST_F(SqliteTest, ExecuteText)
{
    auto sql = "SELECT name FROM people WHERE name='john'";
    auto val = db_.prepare<Text>(sql).execute_value();
    EXPECT_EQ("john", val);
}

TEST_F(SqliteTest, ExecuteBlob)
{
    auto sql = "SELECT data FROM people WHERE name='john'";
    auto val = db_.prepare<Blob>(sql).execute_value();
    EXPECT_EQ(4, val.size());
    EXPECT_EQ('A', val[0]);
    EXPECT_EQ('D', val[3]);
}

TEST_F(SqliteTest, ExecuteIntAndText)
{
    auto sql = "SELECT age, name FROM people";

    auto rows = db_.prepare<Int, Text>(sql).execute();
    EXPECT_EQ(4, rows.size());

    auto row = rows[3];
    EXPECT_EQ(25, get<0>(row));
    EXPECT_EQ("luke", get<1>(row));
}

TEST_F(SqliteTest, Bind)
{
    {
        auto sql = "SELECT name FROM people WHERE age > ?";
        auto stmt = db_.prepare<Text>(sql);

        {
            auto rows = stmt.execute(10);
            EXPECT_EQ(3, rows.size()); 
            EXPECT_EQ("paul", rows[0]);
        }

        {
            auto rows = stmt.bind(10).execute();
            EXPECT_EQ(3, rows.size()); 
            EXPECT_EQ("paul", rows[0]);
        }
    }

    {
        auto sql = "SELECT age FROM people WHERE name LIKE ?";
        auto val = db_.prepare<Int>(sql).execute_value("jo%");
        EXPECT_EQ(10, val);
    }

    {
        auto sql = "SELECT id FROM people WHERE name=? AND age=?";
        auto val = db_.prepare<Int>(sql).execute_value("john", 10);
        EXPECT_EQ(1, val);
    }
}

TEST_F(SqliteTest, ReusePreparedStatement)
{
    {
        auto stmt = db_.prepare<Text>("SELECT name FROM people WHERE age>?");
        auto rows = stmt.execute(10);
        EXPECT_EQ(rows.size(), 3); 
        EXPECT_EQ("paul", rows[0]);

        rows = stmt.execute(20);
        EXPECT_EQ(rows.size(), 1); 
        EXPECT_EQ("luke", rows[0]);
    }
}

TEST_F(SqliteTest, CreateTable)
{
    {
        db_.prepare("CREATE TABLE IF NOT EXISTS test (key TEXT PRIMARY KEY, value INTEGER);").execute();

        db_.prepare("INSERT INTO test (key, value) VALUES ('zero', 0);").execute();
        db_.prepare("INSERT INTO test (key, value) VALUES ('one', 1);").execute();

        auto stmt = db_.prepare("INSERT INTO test (key, value) VALUES (?, ?);");
        stmt.execute("two", 2);
        stmt.execute("three", 3);

        auto rows = db_.prepare<Text, Int>("SELECT key, value FROM test").execute();
        EXPECT_EQ(rows.size(), 4); 
        EXPECT_EQ("one", get<0>(rows[1])); 
        EXPECT_EQ(3, get<1>(rows[3])); 

        db_.prepare("DROP TABLE IF EXISTS test;").execute();
    }
}

TEST_F(SqliteTest, Iterator)
{
    auto sql = "SELECT name, age FROM people";
    auto stmt = db_.prepare<Text, Int>(sql);

    auto cursor = stmt.execute_cursor();
    auto it = cursor.begin();
    auto itData = data_.begin();
    while (it != cursor.end()) {
        EXPECT_EQ(itData->first, get<0>(*it)); 
        EXPECT_EQ(itData->second, get<1>(*it)); 
        ++it;
        ++itData;
    }

    itData = data_.begin();
    for (const auto& x: stmt.execute_cursor()) {
        EXPECT_EQ(itData->first, get<0>(x)); 
        EXPECT_EQ(itData->second, get<1>(x)); 
        ++itData;
    }
}

TEST_F(SqliteTest, IteratorSingleColumn)
{
    auto sql = "SELECT name FROM people";
    auto stmt = db_.prepare<Text>(sql);

    auto itData = data_.begin();
    for (const auto& x: stmt.execute_cursor()) {
        EXPECT_EQ(itData->first, x); 
        ++itData;
    }
}

TEST_F(SqliteTest, Count)
{
    auto sql = "SELECT COUNT(*) FROM people";
    auto val = db_.prepare<Int>(sql).execute_value();
    EXPECT_EQ(4, val);
}

TEST_F(SqliteTest, FlatAPI)
{
    auto val = db_.execute_value<Int>("SELECT COUNT(*) FROM people");
    EXPECT_EQ(4, val);

    auto rows = db_.execute<Text>("SELECT name FROM people WHERE age > ?", 10);
    EXPECT_EQ(3, rows.size()); 
    EXPECT_EQ("paul", rows[0]);

    auto rows2 = db_.execute<Int, Text>("SELECT age, name FROM people");
    EXPECT_EQ(4, rows2.size());

    db_.execute("DROP TABLE IF EXISTS test;");
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix

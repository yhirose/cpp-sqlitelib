
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
            ");"
            ).execute();

        auto stmt = db_.prepare("INSERT INTO people (name, age, data) VALUES (?, ?, ?);");
        stmt.execute("john", 10, std::vector<char>({ 'A', 'B', 'C', 'D' }));
        stmt.execute("paul", 20, std::vector<char>({ 'E', 'B', 'G', 'H' }));
        stmt.execute("mark", 15, std::vector<char>({ 'I', 'J', 'K', 'L' }));
        stmt.execute("luke", 25, std::vector<char>({ 'M', 'N', 'O', 'P' }));
    }

    virtual void TearDown()
    {
        db_.prepare("DROP TABLE IF EXISTS people;").execute();
    }

    Sqlite db_;
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
    EXPECT_EQ(25, std::get<0>(row));
    EXPECT_EQ("luke", std::get<1>(row));
}

TEST_F(SqliteTest, Bind)
{
    {
        auto sql = "SELECT name FROM people WHERE age>?";
        auto stmt = db_.prepare<Text>(sql);

        {
            auto rows = stmt.execute(10);
            EXPECT_EQ(rows.size(), 3); 
            EXPECT_EQ("paul", rows[0]);
        }

        {
            auto rows = stmt.bind(10).execute();
            EXPECT_EQ(rows.size(), 3); 
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
        EXPECT_EQ("one", std::get<0>(rows[1])); 
        EXPECT_EQ(3, std::get<1>(rows[3])); 

        db_.prepare("DROP TABLE IF EXISTS test;").execute();
    }
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix

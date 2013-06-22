
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
	}

    Sqlite db_;
};

TEST_F(SqliteTest, ExecuteInt)
{
	auto sql = "SELECT age FROM people WHERE name='john'";
	{
		auto val = db_.execute_value<Int>(sql);
		EXPECT_EQ(10, val);
	}

	{
		Value val = db_.execute_value(sql);
		EXPECT_EQ(10, val.integer);
	}
}

TEST_F(SqliteTest, ExecuteText)
{
	auto sql = "SELECT name FROM people WHERE name='john'";
	{
		auto val = db_.execute_value<Text>(sql);
		EXPECT_EQ("john", val);
	}

	{
		auto val = db_.execute_value(sql);
		EXPECT_EQ("john", val.text);
	}
}

TEST_F(SqliteTest, ExecuteBlob)
{
	auto sql = "SELECT data FROM people WHERE name='john'";

	{
		auto val = db_.execute_value<Blob>(sql);
		EXPECT_EQ(4, val.size());
		EXPECT_EQ('A', val[0]);
		EXPECT_EQ('D', val[3]);
	}

	{
		auto val = db_.execute_value(sql);
		EXPECT_EQ(4, val.blob.size());
		EXPECT_EQ('A', val.blob[0]);
		EXPECT_EQ('D', val.blob[3]);
	}
}

TEST_F(SqliteTest, ExecuteIntAndText)
{
	auto sql = "SELECT age, name FROM people";

	{
		auto rows = db_.execute<Int, Text>(sql);
		EXPECT_EQ(4, rows.size());

		auto row = rows[3];
		EXPECT_EQ(25, std::get<0>(row));
		EXPECT_EQ("luke", std::get<1>(row));
	}

	{
		auto rows = db_.execute(sql);
		EXPECT_EQ(4, rows.size());

		auto row = rows[3];
		EXPECT_EQ(2, row.size());
		EXPECT_EQ(25, row[0].integer);
		EXPECT_EQ("luke", row[1].text);
	}
}

TEST_F(SqliteTest, Bind)
{
	{
		auto sql = "SELECT name FROM people WHERE age=?";
		auto val = db_.bind(10).execute_value<Text>(sql);
		EXPECT_EQ("john", val);
	}

	{
		auto sql = "SELECT age FROM people WHERE name=?";
		auto val = db_.bind("john").execute_value<Int>(sql);
		EXPECT_EQ(10, val);
	}

	{
		auto sql = "SELECT id FROM people WHERE name=? AND age=?";
		auto val = db_.bind("john", 10).execute_value<Int>(sql);
		EXPECT_EQ(1, val);
	}
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix

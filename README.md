cpp-sqlitelib
=============

C++ SQLite wrapper library

## Open database

    #include <sqlitelib.h>
    using namespace sqlitelib;
    auto db = Sqlite('./test.db');
    db.prepare(
        "CREATE TABLE IF NOT EXISTS people ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT,"
        "    age INTEGER,"
        "    data BLOB"
        ");"
        ).execute();

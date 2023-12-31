#ifndef CVX_DB_SQLITE_EXCEPTIONS_HPP
#define CVX_DB_SQLITE_EXCEPTIONS_HPP

#include <cvx/db/exception.hpp>

#include <sqlite3.h>

namespace cvx { namespace db {

class SQLiteException: public Exception {
public:
    SQLiteException(sqlite3 *handle): Exception(sqlite3_errmsg(handle)) {}
};

} // namespace db
} // namespace cvx

#endif

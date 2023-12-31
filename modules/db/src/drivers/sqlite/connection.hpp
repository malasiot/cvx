#ifndef CVX_DB_SQLITE_DRIVER_CONNECTION_HPP
#define CVX_DB_SQLITE_DRIVER_CONNECTION_HPP

#include <sqlite3.h>

#include <cvx/db/connection_handle.hpp>

namespace cvx { namespace db {

class SQLiteConnectionHandle: public ConnectionHandle {
public:
    SQLiteConnectionHandle(sqlite3 *handle): handle_(handle) {}
    ~SQLiteConnectionHandle() { close() ; }

    void close() override ;

    StatementHandlePtr createStatement(const std::string &sql) ;

    void begin() override ;
    void commit() override ;
    void rollback() override ;

    uint64_t last_insert_rowid() const override ;

private:

    void exec(const std::string &sql...);

    sqlite3 *handle_ ;
};



} // namespace db
} // namespace cvx

#endif

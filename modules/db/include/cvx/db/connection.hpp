#ifndef CVX_DB_CONNECTION_HPP
#define CVX_DB_CONNECTION_HPP

#include <string>

#include <cvx/db/query.hpp>
#include <cvx/db/connection_handle.hpp>
#include <cvx/db/transaction.hpp>
#include <cvx/util/misc/dictionary.hpp>

namespace cvx { namespace db {

class Statement ;
class Query ;

class Connection {

public:

    Connection();
    Connection(const std::string &dsn);

    Connection(const Connection &other) = delete ;
    Connection &operator = ( const Connection &other) = delete ;

    // open connection to database withe given params
    void open(const std::string &dsn);
    void close() ;

    operator int () { return (bool)handle_  ; }

    uint64_t last_insert_rowid() {
        return handle_->last_insert_rowid() ;
    }

//   sqlite3_int64 last_insert_rowid() {
//       return sqlite3_last_insert_rowid(handle_);
//   }

//    int changes() {
//        return sqlite3_changes(handle_);
//    }

    ConnectionHandlePtr handle() const { return handle_ ; }

    Statement prepareStatement(const std::string &sql) ;
    Query prepareQuery(const std::string &sql) ;

    // helper for creating a connection and binding parameters
    template<typename ...Args>
    QueryResult query(const std::string & sql, Args... args) {
        return Query(*this, sql)(args...) ;
    }

    template<typename ...Args>
    void execute(const std::string &sql, Args... args) {
        Statement stmt(*this, sql) ;
        stmt(args...) ;
    }

    Transaction transaction() ;

    void check() ;

protected:

    std::shared_ptr<ConnectionHandle> handle_ ;
};


} // namespace db
} // namespace cvx


#endif

#ifndef CVX_DB_CONNECTION_HANDLE_HPP
#define CVX_DB_CONNECTION_HANDLE_HPP

#include <memory>
#include <cvx/db/statement_handle.hpp>

namespace cvx { namespace db {

class ConnectionHandle {
public:
    ConnectionHandle() = default ;

    virtual ~ConnectionHandle() {}
    virtual void close() = 0 ;
    virtual StatementHandlePtr createStatement(const std::string &sql) = 0;

    virtual void begin() = 0 ;
    virtual void commit() = 0 ;
    virtual void rollback() = 0 ;

    virtual uint64_t last_insert_rowid() const = 0 ;
} ;

typedef std::shared_ptr<ConnectionHandle> ConnectionHandlePtr ;

}
}

#endif

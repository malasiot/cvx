#ifndef CVX_DB_TRANSACTION_HPP
#define CVX_DB_TRANSACTION_HPP

#include <cvx/db/connection_handle.hpp>

#include <string>

namespace cvx { namespace db {

class Connection ;

class Transaction
{
public:

    Transaction(Connection &con_); // the constructor starts the constructor

    // you should explicitly call commit or rollback to close it

    void commit();
    void rollback();

private:

    ConnectionHandlePtr con_ ;
};

}   // namespace db
} // namespace cvx

#endif

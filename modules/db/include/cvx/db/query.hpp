#ifndef CVX_DB_QUERY_HPP
#define CVX_DB_QUERY_HPP

#include <cvx/db/statement.hpp>
#include <cvx/db/query_result.hpp>

#include <string>

namespace cvx { namespace db {

class Connection ;
class QueryResult ;

class Query: public Statement {
public:
    Query(Connection &con, const std::string &sqlite) ;

    template<typename ...Args>
    Query(Connection& con, const std::string & sql, Args... args): Query(con, sql) {
        bindAll(args...) ;
    }

    QueryResult exec() ;

    template<typename ...Args>
    QueryResult operator()(Args... args) {
        bindAll(args...) ;
        return exec() ;
    }

    QueryResult operator()() {
        return exec() ;
    }
};

}   // namespace db
} // namespace cvx


#endif

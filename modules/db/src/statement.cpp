#include <cvx/db/statement.hpp>
#include <cvx/db/connection.hpp>

#include <cvx/util/misc/strings.hpp>
using namespace std ;

namespace cvx { namespace db {

Statement::Statement(Connection &con, const std::string & sql) {
    con.check() ;
    stmt_ = con.handle()->createStatement(sql) ;
}

std::string escapeName(const std::string &unescaped) {
    string e = cvx::util::replaceAllCopy(unescaped, "\"", "\"\"") ;
    return '"' + e + '"' ;
}

} // namespace db
} // namespace cvx

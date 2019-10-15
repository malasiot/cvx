#include "driver_factory.hpp"

#include "drivers/sqlite/driver.hpp"
#ifdef HAS_PGSQL_DRIVER
#include "drivers/pgsql/driver.hpp"
#endif

#include <cvx/util/misc/strings.hpp>

using namespace std ;

namespace cvx { namespace db {

using util::Dictionary ;

std::shared_ptr<ConnectionHandle> DriverFactory::createConnection(const std::string &dsn) const
{
    int pos = dsn.find_first_of(':') ;
    if ( pos == string::npos ) return nullptr ;

    string driver_name = dsn.substr(0, pos) ;
    string param_str = dsn.substr(pos+1) ;

    Dictionary params ;
    parseParamString(param_str, params) ;

    if ( driver_name == "sqlite" )
        return SQLiteDriver::instance().open(params) ;
#ifdef HAS_PGSQL_DRIVER
    if ( driver_name == "pgsql")
        return PGSQLDriver::instance().open(params) ;
#endif
    return nullptr ;

}


bool DriverFactory::parseParamString(const string &str, Dictionary &params)
{
    const auto tokens = cvx::util::split(str, ";") ;

    for ( auto &&tok: tokens ) {
        size_t pos = tok.find('=') ;

        string key, val ;

        if ( pos == string::npos ) {
            key = cvx::util::trimCopy(tok) ;
        }
        else {
            key = cvx::util::trimCopy(tok.substr(0, pos)) ;
            val = cvx::util::trimCopy(tok.substr(pos+1)) ;
        }

        if ( key.empty() ) return false ;
        params.add(key, val) ;
    }

    return true ;
}

}
}


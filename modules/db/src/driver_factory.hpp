#ifndef XDB_DRIVER_FACTORY_HPP
#define XDB_DRIVER_FACTORY_HPP

#include <string>

#include <cvx/db/connection.hpp>
#include <cvx/util/misc/dictionary.hpp>

namespace cvx { namespace db {

class DriverFactory {

public:

    DriverFactory() = default ;

    // open connection to database with given params
    // dsn is a connection string or uri link to local file (similar to PHP PDO)

    std::shared_ptr<ConnectionHandle> createConnection(const std::string &dsn) const ;

    static const DriverFactory &instance() {
        static DriverFactory factory ;
        return factory ;
    }

    static bool parseParamString(const std::string &str, util::Dictionary &params) ;
};


} // namespace db
} // namespace xdb



#endif

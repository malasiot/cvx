#ifndef CVX_DB_SQLITE_DRIVER_HPP
#define CVX_DB_SQLITE_DRIVER_HPP

#include <memory>
#include <cvx/util/misc/dictionary.hpp>

namespace cvx { namespace db {

class ConnectionHandle ;

// The SQLite3 driver
//
// The connections string is of the form sqlite:db=<file_path>[;mode=<value>][;cache=<value>][;mutex=<value>]
//
// where options are:
//   mode:   "r" (read only), "rw" ( read-write ), "rc" ( read-write|create ) ;
//   cache:  "shared" or "private"
//   mutex:  "no" or "full"
// see documentation for explanation (https://www.sqlite.org/c3ref/open.html)

class SQLiteDriver {

public:

    SQLiteDriver() = default ;

    static const SQLiteDriver &instance() {
        static SQLiteDriver instance ;
        return instance ;
    }

    std::shared_ptr<ConnectionHandle> open(const util::Dictionary &params) const ;
};


}
}




#endif

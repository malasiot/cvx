#pragma one

#include <cvx/misc/dir_iterator.hpp>


#include <sys/types.h>
#include <dirent.h>
#include <string.h>

namespace cvx { namespace detail {
class DirectoryIteratorImpl {
public:
    DirectoryIteratorImpl(const std::string &dir, DirectoryFilter filter) ;

    DirectoryIteratorImpl(const std::string &dir, const NameFilter &name_filter, DirectoryFilter filter) ;

    bool first(const std::string &dir) ;

    bool next() ;

    bool isValid() ;

    ~DirectoryIteratorImpl() ;

    const DirectoryEntry &current() const { return current_ ; }

    const std::string &dir() const { return dir_ ; }

private:

    friend class DirectoryIterator ;

    DIR *handle_ ;
    struct dirent *dp_ ;
    std::string dir_ ;
    DirectoryEntry current_ ;
    NameFilter name_filter_ ;
    DirectoryFilter filter_ ;
};


}
}

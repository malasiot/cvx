#include <cvx/misc/dir_iterator.hpp>
#include <cvx/misc/strings.hpp>
#include <regex>

#include "dir_iterator_unix.hpp"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include <cassert>


#define PATH_MAX 4096

using namespace std ;

namespace cvx { namespace detail {

DirectoryIteratorImpl::DirectoryIteratorImpl(const std::string &dir, DirectoryFilter filter): handle_(nullptr), dir_(dir), filter_(filter) {
    first(dir) ;
}

DirectoryIteratorImpl::DirectoryIteratorImpl(const std::string &dir, const NameFilter &name_filter, DirectoryFilter filter):
    handle_(nullptr), dir_(dir), name_filter_(name_filter), filter_(filter) {
    first(dir) ;
}

bool DirectoryIteratorImpl::first(const std::string &dir) {
    if ( ( handle_ = ::opendir(dir.c_str()))== 0 ) return false ;
    return next() ;
}

bool DirectoryIteratorImpl::next() {
    while ( 1 ) {
        dp_ = ::readdir(handle_) ;
        if ( dp_ == NULL ) break ;
        if ( dp_->d_type == DT_DIR && ( !strcmp(dp_->d_name, ".") || !strcmp(dp_->d_name, "..") ) ) continue ;
        current_.path_ = dp_->d_name ;
        if ( dp_->d_type == DT_UNKNOWN )
            current_.type_ = DirectoryEntry::Unknown ;
        else {
            if ( dp_->d_type == DT_DIR)
                current_.type_ = DirectoryEntry::Dir ;
            else if ( dp_->d_type == DT_REG)
                current_.type_ = DirectoryEntry::File ;
            else if ( dp_->d_type == DT_LNK )
                current_.type_ = DirectoryEntry::SymLink ;
        }
        if ( current_.type_ ==  DirectoryEntry::Dir && !(filter_ & DirectoryFilter::MatchDirs ) ) continue ;
        if ( current_.type_ ==  DirectoryEntry::File && !(filter_ & DirectoryFilter::MatchFiles ) ) continue ;
        if ( !name_filter_.match(current_) ) continue ;

        break ;
    }
    return ( dp_ != NULL ) ;
}

bool DirectoryIteratorImpl::isValid() { return handle_ != NULL && dp_ != NULL; }

DirectoryIteratorImpl::~DirectoryIteratorImpl() {
    if ( handle_)
        ::closedir(handle_) ;
}




}
}

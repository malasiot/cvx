#include <cvx/misc/path.hpp>
#include <cvx/misc/strings.hpp>
#include <regex>

#include <cassert>
#include <memory>
#include <random>
#include <cstdarg>

using namespace std ;

namespace cvx {

Path::Path(const std::string &filepath) {
    parse(filepath) ;
}

string Path::fileName() const {
    if ( path_.empty() || trailing_slash_ ) return {} ;
    size_t npos = path_.find_last_of('/') ;
    return ( npos == std::string::npos ) ? path_ : path_.substr(npos+1) ;
}

string Path::fileNameWithoutExtension() const {
    auto n = fileName() ;
    if ( n.empty() ) return n ;
    size_t pos = n.find_last_of('.') ;
    return ( pos == std::string::npos ) ? n : n.substr(0, pos) ;
}

string Path::extension() const {
    string n = fileName() ;
    if ( n.empty() ) return n ;
    size_t pos = n.find_first_of('.') ;
    return ( pos == string::npos ) ? string() : n.substr(pos+1) ;
}

string Path::native() const {
    return root_ + path_ ;
}

Path Path::normalizedPath() const {
    const auto src_elements = split(path_, "/") ;
    vector<string> dst_elements ;

    auto end = src_elements.cend() ;
    for ( auto it = src_elements.cbegin() ; it != end ; ++it ) {
        if (*it == ".") ;
        else if (*it == ".." ) {
            if ( dst_elements.empty() || dst_elements.back() == ".." ) {
                dst_elements.push_back("..") ;
            } else {
                dst_elements.pop_back();
            }
        } else {
            dst_elements.push_back(*it) ;
        }
    }

    Path result ;
    result.root_ = root_ ;
    result.path_ = cvx::join(dst_elements, "/") ;
    return result ;
}

Path Path::absolutePath(const Path &base) const {
    assert(base.isAbsolute()) ;

    if ( isAbsolute() ) return *this ;
    Path res(base) ;
    res.append(*this) ;
    return res ;
}

extern std::string resolve_sym_link(const std::string &path) ;

#include <limits.h>
#include <stdlib.h>

Path Path::canonicalPath(const Path &base) const {
    Path source(absolutePath(base)) ;

    char *resolved = ::realpath(source.native().c_str(), nullptr) ;

    if ( resolved != nullptr ) {
        string path(resolved) ;
        ::free(resolved) ;
        return Path(path) ;
    } else
        throw UnresolvedPathException(source.native()) ;
}

string Path::parent() const {
    return parentPath().native() ;
}

Path Path::parentPath() const {
    string path ;
    size_t pos = path_.find_last_of('/') ;
    path = ( pos == string::npos ) ? string() : path_.substr(0, pos) ;

    Path res ;
    res.root_ = root_ ;
    res.path_ = path ;

    return res ;
}

Path::Path(const Path &parent, const string &child): Path(parent) {
    append(child) ;
}

Path::Path(const std::string &parent, const string &child): Path(parent) {
    append(child) ;
}


Path &Path::append(const Path &child_path) {
    if ( child_path.isAbsolute() )
        *this = child_path ;
    else {
        if ( !child_path.path_.empty() ) path_ += '/' ;
        path_.append(child_path.path_) ;
        trailing_slash_ = child_path.trailing_slash_ ;
    }
    return *this ;
}

string Path::join(const std::initializer_list<string> &paths) {
    Path res ;
    for( const auto &p: paths) {
        res.append(p) ;
    }
    return res.native() ;
}


bool Path::mkdirs() const
{
    string dir_path = root_ ;

    ElementIterator it, start = begin(), end ;
    for(  it = start ; it != end ; ++it ) {
        if ( it != start ) dir_path += path_separator_ ;
        dir_path += *it ;

        if ( exists(dir_path) ) continue ;

        bool res = mkdir(dir_path) ;

        if ( !res ) return false ;
    }

    return true ;
}


bool Path::mkdirs(const string &path) {
    return Path(path).mkdirs() ;
}


vector<string> Path::entries(const string &dir, DirectoryFilter filter, bool relative, bool recursive)
{
    vector<string> entries ;

    if ( recursive ) {
        RecursiveDirectoryIterator dit(dir, filter), dend ;
        for( ; dit != dend ; ++dit ) {
            if ( relative )
                entries.emplace_back(dit->path()) ;
            else
                entries.emplace_back(Path(dir, dit->path()).native()) ;
        }
    } else {
        DirectoryIterator dit(dir, filter), dend ;
        for( ; dit != dend ; ++dit ) {
            if ( relative )
                entries.emplace_back(dit->path()) ;
            else
                entries.emplace_back(Path(dir, dit->path()).native()) ;
        }
    }

    return entries ;
}

std::string rand_string(uint sz) {
    //1) create a non-deterministic random number generator
    static char rchars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, sizeof(rchars)-1);

    std::string str(sz, 0);
    std::generate_n( str.begin(), sz, [&] () { return rchars[ dist(rng) ] ; } );
    return str ;
}

std::string Path::uniquePath(const std::string &dir, const std::string &prefix, const std::string &suffix) {

    while (1) {
        Path res(dir) ;
        std::string filename(prefix + rand_string(8) + suffix) ;
        res /= filename ;
        if ( !res.exists() ) return res.native() ;
    }
}


std::string Path::tempFilePath(const std::string &prefix, const std::string &suffix) {
    return uniquePath(tempPath(), prefix, suffix) ;
}

}


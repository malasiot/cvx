#include <cvx/misc/path.hpp>
#include <cvx/misc/strings.hpp>
#include <regex>

#include <cassert>
#include <memory>
#include <random>

using namespace std ;

namespace cvx {

Path::Path(const std::string &filepath) {
    parse(filepath) ;
}

string Path::extension() const {
    string n = name() ;
    if ( n.empty() ) return n ;

    size_t pos = n.find_first_of('.') ;
    return ( pos == string::npos ) ? string() : n.substr(pos+1) ;
}

string Path::toString() const {
    return root_ + path_ ;
}

Path Path::normalizePath() const {
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
    result.path_ = join(dst_elements, "/") ;
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

Path Path::canonicalPath(const Path &base) const
{
    Path source(absolutePath(base)) ;

    vector<string> elements ;

    for( const auto &e: split(source.path_, "/") ) {

        std::string symlink = resolve_sym_link(e) ;

        if ( !symlink.empty() ) {
                // if this is a symlink we have to rebuild the source string and restart scanning to properly deal with "." abd ".."
/*


                Path link(symlink) ;

                if ( link.isAbsolute() ) {
                    Path tmp(link) ;
                    for( ++it ; it != end ; ++it )
                        tmp.elements_.emplace_back(*it) ;
                    source = tmp ;
                }
                else {
                    Path tmp(res) ;
                    tmp.append(link) ;
                    for( ++it ; it != end ; ++it )
                        tmp.elements_.emplace_back(*it) ;
                    source = tmp ;
                }
                rescan = true ;
                break ;
*/
            }
        else {
            elements.push_back(e) ;
        }
    }

    Path res ;
    res.root_ = source.root_ ;
    res.path_ = join(elements, "/") ;

    return Path(res).normalizePath() ;
}

string Path::parent() const {
    return parentPath().toString() ;
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


void Path::append(const Path &child_path) {
    if ( !child_path.path_.empty() ) path_ += '/' ;
    path_.append(child_path.path_) ;
}


bool Path::mkdirs() const
{
    string dir_path = root_ ;

    for( auto  it = elements_.begin() ; it != elements_.end() ; ++it ) {
        if ( it != elements_.begin() ) dir_path += path_separator_ ;
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

vector<string> Path::entries(const string &dir, DirectoryFilter filter, bool relative)
{
    vector<string> entries ;
    DirectoryIterator dit(dir, filter), dend ;
    for( ; dit != dend ; ++dit ) {
        if ( relative )
            entries.emplace_back(dit->path()) ;
        else
            entries.emplace_back(Path(dir, dit->path()).toString()) ;
    }
    return entries ;
}

string Path::toString(PathElements::const_iterator start, PathElements::const_iterator end) const {

    string res = root_ ;
    for( auto it = start ; it != end ; ++it ) {
        if ( it != start ) res += path_separator_ ;
        res += *it ;
    }
    return res ;
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

Path Path::uniquePath(const Path &dir, const std::string &prefix, const std::string &suffix) {

    while (1) {
        Path res(dir) ;
        std::string filename(prefix + rand_string(8) + suffix) ;
        res /= filename ;
        if ( !res.exists() ) return res ;
    }
}


Path Path::tempFilePath(const std::string &prefix, const std::string &suffix) {
    return uniquePath(tempPath(), prefix, suffix) ;
}

}


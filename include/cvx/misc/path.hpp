#ifndef CVX_PATH_HPP
#define CVX_PATH_HPP

#include <vector>
#include <string>
#include <stdexcept>

#include <cvx/misc/dir_iterator.hpp>
#include <cvx/misc/strings.hpp>

namespace cvx {

// Filesystem path

class Path {
	public:
		
    Path() = default ;
    Path(const Path &parent, const std::string &child) ;
    Path(const std::string &parent, const std::string &child) ;
    Path(const std::string &pathname) ;

    // filename or directory name
    std::string fileName() const;

    std::string fileNameWithoutExtension() const;

    std::string extension() const ;

    // convert to native string
    std::string native() const ;

    // remove redundant elements such as '.' and '..'
    Path normalizedPath() const ;
    std::string normalized() const { return normalizedPath().native() ; }

    // get parent directory if exists

    std::string parent() const ;
    Path parentPath() const ;

    // get absolute path

    Path absolutePath(const Path &base = Path::currentWorkingDir()) const ;
    std::string absolute(const Path &base = Path::currentWorkingDir()) const {
        return absolutePath(base).native() ;
    }

    // makes a unique path by eliminating "." and ".." and resolving symbolic links

    Path canonicalPath(const Path &base = Path::currentWorkingDir()) const ;
    std::string canonical(const Path &base = Path::currentWorkingDir()) const {
        return canonicalPath(base).native() ;
    }


    // check if it is an absolute path
    bool isAbsolute() const { return !root_.empty() ; }

    // check if file exists
    bool exists() const {
        return exists(native()) ;
    }

    // check if it is a directory
    bool isDirectory() const {
        return isDirectory(native()) ;
    }

    // check if it is a regular file
    bool isFile() const {
        return isDirectory(native()) ;
    }


    DirectoryListing subDirs() const {
        return DirectoryListing(native()) ;
    }

    DirectoryListing subDirs(DirectoryFilter filter) const {
        return DirectoryListing(native(), filter) ;
    }

    // append child path

    Path &append(const Path &child) ;

    Path & operator /= (const Path & rhs) {
        append(rhs) ;
        return *this ;
    }

    Path & operator /= (const char *rhs) {
        append(std::string(rhs)) ;
        return *this ;
    }

    Path operator / (const Path & rhs) const {
        Path tmp(*this) ;
        tmp.append(rhs) ;
        return tmp ;
    }

    Path operator / (const char *rhs) const {
        return *this / Path(rhs) ;
    }

    static std::string join(const std::string base, const std::string &child1) {
        return Path(base, child1).native() ;
    }
    static std::string join(const std::string base, const std::string &child1, const std::string &child2) {
        return Path(base, child1).append(child2).native() ;
    }

    static std::string join(const std::initializer_list<std::string> &rest) ;

    static std::vector<std::string> glob(const std::string &dir, const std::string &pattern, bool relative = true, bool recursive = false) {
           return entries(dir, DirectoryFilter(pattern, DirectoryFilter::MatchAll), relative, recursive) ;
    }

    const std::string &root() const { return root_ ; }

    struct ElementIterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::string;
        using pointer           = const std::string*;
        using reference         = const std::string&;

        const Path *path_ = nullptr ;
        std::string current_ ;
        std::string::const_iterator it_, end_ ;

        ElementIterator(const Path *p): path_(p) {
            it_ = std::begin(p->path_) ;
            end_ = std::end(p->path_) ;
            advance() ;
        }

        ElementIterator() = default ;

        reference operator*() const { return current_; }

        // Prefix increment
        ElementIterator& operator++() {
             advance() ;
             return *this ;
        }

        void advance() {
            current_.clear() ;
            while ( it_ != end_ ) {
                if (*it_ != '/') {
                    current_ += *it_ ;
                    ++it_ ;
                } else {
                    ++it_ ;
                    break ;
                }
            }
        }

            // Postfix increment
         ElementIterator operator++(int) { ElementIterator tmp = *this; ++(*this); return tmp; }

         bool isValid() const { return it_ != end_ || !current_.empty(); }

         friend bool operator== (const ElementIterator& a, const ElementIterator& b) {
             return (a.path_ == b.path_ && a.it_ == b.it_ ) ||
                     (!a.path_ && b.path_ && !b.isValid())
                           || (!b.path_ && a.path_ && !a.isValid() ) ;
         };

         bool operator!= (const ElementIterator& b) {
             return !(*this == b);
         }
    };

    ElementIterator begin() const { return ElementIterator(this) ; }
    ElementIterator end() const { return ElementIterator() ; }

    std::vector<std::string> elements() const {
        return split(path_, "/");
    }

    friend std::ostream &operator << ( std::ostream &strm, const Path &path ) {
        strm << path.native() ;
        return strm ;
    }

    bool mkdir() const {
        return mkdir(native()) ;
    }

    bool mkdirs() const ;

    static std::string currentWorkingDir() ;
    static std::string executablePath();
    static std::string homePath();
    static std::string tempPath();
    static std::string nativeDataDir();
    static std::string uniquePath(const std::string &dir, const std::string &prefix, const std::string &suffix = "");
    static std::string tempFilePath(const std::string &prefix = "tmp-", const std::string &suffix = "");
    static std::string nativeConfigDir(const std::string &app_name);

    static bool exists(const std::string &path) ;
    static bool isDirectory(const std::string &path) ;
    static bool isFile(const std::string &path) ;
    static bool mkdir(const std::string &path) ;
    static bool mkdirs(const std::string &path) ;
    static bool remove(const std::string &path) ;
    static bool remove(const Path &path) {
        return remove(path.native()) ;
    }

    static bool rename(const std::string &orig_path, const std::string &new_path) ;

    // get directory contents matching the filter
    static std::vector<std::string> entries(const std::string &dir, DirectoryFilter filter, bool relative = true, bool recursive = false) ;

private:
    void parse(const std::string &path) ; // can throw InvalidPathException

private:

    std::string drive_ ;
    std::string root_ ; // root dir
    std::string path_ ; // relative path
    bool trailing_slash_ = false ;

    static const char path_separator_ ; // native path separator
} ;


class InvalidPathException: public std::runtime_error {
public:
    InvalidPathException(const std::string &path_name):
        std::runtime_error("Invalid filesystem path: " + path_name) {}
};

class UnresolvedPathException: public std::runtime_error {
public:
    UnresolvedPathException(const std::string &path_name):
        std::runtime_error("Unresolved path: " + path_name) {}
};

} // cvx

#endif

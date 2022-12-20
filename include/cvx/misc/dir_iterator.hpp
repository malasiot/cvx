#ifndef CVX_UTIL_DIR_ITERATOR_HPP
#define CVX_UTIL_DIR_ITERATOR_HPP

#include <string>
#include <functional>
#include <memory>
#include <stack>
#include <regex>

namespace cvx {

namespace detail {
    class DirectoryIteratorImpl ;
}
class Path ;

// abstract directory entry and associated path (it is always relative to container)

class DirectoryEntry {
public:
    enum Type { Dir, File, SymLink, Unknown } ;

    Type type() const { return type_ ; }
    std::string path() const { return path_ ; }
    bool isDirectory() const { return type_ == Dir ; }

    operator std::string () const { return path_ ; }

private:

    friend class detail::DirectoryIteratorImpl ;
    friend class RecursiveDirectoryIterator ;

    DirectoryEntry(): type_(Unknown) {}

    std::string path_ ;
    Type type_ ;
};


// used by directory iterator to list only files of specific type and/or matching a pattern

class DirectoryFilter {
public:
    enum MatchFlags {
        MatchDirs = 0x001,
        MatchFiles = 0x002,
        MatchAll = MatchDirs | MatchFiles
    };

    DirectoryFilter(MatchFlags flags = MatchAll) ;
    DirectoryFilter(const std::string &pattern, MatchFlags flags = MatchFiles) ;

    bool match(const DirectoryEntry &e) ;
private:

    bool matchName(const std::string &m) ;
    bool matchName(const DirectoryEntry &e) ;

    std::vector<std::regex> pats_ ;
    MatchFlags flags_ = MatchAll ;
};

// iterator class
class DirectoryIterator: public std::iterator<std::forward_iterator_tag, DirectoryEntry> {
   public:

    DirectoryIterator(const std::string &dir, DirectoryFilter filter = {} ) ;
    DirectoryIterator() ;

    const DirectoryEntry& operator*() const ;
    const DirectoryEntry* operator->() const;

    DirectoryIterator& operator++();

    friend bool operator==(DirectoryIterator a, DirectoryIterator b);
    friend bool operator!=(DirectoryIterator a, DirectoryIterator b) {
        return !(a == b) ;
    }

    const std::string &dir() const ;

private:

    std::shared_ptr<detail::DirectoryIteratorImpl> impl_ ;
};

class RecursiveDirectoryIterator: public std::iterator<std::forward_iterator_tag, DirectoryEntry> {
   public:

    RecursiveDirectoryIterator(const std::string &dir, DirectoryFilter filter = DirectoryFilter::MatchAll ) ;
    RecursiveDirectoryIterator() = default ;

    const DirectoryEntry& operator*() const ;
    const DirectoryEntry* operator->() const;

    RecursiveDirectoryIterator& operator++();

    friend bool operator==(RecursiveDirectoryIterator a, RecursiveDirectoryIterator b);
    friend bool operator!=(RecursiveDirectoryIterator a, RecursiveDirectoryIterator b) {
        return !(a == b) ;
    }

private:
   std::stack<DirectoryIterator> stack_ ;
   bool is_valid_ = false ;
   DirectoryFilter filter_ ;
   std::string dir_ ;
   DirectoryEntry current_ ;

   void setCurrent();
   void next() ;
   std::string relativePath(const std::string &dir, const std::string &m) const ;
};

// iterator range class
class DirectoryListing {
public:

    DirectoryListing(const std::string &dir,
                     DirectoryFilter filter ={}):
        dir_(dir), filter_(filter) {}

    DirectoryIterator begin() { return DirectoryIterator(dir_, filter_) ; }
    DirectoryIterator end() { return DirectoryIterator() ; }

private:
    std::string dir_ ;
    DirectoryFilter filter_ ;
};

// iterator range class
class RecursiveDirectoryListing {
public:

    RecursiveDirectoryListing(const std::string &dir,
                              DirectoryFilter filter = {}):
        dir_(dir), filter_(filter) {}

    RecursiveDirectoryIterator begin() { return RecursiveDirectoryIterator(dir_, filter_) ; }
    RecursiveDirectoryIterator end() { return RecursiveDirectoryIterator() ; }

private:
    std::string dir_ ;
    DirectoryFilter filter_ ;
};


}

#endif

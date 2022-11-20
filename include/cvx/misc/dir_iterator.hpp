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


enum DirectoryFilter {
    MatchDirs = 0x001,
    MatchFiles = 0x002,
    MatchAll = MatchDirs | MatchFiles
};

class NameFilter {
public:
    enum Flags { FileNames, DirNames, None } ;

    NameFilter(const std::string &pattern, Flags flags = FileNames) ;
    NameFilter() = default ;

    bool match(const std::string &m) ;
    bool match(const DirectoryEntry &e) ;

private:

    std::vector<std::regex> pats_ ;
    Flags flags_ = None;
};

// iterator class
class DirectoryIterator: public std::iterator<std::forward_iterator_tag, DirectoryEntry> {
   public:

    DirectoryIterator(const std::string &dir, DirectoryFilter filter = DirectoryFilter::MatchAll ) ;
    DirectoryIterator(const std::string &dir, const NameFilter &name_filter, DirectoryFilter filter = DirectoryFilter::MatchAll ) ;
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
    RecursiveDirectoryIterator(const std::string &dir, const NameFilter &name_filter, DirectoryFilter filter = DirectoryFilter::MatchAll ) ;
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
   NameFilter name_filter_ ;
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
                     DirectoryFilter filter = DirectoryFilter::MatchAll):
        dir_(dir), filter_(filter) {}
    DirectoryListing(const std::string &dir, const NameFilter &name_filter,
                     DirectoryFilter filter = DirectoryFilter::MatchAll):
        dir_(dir), filter_(filter), name_filter_(name_filter) {}

    DirectoryListing(const Path &dir,
                     DirectoryFilter filter = DirectoryFilter::MatchAll) ;


    DirectoryIterator begin() { return DirectoryIterator(dir_, name_filter_, filter_) ; }
    DirectoryIterator end() { return DirectoryIterator() ; }

private:
    std::string dir_ ;
    DirectoryFilter filter_ ;
    NameFilter name_filter_ ;
};

// iterator range class
class RecursiveDirectoryListing {
public:

    RecursiveDirectoryListing(const std::string &dir,
                     DirectoryFilter filter = DirectoryFilter::MatchAll):
        dir_(dir), filter_(filter) {}
    RecursiveDirectoryListing(const std::string &dir, const NameFilter &name_filter,
                     DirectoryFilter filter = DirectoryFilter::MatchAll):
        dir_(dir), filter_(filter), name_filter_(name_filter) {}


    RecursiveDirectoryIterator begin() { return RecursiveDirectoryIterator(dir_, name_filter_, filter_) ; }
    RecursiveDirectoryIterator end() { return RecursiveDirectoryIterator() ; }

private:
    std::string dir_ ;
    DirectoryFilter filter_ ;
    NameFilter name_filter_ ;
};


}

#endif

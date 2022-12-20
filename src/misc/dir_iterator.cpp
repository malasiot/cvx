#include <cvx/misc/dir_iterator.hpp>
#include <cvx/misc/strings.hpp>
#include <cvx/misc/path.hpp>

#include <cstring>
#include <iostream>
#include <cassert>

#include "dir_iterator_unix.hpp"

using namespace std ;

namespace cvx {

static string glob_to_regex(const char *pat)
{
    // Convert pattern
    string rx = "^", be ;

    int i = 0;
    const char *pc = pat ;
    size_t clen = strlen(pat) ;
    bool in_char_class = false ;

    while (i < clen)
    {
        char c = pc[i++];

        switch (c)
        {
        case '*':
            rx += "[^\\\\/]*" ;
            break;
        case '?':
            rx += "[^\\\\/]" ;
            break;
        case '$':  //Regex special characters
        case '(':
        case ')':
        case '+':
        case '.':
        case '|':
            rx += '\\';
            rx += c;
            break;
        case '\\':
            if ( pc[i] == '*' ) rx += "\\*" ;
            else if ( pc[i] == '?' )  rx += "\\?" ;
            ++i ;
            break ;
        case '[':
        {
            if ( in_char_class )  rx += "\\[";
            else {
                in_char_class = true ;
                be += c ;
            }
            break ;
        }
        case ']':
        {
            if ( in_char_class ) {
                in_char_class = false ;
                rx += be ;
                rx += c ;
                rx += "{1}" ;
                be.clear() ;
            }
            else rx += "\\]" ;

            break ;
        }
        case '%':
        {
#ifdef USE_BOOST_REGEX
            regex rd("(0\\d)?d", regex::extended | regex::icase) ;
#else
            regex rd("(0\\d)?d", regex_constants::extended | regex_constants::icase) ;
#endif

            smatch what;
            string q(pat + i) ;

            if ( regex_match(q, what, rd) )
            {

                rx += "[[:digit:]]" ;

                if ( what.size() == 2 )
                {
                    rx +=  "{" ;
                    rx += what[1] ;
                    rx += "}" ;
                }
                else
                    rx += "+" ;

                i += what.size() ;
            }
            else
            {
                if ( in_char_class ) be += c ;
                else rx += c;
            }
            break ;

        }
        default:
            if ( in_char_class ) be += c ;
            else rx += c;
        }
    }

    rx += "$" ;
    return rx ;
}



DirectoryIterator::DirectoryIterator(const string &dir, DirectoryFilter filter):
    impl_(new detail::DirectoryIteratorImpl(Path(dir).native(), filter)) {}

DirectoryIterator::DirectoryIterator() {
}

const DirectoryEntry &DirectoryIterator::operator*() const
{
    assert(impl_) ;
    return impl_->current() ;
}

const DirectoryEntry *DirectoryIterator::operator->() const
{
    assert(impl_) ;
    return &impl_->current() ;
}

DirectoryIterator &DirectoryIterator::operator++()
{
    impl_->next() ;
    return *this ;
}

const string &DirectoryIterator::dir() const {
    return impl_->dir() ;
}

bool operator==(DirectoryIterator a, DirectoryIterator b)
{
    return a.impl_ == b.impl_
              || (!a.impl_ && b.impl_ && !b.impl_->isValid())
              || (!b.impl_ && a.impl_ && !a.impl_->isValid());
}


bool operator==(RecursiveDirectoryIterator a, RecursiveDirectoryIterator b)
{
    return a.stack_ == b.stack_ ;
}

RecursiveDirectoryIterator::RecursiveDirectoryIterator(const string &dir, DirectoryFilter filter):
    dir_(Path(dir).native()), filter_(filter) {
    stack_.emplace(DirectoryIterator(dir_, DirectoryFilter::MatchAll) );
    next() ;
}


const DirectoryEntry &RecursiveDirectoryIterator::operator*() const {
    assert(!stack_.empty()) ;
    return current_ ;
}

void RecursiveDirectoryIterator::setCurrent() {
    auto it = stack_.top() ;
    auto e = *it ;
    current_.type_ = e.type() ;
    current_.path_ = relativePath(it.dir(), e.path_) ;
}

const DirectoryEntry* RecursiveDirectoryIterator::operator->() const {
    assert(!stack_.empty()) ;
    return &current_ ;
}

void RecursiveDirectoryIterator::next() {

    bool found = false ;
    while ( !found ) {

        auto &it = stack_.top() ;
        DirectoryIterator eend ;

        while ( it == eend ) {
            stack_.pop() ;
            if ( stack_.empty() ) return ;
            it = stack_.top() ;
            ++it ;
        }

        DirectoryEntry e = *it ;
        e.path_ = relativePath(it.dir(), e.path_) ;
        if ( e.type() == DirectoryEntry::Dir ) {
            if ( filter_.match(e) ) {
                found = true ;
                setCurrent() ;
            }
            string sub_dir = it.dir() + '/' + e.path() ;
            stack_.emplace(DirectoryIterator(sub_dir, DirectoryFilter::MatchAll)) ;
        } else {
            if ( e.type() == DirectoryEntry::File )  {
                if ( filter_.match(e) ) {
                    found = true ;
                    setCurrent() ;
                }
            }
            ++it ;

        }


    }


}

string RecursiveDirectoryIterator::relativePath(const string &adir, const string &m) const {
    if ( adir.length() != dir_.length() ) {
        string sub_dir = adir.substr(dir_.length()+1) ;
        return sub_dir + '/' + m ;
    } else {
        return m ;
    }
}


RecursiveDirectoryIterator &RecursiveDirectoryIterator::operator++() {
    next() ;
    return *this ;
}

DirectoryFilter::DirectoryFilter(DirectoryFilter::MatchFlags flags): flags_(flags) {
}

DirectoryFilter::DirectoryFilter(const string &pattern, MatchFlags flags): flags_(flags) {
    std::vector<string> el = split(pattern, ";") ;

    for(size_t i=0 ; i<el.size() ; i++) {
        try {
            string rx_str = glob_to_regex(el[i].c_str()) ;
            regex rx(rx_str, regex_constants::extended) ;
            pats_.emplace_back(std::move(rx)) ;
        }
        catch ( regex_error &e ) {
            cerr << "glob pattern expression error: " << el[i] << endl ;
        }
    }
}

bool DirectoryFilter::match(const DirectoryEntry &e) {
    if ( e.type() ==  DirectoryEntry::Dir && !( flags_ & MatchDirs ) ) return false ;
    if ( e.type() ==  DirectoryEntry::File && !( flags_ & MatchFiles ) ) return false ;
    return matchName(e.path()) ;
}

bool DirectoryFilter::matchName(const string &m) {
    if ( pats_.empty() ) return true ;
    for(size_t i=0 ; i<pats_.size() ; i++ ) {
        if ( regex_match(m, pats_[i]) )
            return true ;
    }
    return false ;
}



}

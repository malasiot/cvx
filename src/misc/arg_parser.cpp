#include <cvx/misc/arg_parser.hpp>


#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std ;

namespace cvx {

bool ArgumentParser::has(const char *flag) {
    for( const auto &o: options_ ) {
        for( const auto &f: o.flags_ ) {
            if ( f == flag && o.matched_ ) {
                return true ;
            }
        }
    }
    return false ;
}

void ArgumentParser::parse(size_t argc, const char *argv[], size_t c0) {
    std::vector<std::string> args ;
    std::copy(argv + c0, argv + argc, std::back_inserter(args)) ;

    parse(args) ;
}

void ArgumentParser::parse(const std::vector<std::string> &argv) {
    size_t argc = argv.size() ;

    PositionalContainer::iterator cpos = positional_.begin() ;

    pos_ = 0 ;

    while ( pos_ < argc ) {

        if ( argv[pos_][0] == '-' ) { // we have a non-positional option

            string val ;
            auto match = findMatchingArg(argv[pos_], val) ; // find matching arg

            if ( match != options_.end() ) { // found
                consumeArg(*match, val, argv, pos_)  ;
                if ( match->action_ && !match->action_() ) break ;
            }
            else throw InvalidOption(*this, argv[pos_]) ; // unknown option
        } else if ( cpos != positional_.end() ) {  // try positional arguments
            string val = argv[pos_] ;
            consumePositionalArg(*cpos, argv, pos_) ;
            ++cpos ;
        } else throw InvalidOption(*this, argv[pos_]) ;
    }

    setDefaults() ;
    checkRequired() ;
}



// adapted from boost::program_options code

static void format_paragraph(std::ostream& strm,
                             std::string par,
                             unsigned indent,
                             unsigned line_length)
{
    assert(indent < line_length);
    line_length -= indent;

    // index of tab (if present) is used as additional indent relative
    // to first_column_width if paragrapth is spanned over multiple
    // lines if tab is not on first line it is ignored
    string::size_type par_indent = par.find('\t');

    if ( par_indent == string::npos ) par_indent = 0;
    else  {
        // erase tab from string
        par.erase(par_indent, 1);

        // this assert may fail due to user error or environment conditions!
        assert(par_indent < line_length);

        // ignore tab if not on first line
        if ( par_indent >= line_length ) par_indent = 0 ;
    }

    if (par.size() < line_length) strm << par ; // description is short no formating needed
    else {
        string::const_iterator       line_begin = par.begin();
        const string::const_iterator par_end = par.end();

        bool first_line = true; // of current paragraph!

        while ( line_begin < par_end ) {  // paragraph lines
            if ( !first_line ) {
                // If line starts with space, but second character
                // is not space, remove the leading space.
                // We don't remove double spaces because those
                // might be intentianal.
                if ((*line_begin == ' ') && ((line_begin + 1 < par_end) && (*(line_begin + 1) != ' ')))
                    line_begin += 1;  // line_begin != line_end
            }
            // Take care to never increment the iterator past
            // the end, since MSVC 8.0 (brokenly), assumes that
            // doing that, even if no access happens, is a bug.
            unsigned remaining = static_cast<unsigned>(std::distance(line_begin, par_end));
            string::const_iterator line_end = line_begin +
                    ((remaining < line_length) ? remaining : line_length);

            // prevent chopped words
            // Is line_end between two non-space characters?
            if ((*(line_end - 1) != ' ') && ((line_end < par_end) && (*line_end != ' '))) {
                // find last ' ' in the second half of the current paragraph line
                string::const_iterator last_space =
                        find(reverse_iterator<string::const_iterator>(line_end),
                             reverse_iterator<string::const_iterator>(line_begin),
                             ' ').base();

                if ( last_space != line_begin ) {
                    // is last_space within the second half ot the current line
                    if (static_cast<unsigned>(std::distance(last_space, line_end)) < (line_length / 2))
                        line_end = last_space;
                }
            } // prevent chopped words

            // write line to stream
            copy(line_begin, line_end, ostream_iterator<char>(strm));

            if ( first_line ) {
                indent += static_cast<unsigned>(par_indent);
                line_length -= static_cast<unsigned>(par_indent); // there's less to work with now
                first_line = false;
            }

            // more lines to follow?
            if ( line_end != par_end ) {
                strm << '\n';
                for(unsigned pad = indent; pad > 0; --pad) strm.put(' ') ;
            }

            // next line starts after of this line
            line_begin = line_end;
        } // paragraph lines
    }

}


static void format_description(std::ostream& strm,
                               const std::string& desc,
                               unsigned first_column_width,
                               unsigned line_length)  {

    assert( line_length > 1 );

    --line_length;

    assert(line_length > first_column_width);

    vector<string> lines = split(desc, "\n") ;

    for( auto it = lines.begin() ; it != lines.end() ;  ) {
        format_paragraph(strm, *it, first_column_width, line_length);
        ++it ;

        if ( it != lines.end() ) {
            strm << '\n';
            for ( uint pad = first_column_width ; pad > 0; --pad ) strm.put(' ') ;
        }
    }  // paragraphs
}

static void format_text(std::ostream& strm,  const std::string& str, unsigned line_length) {
    string tmp;
    string::const_iterator it = str.begin() ;
    char cur = 0, last = 0;
    size_t i = 0;

    while ( it != str.end() ) {
        cur = *it++ ;
        if ( ++i == line_length ) {
            ltrim(tmp) ;
            strm << endl << tmp ;
            i = tmp.length() ;
            tmp.clear() ;
        } else if ( cur == '\n' ) {
            strm << tmp;
            tmp.clear();
            i = 0 ;
        } else if (isspace(cur) && !isspace(last)) { // a word
            strm << tmp;
            tmp.clear();
        }

        tmp += cur;
        last = cur;
    }

}
void ArgumentParser::printUsage(ostream &strm, uint line_length, uint min_description_width)
{
    if ( !description_.empty() ) {
        format_text(strm, description_, line_length) ;
        strm << endl << endl ;
    }

    printOptions(strm, line_length, min_description_width) ;

    if ( !epilog_.empty() ) {
        strm << endl ;
        format_text(strm, epilog_, line_length) ;
    }
}

uint ArgumentParser::getOptimalColumnWidth(uint line_length, uint min_description_length)
{
    /* Find the maximum width of the option column */

    uint width = 23 ;
    for ( const Option &o: options_ ) {
        string ss = o.formatOptionFlags() ;
        width = std::max(width, static_cast<uint>(ss.size()));
    }

    const uint start_of_description_column = line_length - min_description_length;

    width = std::min(width, start_of_description_column - 1);

    /* add an additional space to improve readability */
    ++width;
    return width;
}

void ArgumentParser::printOptions(ostream &strm, uint line_length, uint min_description_length)
{
    uint col_width = getOptimalColumnWidth(line_length, min_description_length) ;

    if (!options_caption_.empty())
        strm << options_caption_ << ":\n";
    for( const Option &o: options_ ) {
        o.printDescription(strm, col_width, line_length);
        strm << '\n' ;
    }
}

void ArgumentParser::consumeArg(Option &a, const std::string &val, const std::vector<std::string> &argv, size_t &c) {
    a.matched_ = true ;

    if ( !a.value_ ) { ++c ; }
    else if ( val.empty() ) { // the value is specified after the flag
        ++c ;
        bool hasArg = ( c < argv.size() && !startsWith(argv[c], "-") ) ;

        if ( hasArg ) {
            if ( !a.value_->read(argv[c], a.value_parser_) )
                throw IncorrectArguments(*this, a.short_flag_) ;
            else c++ ;
        }
        else if ( !a.implicit_value_.empty() )  {
            a.value_->read(a.implicit_value_, a.value_parser_) ;
        }
        else {
            throw IncorrectArguments(*this, a.short_flag_) ;
        }

     } else {
        if ( !a.value_->read(val, a.value_parser_) )
            throw IncorrectArguments(*this, a.short_flag_) ;
        c++ ;
    }
    /*
    for(const auto &val: a.values_ ) {
        if ( !val->read(c, argv) )
            throw IncorrectArguments(*this, a.short_flag_) ;
    }
    */
}

void ArgumentParser::consumePositionalArg(Positional &a, const std::vector<std::string> &argv, size_t &c) {
    a.matched_ = true ;
/*
    for(const auto &val: a.values_ ) {
        if ( !val->read(c, argv) )
            throw IncorrectArguments(*this, "") ;
    }
    */
}


ArgumentParser::OptionsContainer::iterator ArgumentParser::findMatchingArg(const string &arg, string &val) {
    return std::find_if(options_.begin(), options_.end(), [&arg, &val] ( const OptionsContainer::value_type &t) { return t.matches(arg, val) ;} ) ;
}


void ArgumentParser::setDefaults()
{
    /*   for( auto &&o: options_ ) {
        if ( !o.matched_ && !o.default_value_.empty() ) {
            stringstream strm(o.default_value_) ;
            if ( !o.value_->read(strm) )  ;
        }
    }
    */
}

void ArgumentParser::checkRequired()
{
    for( auto &&o: options_ ) {
        if ( o.is_required_ && !o.matched_ ) throw MissingRequiredOption(*this, o.short_flag_) ;
    }

}

string ArgumentParser::Option::formatOptionFlags() const
{

    stringstream ss ;
    ss << "  " << join(flags_, "|") ;

    ss << ' ' << name_ ;

    string first_column = ss.str() ;

    return first_column ;
}

void ArgumentParser::Option::printDescription(ostream &strm, uint first_column_width, uint line_length) const
{
    string first_column = formatOptionFlags() ;
    strm << first_column ;

    if ( !description_.empty() ) {
        if ( first_column.size() >= first_column_width ) {
            strm.put('\n');
            for ( uint pad = first_column_width ; pad > 0; --pad ) strm.put(' ') ;
        } else {
            for( uint pad = first_column_width - static_cast<uint>(first_column.size()); pad > 0; --pad ) strm.put(' ') ;
        }

        format_description(strm, description_, first_column_width, line_length);
    }
}

ArgumentParser::Option::Option(const string &flags, const std::string &desc):
    is_required_(false), matched_(false), is_positional_(false), description_(desc)
{
    assert(!flags.empty()) ;

    auto pos = flags.find_first_of(" \t") ;

    string flags_part ;
    if ( pos != std::string::npos ) {
        flags_part = flags.substr(0, pos) ;
        name_ = flags.substr(pos+1) ;
    }
    else flags_part = flags ;

    assert(!flags_part.empty()) ;
    flags_ = split(flags_part, "|,") ;

    short_flag_ = flags_[0] ;
    for(const string &f : flags_) {
        if ( startsWith(f, "--") ) continue ;
        else if ( f.at(0) == '-' )
            short_flag_ = f ;
    }
}

/*
ArgumentParser::Option::Option(const string &flags, std::shared_ptr<Value> val): value_(val), min_args_(1), max_args_(1),
    is_required_(false), matched_(false), is_positional_(false)
{
    name_ = "<arg>" ;
    flags_ = split(flags, "|,") ;
    for(const string &f : flags_) {
        if ( startsWith(f, "--") ) continue ;
        else if ( f.at(0) == '-' )
            short_flag_ = f ;
    }
}

ArgumentParser::Option::Option(std::shared_ptr<Value> val): value_(val), min_args_(1), max_args_(1),
    is_required_(false), matched_(false), is_positional_(true)
{
    name_ = "<arg>" ;
}
*/

bool ArgumentParser::Option::matches(const string &arg, string &val) const {
    if ( !short_flag_.empty() ) { // short flag supported
        if ( arg.size() >= 2 ) { // test if short flag is given
            if ( arg.compare(0, 2, short_flag_) == 0 ) {
                val = arg.substr(2) ;
                return true ;
            }
        }
    }

    for ( const auto &flag: flags_ ) {
        if ( startsWith(flag, "--") ) {
            if ( arg.compare(0, flag.size(), flag ) == 0 ) {
                if ( arg.size() > flag.size() && arg[flag.size()] == '=' ) { // parse argument
                    val = arg.substr(flag.size()+1) ;
                }
                return true ;
            }
        }
    }


    return false ;
}
}

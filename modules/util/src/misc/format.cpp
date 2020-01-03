#include <cvx/util/misc/format.hpp>

using namespace std ;

namespace cvx { namespace util {

static detail::FormatPart parse_literal(const char *&p) {
    detail::FormatPart part ;
    part.type_ = detail::FormatPart::LITERAL ;
    part.begin_ = p ;

    while ( *p != 0 ) {
        if ( *p == '{' ) {
            if ( *(p+1) != '{' ) {
                 break ;
            } else {
                part.end_ = ++p;
                ++p ;
                return part ;
            }
        } else ++p;
    }

    part.end_ = p ;

    return part ;
}

static size_t parse_integer(const char *&p) {
    size_t val = 0 ;
    for ( ; *p != 0 && isdigit(*p) ; ++p ) {
        val = val * 10 + static_cast<size_t>(*p - '0') ;
    }
    return val ;
}

static bool parse_align_flag(const char *p,  detail::FormatPart &part) {
    switch (*p) {
    case '<' :
        part.align_ = detail::FormatPart::ALIGN_LEFT ; return true;
    case '>' :
        part.align_ = detail::FormatPart::ALIGN_RIGHT ; return true;
    case '^':
        part.align_ = detail::FormatPart::ALIGN_CENTER ; return true;
    default:
        return false ;
    }
}

static void parse_fill_align_flag(const char *&p,  detail::FormatPart &part) {
    if ( parse_align_flag(p, part) ) ++p ;
    else {
        char fill = *p ;
        if ( parse_align_flag(p+1, part) ) {
            part.fill_char_ = fill ;
            ++p ; ++p ;
        }
    }
}

static void parse_sign(const char *&p,  detail::FormatPart &part) {
    switch (*p) {
    case '+':
        part.sign_ = detail::FormatPart::SIGN_BOTH ; ++p ; return ;
    case '-':
        part.sign_ = detail::FormatPart::SIGN_NEGATIVE_ONLY ; ++p ; return ;
    case ' ':
        part.sign_ = detail::FormatPart::SIGN_NEGATIVE_PAD ; ++p ; return ;
    }
}

static void parse_alt_form(const char *&p,  detail::FormatPart &part) {
    if ( *p == '#' ) {
        part.alt_form_ = true ;
        ++p ;
    }
}

static void parse_zero_padding(const char *&p,  detail::FormatPart &part) {
    if ( *p == '0' ) {
        part.zero_padding_ = true ;
        ++p ;
    }
}

static void parse_type(const char *&p,  detail::FormatPart &part) {
    switch (*p) {
    case 'd':
        ++p ; part.type_ = detail::FormatPart::INT ;
        break ;
    case 'B':
        part.uppercase_ = true ;
    case 'b':
        ++p ; part.type_ = detail::FormatPart::BIN ; break ;
    case 'o':
        ++p ; part.type_ = detail::FormatPart::OCT ;
        break ;
    case 'X':
        part.uppercase_ = true ;
    case 'x':
        part.type_ = detail::FormatPart::HEX ; ++p ;
        break ;
    case 'E':
        part.uppercase_ = true ;
    case 'e':
        part.type_ = detail::FormatPart::FLE ; ++p ;
        break ;
    case 'G':
        part.uppercase_ = true ;
    case 'g':
        part.type_ = detail::FormatPart::FLG ; ++p ;
        break ;
    case 'F':
        part.uppercase_ = true ;
    case 'f':
        part.type_ = detail::FormatPart::FLF ; ++p ;
        break ;
    case 'c':
        part.type_ = detail::FormatPart::CHR ; ++p ;
        break ;
    case 's':
        part.type_ = detail::FormatPart::STR ; ++p ;
        break ;
    case 'p':
        part.type_ = detail::FormatPart::PTR ; ++p ;
        break ;
    }
}


static bool parse_format_spec(const char *&p,  detail::FormatPart &part) {

    if ( *p == 0 ) return false ;

    parse_fill_align_flag(p, part) ;
    parse_sign(p, part) ;
    parse_alt_form(p, part) ;
    parse_zero_padding(p, part) ;
    if ( isdigit(*p) )
        part.width_ = parse_integer(p) ;
    if ( *p == '.' )
        part.precision_ = parse_integer(p) ;
    parse_type(p, part) ;

    return true ;
}

static detail::FormatPart parse_part(const char *&p) {
    detail::FormatPart part ;
    part.begin_ = p ;
    ++p ;
    bool width_parsed = false ;

    if ( isdigit(*p) ) { // positional argument
        part.arg_ = parse_integer(p) ;
    }

    if ( *p == ':' ) {
        ++p ; parse_format_spec(p, part) ;
    }

    return part ;

}

void Format::parse() {
    const char *p = fmt_ ;

    while( *p != 0 ) {
        if ( *p == '{' && *(p+1) != '{' ) {
            detail::FormatPart current = parse_part(p) ;
            parts_.emplace_back(std::move(current)) ;
            if ( *p == '}' ) {
                if ( *(p+1) == '}' ) {
                    detail::FormatPart current = parse_literal(p) ;
                    parts_.emplace_back(std::move(current)) ;
                }
                else ++p ;
            } else {
                throw FormatParseException("") ;
            }
        } else {
            detail::FormatPart current = parse_literal(p) ;
            parts_.emplace_back(std::move(current)) ;
        }
    }
}


using detail::FormatArg ;
using detail::FormatPart ;

static void format_float(ostream &strm, const FormatPart &part, double val) {
    switch ( part.type_ ) {
    case FormatPart::COPY:
       strm << val ;
        break ;
    case FormatPart::FLF:
          strm.setf(std::ios::fixed, std::ios::floatfield);
          strm << val ;
        break ;
    case FormatPart::FLG:
        strm.setf(strm.flags() & ~std::ios::floatfield);
        strm << val ;
        break ;
    case FormatPart::FLE:
         strm.setf(std::ios::scientific, std::ios::floatfield);
         strm << val ;
          break ;

     default:
        throw FormatArgException("") ;
    }
}

static void format_string(ostream &strm, const FormatPart &part, const char *val) {
    switch ( part.type_ ) {
    case FormatPart::COPY:
    case FormatPart::STR:
        strm << val ;
        break ;
    default:
        throw FormatArgException("") ;
    }
}


static void format_integer(ostream &strm, const FormatPart &part, int64_t val) {
    switch ( part.type_ ) {
    case FormatPart::BIN:
       break ;
    case FormatPart::HEX:
        strm.setf(std::ios::hex, std::ios::basefield);
        strm << val ;
        break ;
    case FormatPart::INT:
        strm.setf(std::ios::dec, std::ios::basefield);
        strm << val ;
        break ;
    case FormatPart::OCT:
        strm.setf(std::ios::oct, std::ios::basefield);
        strm << val ;
        break ;
    default:
       throw FormatArgException("") ;
    }
}

static void format_boolean(ostream &strm, const FormatPart &part, bool val) {
    switch ( part.type_ ) {
    case FormatPart::COPY:
    case FormatPart::STR:
        strm << ((val) ? "true" : "false");
        break ;
    case FormatPart::BIN:
    case FormatPart::HEX:
    case FormatPart::CHR:
    case FormatPart::INT:
    case FormatPart::OCT:
        format_integer(strm, part, static_cast<unsigned int>(val));
        break ;
     default:
        throw FormatArgException("") ;
    }
}

static void format_pointer(ostream &strm, const FormatPart &part, const void *val) {
    switch ( part.type_ ) {
    case FormatPart::COPY:
    case FormatPart::PTR:
        strm.setf(std::ios::hex, std::ios::basefield);
        strm << reinterpret_cast<uintptr_t>(val) ;
        break ;
    default:
        throw FormatArgException("") ;
    }
}

static void format_char(ostream &strm, const FormatPart &part, char val) {
    switch ( part.type_ ) {
    case FormatPart::COPY:
    case FormatPart::CHR:
        strm << val;
        break ;
    case FormatPart::BIN:
    case FormatPart::HEX:
    case FormatPart::INT:
    case FormatPart::OCT:
        format_integer(strm, part, static_cast<unsigned int>(val));
        break ;
     default:
        throw FormatArgException("") ;
    }
}




Format::Format(const char *fmt): fmt_(fmt) {
    parse() ;
}


void Format::format(std::ostream &strm, FormatArg args[], size_t n_args) const
{
    int carg = 0 ;
    for( const detail::FormatPart &part: parts_ ) {
        if ( part.type_ == detail::FormatPart::LITERAL ) {
            strm.write(part.begin_, part.end_ - part.begin_) ;
        } else {
            int arg = ( part.arg_ == -1 ) ? carg++ : part.arg_ ;

            if ( arg < n_args ) {

                // Saved stream state
                std::streamsize origWidth = strm.width();
                std::streamsize origPrecision = strm.precision();
                std::ios::fmtflags origFlags = strm.flags();
                char origFill = strm.fill();

                if ( part.precision_ >= 0 )
                    strm.precision(part.precision_) ;

                if ( part.width_ >= 0 )
                    strm.width(part.width_) ;

                if ( part.flag_ & detail::FormatPart::FLAG_PREPEND_ZEROS ) {
                    strm.fill('0');
                    strm.setf(std::ios::internal, std::ios::adjustfield);
                }

                if ( part.flag_ & detail::FormatPart::FLAG_LEFT_ALIGN ) {
                    strm.fill(' ');
                    strm.setf(std::ios::left, std::ios::adjustfield);
                }

                if ( part.flag_ & detail::FormatPart::FLAG_HASH ) {
                    strm.setf(std::ios::showpoint | std::ios::showbase);
                }

                if ( part.flag_ & detail::FormatPart::FLAG_PREPEND_PLUS ) {
                    strm.setf(std::ios::showpos);
                }

                if ( part.uppercase_ )
                    strm.setf(std::ios::uppercase);

                const FormatArg &v = args[arg] ;

                switch (v.tag_ ) {
                    case FormatArg::Type::Float:
                        format_float(strm, part, v.f_) ;
                        break ;
                    case FormatArg::Type::UnsignedInteger:
                        format_integer(strm, part, v.u_) ;
                        break ;
                    case FormatArg::Type::SignedInteger:
                        format_integer(strm, part, v.i_);
                        break ;
                    case FormatArg::Type::Boolean:
                        format_boolean(strm, part, v.b_);
                        break ;
                    case FormatArg::Type::String:
                        format_string(strm, part, v.s_);
                        break ;
                    case FormatArg::Type::Char:
                        format_char(strm, part, v.c_);
                        break ;
                    case FormatArg::Type::Pointer:
                        format_pointer(strm, part, v.p_);
                        break ;
                }

                strm.width(origWidth);
                strm.precision(origPrecision);
                strm.flags(origFlags);
                strm.fill(origFill);


            }

        }
    }

}









}}

#include <cvx/util/misc/format.hpp>

namespace cvx { namespace util {

static bool parse_literal(const char *&p, detail::FormatPart &part) {
    part.type_ = detail::FormatPart::LITERAL ;
    part.begin_ = p ;

    while ( *p != 0 ) {
        if ( *p == '%' ) {
            if ( *(p+1) != '%' ) {
                 break ;
            } else {
                part.end_ = ++p;
                ++p ;
                return true ;
            }
        } else ++p;
    }

    part.end_ = p ;

    return true ;
}

static size_t parse_integer(const char *&p) {
    size_t val = 0 ;
    for ( ; *p != 0 && isdigit(*p) ; ++p ) {
        val = val * 10 + static_cast<size_t>(*p - '0') ;
    }
    return val ;
}

static bool parse_part(const char *&p, detail::FormatPart &part) {
    part.begin_ = p ;
    ++p ;
    bool width_parsed = false ;

    if ( isdigit(*p) ) { // fill, width or positional argument
        if ( *p == '0' ) {
            part.flag_ = detail::FormatPart::FLAG_PREPEND_ZEROS ;
            ++p ;

            if ( isdigit(*p) ) {
                int w = parse_integer(p) ;
                part.width_ = w ;
                width_parsed = true ;
            }
        } else {
            int value = parse_integer(p) ;

            if ( value != 0 ) {
                if ( *p == '$' ) { // positional argument
                    part.arg_ = value - 1 ;
                    ++p ;
                }
                else {
                    part.width_ = value ;
                    width_parsed = true ;
                }
            } else {
                return false ;
            }
        }
    }

    if ( !width_parsed ) {
        while (*p != 0 ) {
            switch (*p) {
            case '#':
                ++p ;
                part.flag_ |= detail::FormatPart::FLAG_HASH ;
                continue ;
            case '0':
                ++p ;
                part.flag_ |= detail::FormatPart::FLAG_PREPEND_ZEROS ;
                continue ;
            case '-':
                ++p ;
                part.flag_ |= detail::FormatPart::FLAG_LEFT_ALIGN ;
                continue ;
            case ' ':
                ++p ;
                part.flag_ |= detail::FormatPart::FLAG_PREPEND_SPACE ;
                continue ;
            case '+':
                part.flag_ |= detail::FormatPart::FLAG_PREPEND_PLUS ;
                ++p ;
                continue ;
            default:
                break;
            }
            break ;
        }

        if ( isdigit(*p) ) { // parse width (we do not support width indirections)
            part.width_ = parse_integer(p) ;
            width_parsed = true ;
        }
    }

    if ( *p == '.' ) {
        ++p ;
        if ( isdigit(*p) )
            part.precision_ = parse_integer(p) ;
    }

    switch (*p) {
    case 'l': case 'L': case 'j': case 'Z': case 'h': case 't':
        ++p ;
        break ;
    }

    switch (*p) {
    case 'u': case 'd': case 'i':
        ++p ; part.type_ = detail::FormatPart::INT ;
        break ;
    case 'o':
        ++p ; part.type_ = detail::FormatPart::OCT ;
        break ;
    case 'X':
        part.uppercase_ = true ;
    case 'x': case 'p':
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
    default:
        return false ;

    }

    return true ;

}

void Format::parse() {
    const char *p = fmt_ ;

    detail::FormatPart current ;

    while( *p != 0 ) {
        if ( *p == '%' && *(p+1) != '%' ) {
            if ( !parse_part(p, current) ) {
                throw FormatParseException("") ;
            }
        } else {
            parse_literal(p, current) ;
        }

        parts_.emplace_back(std::move(current)) ;
    }
}

Format::Format(const char *fmt): fmt_(fmt) {
    parse() ;
}

void Format::format(std::ostream &strm, Variant args[], size_t n_args) const
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

                const Variant &v = args[arg] ;
                if ( part.type_ == detail::FormatPart::STR ) {
                    strm << v.toString() ;
                } else if ( part.type_ == detail::FormatPart::FLF ) {
                    strm.setf(std::ios::fixed, std::ios::floatfield);
                    strm << v.toFloat() ;
                } else if ( part.type_ == detail::FormatPart::FLE ) {
                    strm.setf(std::ios::scientific, std::ios::floatfield);
                    strm << v.toFloat() ;
                } else if ( part.type_ == detail::FormatPart::FLG ) {
                    strm.setf(strm.flags() & ~std::ios::floatfield);
                    strm << v.toFloat() ;
                } else if ( part.type_ == detail::FormatPart::INT ) {
                    strm.setf(std::ios::dec, std::ios::basefield);
                    strm << v.toSignedInteger() ;
                } else if ( part.type_ == detail::FormatPart::OCT ) {
                    strm.setf(std::ios::oct, std::ios::basefield);
                    strm << v.toUnsignedInteger() ;
                } else if ( part.type_ == detail::FormatPart::HEX ) {
                    strm.setf(std::ios::hex, std::ios::basefield);
                    strm << v.toUnsignedInteger() ;
                } else if ( part.type_ == detail::FormatPart::CHR ) {
                    std::string val = v.toString() ;
                    if ( !val.empty() ) {
                        strm.fill(' ');
                        strm << val[0] ;
                    }
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

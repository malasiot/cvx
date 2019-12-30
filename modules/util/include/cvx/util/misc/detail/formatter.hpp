#ifndef CVX_UTIL_MISC_DETAIL_FORMAT_PART_HPP
#define  CVX_UTIL_MISC_DETAIL_FORMAT_PART_HPP

namespace cvx { namespace util {

namespace detail {

struct FormatPart {
public:

    enum FormatterType {
        FLF, FLE, FLG, INT, OCT, HEX, BIN, STR, CHR, LITERAL
    } ;

    enum Flag {
        FLAG_PREPEND_ZEROS = 0x1, FLAG_LEFT_ALIGN = 0x2, FLAG_PREPEND_PLUS = 0x4,
        FLAG_PREPEND_SPACE = 0x8, FLAG_HASH = 0x10a  } ;

    int arg_ = -1 ;
    int flag_ = 0 ;
    char fill_char_ = ' ';
    int width_ = 0 ;
    int precision_ = -1 ;
    bool uppercase_ = false ;
    FormatterType type_ ;
    const char *begin_, *end_ ;
};



}}}
#endif

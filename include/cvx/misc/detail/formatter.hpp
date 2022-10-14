#ifndef CVX_UTIL_MISC_DETAIL_FORMAT_PART_HPP
#define  CVX_UTIL_MISC_DETAIL_FORMAT_PART_HPP

#include <iostream>

namespace cvx {

namespace detail {

struct FormatPart {
public:

    enum FormatterType {
        FLF, FLE, FLG, INT, OCT, HEX, BIN, STR, CHR, LITERAL, COPY, PTR
    } ;

    enum Align {
        ALIGN_LEFT, ALIGN_RIGHT, ALIGN_DEFAULT, PAD_AFTER_SIGN
    } ;

    enum Sign {
        SIGN_BOTH, SIGN_NEGATIVE_ONLY, SIGN_NEGATIVE_PAD
    };

    enum Flag {
        FLAG_PREPEND_ZEROS = 0x1, FLAG_LEFT_ALIGN = 0x2, FLAG_PREPEND_PLUS = 0x4,
        FLAG_PREPEND_SPACE = 0x8, FLAG_HASH = 0x10a  } ;

    Align align_ = ALIGN_DEFAULT ;
    Sign sign_ = SIGN_NEGATIVE_ONLY;
    bool alt_form_ = false ;
    bool zero_padding_ = false ;
    int arg_ = -1 ;
    int flag_ = 0 ;
    char fill_char_ = ' ';
    int width_ = 0 ;
    int precision_ = -1 ;
    bool uppercase_ = false ;
    FormatterType type_ = COPY ;
    const char *begin_, *end_ ;
};

struct FormatArg {
public:
    enum class Type : uint8_t {
        Boolean, Char, String, SignedInteger, UnsignedInteger, Float, Double, Pointer
    };

    FormatArg(bool b): b_(b), tag_(Type::Boolean) {}
    FormatArg(char c): c_(c), tag_(Type::Char) {}
    FormatArg(int i): i_(i), tag_(Type::SignedInteger) {}
    FormatArg(uint i): u_(i), tag_(Type::UnsignedInteger) {}
    FormatArg(short i): i_(i), tag_(Type::SignedInteger) {}
    FormatArg(unsigned short i): u_(i), tag_(Type::UnsignedInteger) {}
    FormatArg(long i): i_(i), tag_(Type::SignedInteger) {}
    FormatArg(unsigned long i): u_(i), tag_(Type::UnsignedInteger) {}
    FormatArg(long long i): i_(i), tag_(Type::SignedInteger) {}
    FormatArg(unsigned long long i): u_(i), tag_(Type::UnsignedInteger) {}
    FormatArg(float f): f_(f), tag_(Type::Float) {}
    FormatArg(double d): d_(d), tag_(Type::Double) {}
    FormatArg(const char *s): s_(s), tag_(Type::String) {}
    FormatArg(const std::string &s): s_(s.c_str()), tag_(Type::String) {}
    FormatArg(const void *p): p_(p), tag_(Type::Pointer) {}

    union {
        bool b_ ;
        char c_ ;
        const char *s_ ;
        int64_t i_ ;
        uint64_t u_ ;
        float f_ ;
        double d_;
        const void *p_ ;
    } ;

    Type tag_ ;
};

}}
#endif

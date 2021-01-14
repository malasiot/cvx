#ifndef CVX_UTIL_BASE64_HPP
#define CVX_UTIL_BASE64_HPP

#include <string>

namespace cvx {

// encode string to base64
std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

}

#endif

#ifndef CVX_UTIL_OPTIONAL_HPP
#define CVX_UTIL_OPTIONAL_HPP

/* just wrapping 3rdparty implementation of optional to support c++11 */
/* it fallbacks to std::optional for c++17 and above */

#include <cvx/misc/detail/optional.hpp>

namespace cvx {
    using nonstd::optional ;
}

#endif

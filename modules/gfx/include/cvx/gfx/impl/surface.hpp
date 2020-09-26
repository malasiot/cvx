#ifndef CVX_GFX_DETAIL_SURFACE_HPP
#define CVX_GFX_DETAIL_SURFACE_HPP

#include <cairo/cairo.h>

#include <cvx/gfx/image.hpp>

namespace cvx { namespace gfx {
namespace detail {

class Surface {
public:

    cairo_surface_t *surf_ = nullptr ;
    double dpix_, dpiy_ ;
};

}
}}
#endif

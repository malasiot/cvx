#ifndef CVX_GFX_CAIRO_BACKEND_HPP
#define CVX_GFX_CAIRO_BACKEND_HPP

#include <cvx/gfx/bbox.hpp>

#include <cairo/cairo.h>
#include <stack>
#include <memory>

#include <cvx/gfx/font.hpp>
#include <cvx/gfx/pen.hpp>
#include <cvx/gfx/brush.hpp>
#include <cvx/gfx/path.hpp>

namespace cvx { namespace gfx {
namespace detail {

class Backend {
public:
    cairo_t *source_cr_ = nullptr, *cr_ = nullptr;
    cairo_surface_t *surf_ = nullptr , *proxy_surf_ = nullptr;
    std::shared_ptr<Canvas> mask_ ;

    Backend() ;
    ~Backend() ;

    struct State {
         State() ;

         std::shared_ptr<Pen> pen_ ;
         std::shared_ptr<Brush> brush_ ;

         Font font_ ;
         Matrix2d trans_ ;
     };

    std::stack<State> state_ ;

protected:

    void init() ;
    void flush() ;
    void set_cairo_stroke(const Pen &pen) ;
    void cairo_apply_linear_gradient(const LinearGradientBrush &lg);
    void cairo_apply_radial_gradient(const RadialGradientBrush &rg);
    void cairo_apply_pattern(const PatternBrush &pat);
    void fill_stroke_shape();
    void set_cairo_fill(const std::shared_ptr<Brush> &br);
    void line_path(double x0, double y0, double x1, double y1) ;
    void rect_path(double x0, double y9, double w, double h) ;
    void path(const Path &path) ;
    void polyline_path(double *pts, int n, bool) ;
    cairo_t *cr() ;

} ;



} // namespace detail
} // namespace gfx ;
} // namespace cvx

#endif

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
#include <cvx/gfx/surface.hpp>

namespace cvx { namespace gfx {
namespace detail {

struct State {
     State() ;
     State(const State &other):
            pen_(other.pen_->clone()),
            brush_(other.brush_->clone()),
            font_(other.font_),
            trans_(other.trans_)
           {}

     std::unique_ptr<PenBase> pen_;
     std::unique_ptr<BrushBase> brush_ ;

     Font font_ ;
     Matrix2d trans_ ;
 };

class RenderingContext {
public:

    RenderingContext() ;
    ~RenderingContext() ;

    cairo_t *cr_ = nullptr;
    std::stack<State> state_ ;

protected:

    void set_cairo_stroke(const Pen &pen) ;
    void cairo_apply_linear_gradient(const LinearGradientBrush &lg);
    void cairo_apply_radial_gradient(const RadialGradientBrush &rg);
    void cairo_apply_pattern(const PatternBrush &pat);
    void fill_stroke_shape();
    void set_cairo_fill(const Brush *br);
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

#ifndef CVX_GFX_CANVAS_HPP
#define CVX_GFX_CANVAS_HPP

#include <memory>

#include <string>
#include <cvx/gfx/pen.hpp>
#include <cvx/gfx/brush.hpp>
#include <cvx/gfx/font.hpp>
#include <cvx/gfx/xform.hpp>
#include <cvx/gfx/path.hpp>
#include <cvx/gfx/font.hpp>
#include <cvx/gfx/surface.hpp>

#include <cvx/gfx/image.hpp>
#include <cvx/gfx/rectangle.hpp>
#include <cvx/gfx/glyph.hpp>
#include <cvx/gfx/text_layout.hpp>
#include <cvx/gfx/svg_document.hpp>

#include <cvx/gfx/backends/cairo/canvas.hpp>

namespace cvx { namespace gfx {

enum TextAlignFlags {
    TextAlignLeft = 0x01, TextAlignRight = 0x02, TextAlignTop = 0x04, TextAlignBottom = 0x08, TextAlignHCenter = 0x10, TextAlignVCenter = 0x20, TextAlignBaseline = 0x40
}  ;

enum BlendMode {
    CLEAR, SRC, SRC_OVER, SRC_IN, SRC_OUT, SRC_ATOP, DST, DST_OVER, DST_IN, DST_OUT, DST_ATOP, XOR, ADD,
    SATURATE, MULTIPLY, SCREEN, OVERLAY, DARKEN, LIGHTEN, DODGE, BURN, HARD_LIGHT, SOFT_LIGHT, DIFFERENCE,
    EXCLUSION, HSL_HUE, HSL_COLOR, HSL_LUMINOSITY
};

class StyledText ;

class Canvas: public detail::RenderingContext {
protected:
    Canvas(Canvas &&op) = delete ;
    Canvas& operator=(Canvas&& op) = delete ;
public:
   ~Canvas() ;
    Canvas(Surface &surface) ;

    const Surface &surface() const { return surface_ ; }

    void save() ;
    void restore() ;

    void setTransform(const Matrix2d &tr) ;

    void setPen(const PenBase &pen) ;
    void setBrush(const BrushBase &brush) ;
    void setFont(const Font &font) ;
    void setBlendMode(BlendMode mode) ;
    void setTextAlign(int flags) ;
    void setTextDirection(TextDirection dir) ;

    void setAntialias(bool antiAlias = true) ;

    void setClipRect(double x0, double y0, double w, double h) ;
    void setClipRect(const Rectangle2d &r) ;
    void setClipPath(const Path &p, FillRule frule= FillRule::EvenOdd) ;

    void drawLine(double x0, double y0, double x1, double y1) ;
    void drawLine(const Point2d &p1, const Point2d &p2) ;
    void drawRect(double x0, double y0, double w, double h) ;
    void drawRect(const Rectangle2d &r) ;

    void drawPath(const Path &path) ;

    void drawPolyline(double *pts, int nPts) ;
    void drawPolygon(double *pts, int nPts) ;
    void drawCircle(double cx, double cy, double r) ;
    void drawCircle(const Point2d &center, double r) ;
    void drawEllipse(double xp, double yp, double ax, double ay) ;

    void drawText(const std::string &textStr, double x0, double y0) ;
    void drawText(const std::string &textStr, double x0, double y0, double width, double height) ;
    void drawText(const std::string &textStr, const Point2d &p) ;
    void drawText(const std::string &textStr, const Rectangle2d &r) ;

    void drawGlyph(const Glyph &g, const Point2d &p) ;
    void drawGlyphs(const std::vector<Glyph> &glyphs, const std::vector<Point2d> &positions) ;

    void drawImage(const Image &im,  double opacity) ;

    void drawSVG(const SVGDocument &doc) ;

    void fill(const Color &color) ;

protected:
    Surface &surface_ ;
} ;

}}

#endif

#ifndef CVX_GFX_PEN_HPP
#define CVX_GFX_PEN_HPP

#include <cvx/gfx/color.hpp>
#include <vector>
#include <memory>

namespace cvx { namespace gfx {

enum class LineJoin { Miter, Round, Bevel } ;
enum class LineCap { Butt, Round, Square } ;
enum LineStyle { SolidLine, DashLine, DotLine, DashDotLine, CustomDashLine } ;

class PenBase {

public:

    virtual std::unique_ptr<PenBase> clone() const = 0 ;

} ;

class Pen: public PenBase {

public:

    Pen( const Color &clr = Color(0, 0, 0), double line_width = 1.0, LineStyle style = SolidLine ) ;
    std::unique_ptr<PenBase> clone() const override { return std::unique_ptr<Pen>(new Pen(*this)) ; }

    Pen &setColor(const Color &brush) ;
    Pen &setLineWidth(double width) ;
    Pen &setMiterLimit(double limit) ;
    Pen &setLineJoin(LineJoin join) ;
    Pen &setLineCap(LineCap cap) ;
    Pen &setDashArray(const std::vector<double> &) ;
    Pen &setDashOffset(double offset) ;
    Pen &setLineStyle(LineStyle dash) ;

    LineJoin lineJoin() const { return line_join_ ; }
    LineCap lineCap() const { return line_cap_ ; }
    LineStyle lineStyle() const { return line_style_ ; }
    Color lineColor() const { return line_color_ ; }
    double lineWidth() const { return line_width_ ; }
    double miterLimit() const { return miter_limit_ ; }
    const std::vector<double> &dashArray() const { return dash_array_ ; }
    double dashOffset() const { return dash_offset_ ; }

private:

    double line_width_, miter_limit_ = 0 ;
    LineJoin line_join_ = LineJoin::Round ;
    LineCap line_cap_ = LineCap::Round ;
    LineStyle line_style_ = SolidLine ;
    std::vector<double> dash_array_ ;
    double dash_offset_ = 0;
    Color line_color_ ;
} ;

class EmptyPen: public PenBase {
public:
    EmptyPen() {}
    std::unique_ptr<PenBase> clone() const override { return std::unique_ptr<PenBase>(new EmptyPen(*this)) ; }
};


} }

#endif

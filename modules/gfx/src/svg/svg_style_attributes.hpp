#ifndef CVX_GFX_SVG_STYLE_ATTRIBUTES_HPP
#define CVX_GFX_SVG_STYLE_ATTRIBUTES_HPP

#include "svg_length.hpp"
#include "svg_dom_exceptions.hpp"
#include "svg_parse_util.hpp"

#include <cvx/gfx/color.hpp>

namespace svg {

enum class FillRule { EvenOdd, NonZero } ;
enum class ClipRule { EvenOdd, NonZero } ;
enum class LineJoinType { Miter, Round, Bevel } ;
enum class LineCapType { Butt, Round, Square } ;
enum class FontStyle { Normal, Oblique, Italic } ;
enum class FontVariant { Normal, SmallCaps } ;
enum class FontWeight { Normal, Bold, Bolder, Lighter,
                        W100, W200, W300, W400, W500, W600, W700, W800, W900 } ;
enum class FontStretch { Normal, UltraCondensed, ExtraCondensed, Condensed,
                         Narrower, SemiCondensed, SemiExpanded, Expanded,
                         Wider, ExtraExpanded, UltraExpanded } ;
enum class FontSizeType { Length, XXSmall, XSmall, Small, Medium, Large, XLarge, XXLarge, Larger, Smaller } ;

enum class TextDecoration { None, Underline, Overline, Strike } ;
enum class TextAnchor { Start, Middle, End } ;
enum class ShapeQuality { Auto, OptimizeSpeed, CrispEdges, GeometricPrecision } ;
enum class TextQuality { Auto, OptimizeSpeed, OptimizeLegibility, GeometricPrecision } ;

enum class DisplayMode { None, Inline, BBox } ;
enum class VisibilityMode { Visible, Hidden, Collapse } ;

enum class PaintType { None, SolidColor, CurrentColor, PaintServer } ;
enum class OverflowType { Visible, Hidden, Scroll, Auto } ;

struct FontSize {

    FontSize(FontSizeType type): type_(type) {}
    FontSize(Length l): val_(l), type_(FontSizeType::Length) {}

    bool parse(const std::string &v) ;
    FontSizeType type_ ;
    Length val_ ;
protected:
    friend class Style ;
    FontSize() = default ;
};

class URI {
public:
    URI() = default ;

    bool parse(const std::string &v) {
        id_ = parse_uri(v) ;
        return !id_.empty() ;
    }

    const std::string id() const { return id_ ; }

protected:

    std::string id_ ;

} ;

using cvx::gfx::CSSColor ;

struct Paint {
    Paint() = default ;
    Paint(PaintType t): type_(t), color_{0, 0, 0} {}
    Paint(const std::string &paint_id): type_(PaintType::PaintServer), server_id_(paint_id) {}
    Paint(const CSSColor &clr): type_(PaintType::SolidColor), color_(clr) {}

    bool parse(const std::string &val) ;

    PaintType type() const { return type_ ; }

    const cvx::gfx::CSSColor &color() const { return color_ ; }
    const std::string serverId() const { return server_id_ ; }

protected:

    PaintType type_ = PaintType::None ;
    std::string server_id_ ;
    cvx::gfx::CSSColor color_ = {0, 0, 0} ;
} ;

struct FillPaint: public Paint {
    FillPaint() = default ;
    FillPaint(const std::string &id): Paint(id) {}
    FillPaint(const CSSColor &clr): Paint(clr) {}
} ;

struct StrokePaint: public Paint {
    StrokePaint() = default ;
    StrokePaint(const std::string &id): Paint(id) {}
    StrokePaint(const CSSColor &clr): Paint(clr) {}
} ;

#define SVG_STYLE_ATTRIBUTE(aname, atype, avar, adef)\
    protected:\
    std::shared_ptr<atype> avar ;\
    public:\
    void set##aname(const atype &v) {\
    if ( !avar ) avar.reset(new atype(v)) ;\
    else *avar = v ;\
}\
    atype get##aname() const {\
    if ( !avar ) return adef ;\
    else return *avar ;\
}\



}
#endif

#include <cvx/gfx/impl/canvas.hpp>
#include <cvx/gfx/canvas.hpp>

#include <cairo/cairo.h>
#include <cassert>
#include <cmath>
#include <regex>

#ifdef _WIN32
#include <cairo-win32.h>
#else
#include <cairo/cairo-ft.h>
#include <fontconfig/fcfreetype.h>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include "text_layout_engine.hpp"
#include "font_manager.hpp"

#endif

#include <mutex>
#include <iostream>

using namespace std;

namespace cvx { namespace gfx {
namespace detail {

State::State(): font_("Arial", 10), pen_(new EmptyPen), brush_(new EmptyBrush) {
    text_align_flags_ = TextAlignLeft | TextAlignTop ;
    text_direction_ = TextDirection::Auto ;
    text_decoration_ = TextDecorationNone ;
}


void RenderingContext::set_cairo_stroke(const Pen &pen) {

    const Color &clr = pen.lineColor() ;

    if ( clr.a()  == 1.0 ) cairo_set_source_rgb(cr(), clr.r(), clr.g(), clr.b()) ;
    else cairo_set_source_rgba(cr(), clr.r(), clr.g(), clr.b(), clr.a() ) ;

    cairo_set_line_width (cr(), pen.lineWidth());

    cairo_set_miter_limit (cr(), pen.miterLimit()) ;

    switch ( pen.lineCap() ) {
    case LineCap::Butt:
        cairo_set_line_cap(cr(), CAIRO_LINE_CAP_BUTT) ;
        break ;
    case LineCap::Round:
        cairo_set_line_cap(cr(), CAIRO_LINE_CAP_ROUND) ;
        break ;
    case LineCap::Square:
        cairo_set_line_cap(cr(), CAIRO_LINE_CAP_SQUARE) ;
        break ;
    }

    switch ( pen.lineJoin() ) {
    case LineJoin::Miter:
        cairo_set_line_join(cr(), CAIRO_LINE_JOIN_MITER) ;
        break ;
    case LineJoin::Round:
        cairo_set_line_join(cr(), CAIRO_LINE_JOIN_ROUND) ;
        break ;
    case LineJoin::Bevel:
        cairo_set_line_join(cr(), CAIRO_LINE_JOIN_BEVEL) ;
        break ;
    }

    double dash_offset = pen.dashOffset() ;

    const auto &dash_array = pen.dashArray() ;

    if ( dash_array.empty() )
        cairo_set_dash(cr(), 0, 0, 0) ;
    else
        cairo_set_dash(cr(), &dash_array[0], dash_array.size(), dash_offset) ;
}

void RenderingContext::cairo_apply_linear_gradient(const LinearGradientBrush &lg) {
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    double x0 = lg.x0(), y0 = lg.y0(), x1 = lg.x1(), y1 = lg.y1() ;

    pattern = cairo_pattern_create_linear ( x0, y0, x1, y1 ) ;

    Matrix2d tr = lg.transform() ;
    cairo_matrix_init (&matrix, tr.m1(), tr.m2(), tr.m3(), tr.m4(), tr.m5(), tr.m6());
    cairo_matrix_invert (&matrix);
    cairo_pattern_set_matrix (pattern, &matrix);

    if ( lg.spread() == SpreadMethod::Reflect )
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REFLECT);
    else if ( lg.spread() == SpreadMethod::Repeat )
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
    else
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

    for( const auto &stop: lg.stops() ) {
        const Color &clr = stop.clr_ ;
        cairo_pattern_add_color_stop_rgba (pattern, stop.offset_, clr.r(), clr.g(), clr.b(), clr.a() * lg.fillOpacity() );
    }

    cairo_set_source (cr(), pattern);
    cairo_pattern_destroy (pattern);
}

void RenderingContext::cairo_apply_radial_gradient(const RadialGradientBrush &rg) {
    cairo_pattern_t *pattern;
    cairo_matrix_t matrix;

    double fx = rg.fx(), fy=rg.fy(), cx=rg.cx(), cy=rg.cy(), r=rg.radius() ;

    pattern = cairo_pattern_create_radial ( fx,	fy, 0.0, cx, cy, r) ;

    Matrix2d tr = rg.transform() ;
    cairo_matrix_init (&matrix, tr.m1(), tr.m2(), tr.m3(), tr.m4(), tr.m5(), tr.m6());
    cairo_matrix_invert (&matrix);
    cairo_pattern_set_matrix (pattern, &matrix);

    if ( rg.spread() == SpreadMethod::Reflect )
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REFLECT);
    else if ( rg.spread() == SpreadMethod::Repeat )
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
    else
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

    for( const auto &stop: rg.stops() ) {
        const Color &clr = stop.clr_ ;
        cairo_pattern_add_color_stop_rgba (pattern, stop.offset_, clr.r(), clr.g(), clr.b(), clr.a() * rg.fillOpacity() );
    }

    cairo_set_source (cr(), pattern);
    cairo_pattern_destroy (pattern);
}

void RenderingContext::cairo_apply_pattern(const PatternBrush &pat) {

    const Surface &c = pat.pattern() ;

    cairo_pattern_t *pattern = cairo_pattern_create_for_surface (c.surf_);

    if ( pat.spread() == SpreadMethod::Reflect )
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REFLECT);
    else if ( pat.spread() == SpreadMethod::Repeat )
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
    else
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

    cairo_matrix_t matrix;

    Matrix2d tr = pat.transform() ;
    cairo_matrix_init (&matrix, tr.m1(), tr.m2(), tr.m3(), tr.m4(), tr.m5(), tr.m6());
    cairo_matrix_invert (&matrix);
    cairo_pattern_set_matrix (pattern, &matrix);

    cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST); //?

    cairo_set_source (cr(), pattern);

    cairo_pattern_destroy (pattern);
}

void RenderingContext::fill_stroke_shape() {

    const State &state = state_.top();

    if ( Brush *br = dynamic_cast<Brush *>(state.brush_.get()) )  {

        set_cairo_fill(br) ;

        if ( !state.pen_->isEmpty()  ) cairo_fill_preserve(cr()) ;
        else cairo_fill (cr());
    }

    if ( Pen *pen = dynamic_cast<Pen *>(state.pen_.get()) )  {
            set_cairo_stroke(*pen) ;
            cairo_stroke(cr()) ;
    }
}

void RenderingContext::set_cairo_fill(const Brush *br) {

    if ( br->fillRule() == FillRule::EvenOdd)
        cairo_set_fill_rule (cr(), CAIRO_FILL_RULE_EVEN_ODD);
    else if ( br->fillRule() == FillRule::NonZero )
        cairo_set_fill_rule (cr(), CAIRO_FILL_RULE_WINDING);

    if ( const SolidBrush *brush = dynamic_cast<const SolidBrush *>(br) )
    {
        const Color &clr = brush->color() ;

        if ( clr.a() * br->fillOpacity()  == 1.0 ) cairo_set_source_rgb(cr(), clr.r(), clr.g(), clr.b()) ;
        else cairo_set_source_rgba(cr(), clr.r(), clr.g(), clr.b(), clr.a() * br->fillOpacity() ) ;
    }
    else if ( const LinearGradientBrush *brush = dynamic_cast<const LinearGradientBrush *>(br) )
        cairo_apply_linear_gradient(*brush) ;
    else if ( const RadialGradientBrush *brush = dynamic_cast<const RadialGradientBrush *>(br) )
        cairo_apply_radial_gradient(*brush) ;
    else if ( const PatternBrush *brush = dynamic_cast<const PatternBrush *>(br) )
        cairo_apply_pattern(*brush) ;
}


void RenderingContext::line_path(double x0, double y0, double x1, double y1) {
    cairo_move_to(cr(), x0, y0) ;
    cairo_line_to(cr(), x1, y1) ;
}

void RenderingContext::polyline_path(double *pts, int n, bool close) {
    if ( n < 2 ) return ;

    cairo_move_to(cr(), pts[0], pts[1]) ;

    for(int i=1, k=2 ; i<n ; i++, k+=2 )
        cairo_line_to(cr(), pts[k], pts[k+1]) ;

    if ( close ) cairo_close_path(cr()) ;
}

cairo_t *RenderingContext::cr() {
    return cr_ ;
}



void RenderingContext::path(const Path &path) {

    cairo_new_path(cr()) ;

    for( const auto &pcb: path.commands() ) {

        switch( pcb.cmd_ ) {
        case Path::MoveToCmd:
            cairo_move_to(cr(), pcb.arg0_, pcb.arg1_) ;
            break ;
        case Path::LineToCmd:
            cairo_line_to(cr(), pcb.arg0_, pcb.arg1_) ;
            break ;
        case Path::CurveToCmd:
            cairo_curve_to(cr(), pcb.arg0_, pcb.arg1_, pcb.arg2_, pcb.arg3_, pcb.arg4_, pcb.arg5_) ;
            break ;
        case Path::ClosePathCmd:;
            cairo_close_path(cr()) ;
            break ;
        }
    }
}


void RenderingContext::rect_path(double x0, double y0, double w, double h) {
    cairo_rectangle(cr(), x0, y0, w, h);
}

cairo_surface_t *cairo_create_image_surface(const Image &im)
{
    cairo_surface_t *psurf ;

    int width = im.width(), height = im.height() ;

    int src_stride = im.stride() ;

    psurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height) ;

    // this is needed to work with transparency
    cairo_t *cr2 = cairo_create (psurf);
    cairo_set_operator (cr2, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba (cr2, 0, 0, 0, 1);
    cairo_paint (cr2);
    cairo_destroy (cr2);

    unsigned char *dst = cairo_image_surface_get_data(psurf) ;
    int dst_stride = cairo_image_surface_get_stride(psurf);

    unsigned char *dp, *drp = dst ;
    unsigned char *sp, *srp = (unsigned char *)im.pixels() ;

    if ( im.format() == ImageFormat::RGB24  )
    {
        for (int i = 0; i < height; i++)
        {
            dp = drp; sp = srp ;

            for (int j = 0; j < width; j++)
            {
                unsigned char red =   sp[0]  ;
                unsigned char green = sp[1]  ;
                unsigned char blue =  sp[2]  ;
                unsigned char alpha = 255 ;
                unsigned int p = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
                *((int32_t *)dp) = p ;

                dp += 4 ;
                sp += 3 ;
            }

            drp += dst_stride ;
            srp += src_stride ;
        }
    }
    else if ( im.format() == ImageFormat::ARGB32 )
    {
        for (int i = 0; i < height; i++)
        {
            dp = drp; sp = srp ;

            for (int j = 0; j < width; j++)
            {
                unsigned char alpha = sp[0] ;
                unsigned char red =   sp[1] ;
                unsigned char green = sp[2] ;
                unsigned char blue =  sp[3] ;

                unsigned int p = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);

                *((uint32_t *)dp) = p ;

                dp += 4 ;
                sp += 4 ;
            }

            drp += dst_stride ;
            srp += src_stride ;
        }
    }


    return psurf ;
}


RenderingContext::RenderingContext() {

}

RenderingContext::~RenderingContext() {
    cairo_destroy(cr_) ;
}


} // namespace detail

void Canvas::setPen(const PenBase &pen) {
    state_.top().pen_ = pen.clone() ;
}

void Canvas::setBrush(const BrushBase &br) {
    state_.top().brush_ = br.clone() ;
}


void Canvas::save() {
    cairo_save(cr()) ;

    detail::State inherited(state_.top()) ;
    state_.emplace(inherited) ;
}

void Canvas::restore() 	{
    cairo_restore(cr()) ;
    state_.pop() ;
}

void Canvas::setFont(const Font &font) {
    state_.top().font_ = font ;
}

void Canvas::drawText(Text &layout, double x0, double y0, double width, double height) {
    const detail::State &state = state_.top() ;

    const Font &f = state.font_ ;

    unsigned int flags = state.text_align_flags_ ;

    TextDecoration decoration = state.text_decoration_ ;

    cairo_scaled_font_t *scaled_font = FontManager::instance().createFont(f) ;

    layout.setFont(f) ;
    layout.setWrapWidth(width) ;
    layout.setTextDirection(state.text_direction_) ;

    cairo_font_extents_t extents ;
    cairo_scaled_font_extents(scaled_font, &extents) ;

    double y = 0, tx = 0, ty = 0 ;

    if ( flags & TextAlignVCenter )
        ty = ( height - layout.height())/2;
    else if ( flags & TextAlignBottom )
        ty = height - layout.height()  ;

    bool is_first_line = true ;

    for (const GlyphRun &line: layout.lines() ) {

        if ( flags & TextAlignHCenter )
            tx =  ( width - line.width() )/2.0 ;
        else if ( flags & TextAlignRight )
            tx =  width - line.width();

        if ( is_first_line ) {
            y += line.ascent() ;
            is_first_line = false ;
        }

        unsigned num_glyphs = line.numGlyphs() ;

        double gx = 0, ga = std::numeric_limits<double>::max() ;
        cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate (num_glyphs + 1);

        for ( unsigned i=0; i<num_glyphs; i++) {
            const Glyph &g = line.glyphs()[i] ;
            cairo_glyphs[i].index = g.index_ ;
            cairo_glyphs[i].x = gx + g.x_offset_ ;
            cairo_glyphs[i].y =  - g.y_offset_ ;
            ga = std::min(ga, g.y_advance_) ;

            gx +=  g.x_advance_;
        }

        cairo_save(cr()) ;

        cairo_set_scaled_font(cr(), scaled_font) ;

        cairo_translate(cr(), x0 + tx, y0 + y + ty) ;

        //      cairo_show_glyphs(cr_, cairo_glyphs, num_glyphs) ;
        cairo_glyph_path(cr(), cairo_glyphs, num_glyphs);

     //   cairo_fill(cr()) ;

        fill_stroke_shape() ;

        cairo_restore(cr()) ;

        cairo_glyph_free(cairo_glyphs) ;

        static const int text_underline_offset = 5 ;
        static const int text_underline_stroke = 2 ;

        if ( decoration == TextDecorationUnderline ) {
            cairo_rectangle(cr_, x0 + tx, y0 + y + ty + text_underline_offset - text_underline_stroke/2, gx, text_underline_stroke );
            fill_stroke_shape();
        } else if ( decoration == TextDecorationStrikeThrough ) {
            cairo_rectangle(cr_, x0 + tx, y0 + y + ty - text_underline_stroke/2 - extents.ascent/3, gx, text_underline_stroke );
            fill_stroke_shape();
        }
#if 0
        cairo_rectangle(cr_, 0, 0, layout.width(), -line.ascent()) ;
        cairo_rectangle(cr_, 0, 0, layout.width(), line.descent()) ;
#endif

        y += line.height() ;
#if 0
        cairo_set_source_rgb(cr_, 1, 0, 0) ;

        cairo_stroke(cr_) ;
#endif


    }

    cairo_scaled_font_destroy(scaled_font) ;
}

void Canvas::drawText(const std::string &text, double x0, double y0, double width, double height) {
    Text layout(text) ;
    drawText(layout, x0, y0, width, height) ;
}

void Canvas::drawText(const string &textStr, const Point2d &p) {
    drawText(textStr, p.x(), p.y()) ;
}

void Canvas::drawText(const string &textStr, const Rectangle2d &r) {
    drawText(textStr, r.x(), r.y(), r.width(), r.height()) ;
}

void Canvas::drawText(Text &text, const Point2d &p) {
    drawText(text, p.x(), p.y()) ;
}

void Canvas::drawText(Text &text, const Rectangle2d &r) {
    drawText(text, r.x(), r.y(), r.width(), r.height()) ;
}

void Canvas::drawText(Text &text, double x0, double y0)
{
    const detail::State &state = state_.top() ;

    const Font &f = state.font_ ;

    text.setFont(f) ;
    text.setTextDirection(state.text_direction_) ;

    const GlyphRun &line = text.lines()[0] ;

    unsigned num_glyphs = line.numGlyphs() ;

    double gx = 0 ;
    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate (num_glyphs + 1);

    for ( unsigned i=0; i<num_glyphs; i++) {
        const Glyph &g = line.glyphs()[i] ;
        cairo_glyphs[i].index = g.index_ ;
        cairo_glyphs[i].x = gx + g.x_offset_ ;
        cairo_glyphs[i].y =  - g.y_offset_ ;

        gx +=  g.x_advance_;
    }

    cairo_scaled_font_t *scaled_font = FontManager::instance().createFont(f) ;
    cairo_set_scaled_font(cr(), scaled_font) ;

    cairo_save(cr()) ;

    cairo_translate(cr(), x0, y0) ;

    cairo_glyph_path(cr(), cairo_glyphs, num_glyphs);

    fill_stroke_shape();
#if 0
    cairo_rectangle(cr_, 0, 0, layout.width(), -line.ascent_) ;
    cairo_rectangle(cr_, 0, 0, layout.width(), line.descent_) ;
#endif

#if 0
    cairo_set_source_rgb(cr_, 1, 0, 0) ;

    cairo_stroke(cr_) ;
#endif
    cairo_restore(cr()) ;

    cairo_glyph_free(cairo_glyphs) ;

    cairo_scaled_font_destroy(scaled_font) ;

}

void Canvas::drawText(const std::string &text, double x0, double y0) {
    Text layout(text) ;
    drawText(layout, x0, y0) ;
}

void Canvas::drawGlyphs(const vector<Glyph> &glyphs, const vector<Point2d> &gpos)
{
    const Font &f = state_.top().font_ ;

    cairo_scaled_font_t *scaled_font = FontManager::instance().createFont(f) ;

    unsigned num_glyphs = glyphs.size() ;

    assert(num_glyphs == gpos.size()) ;

    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate (num_glyphs + 1);

    for ( unsigned i=0; i<num_glyphs; i++ ) {
        const Glyph &g = glyphs[i] ;
        cairo_glyphs[i].index = g.index_ ;
        cairo_glyphs[i].x = gpos[i].x() ;
        cairo_glyphs[i].y = gpos[i].y() ;
    }

    cairo_save(cr()) ;

    cairo_set_scaled_font(cr(), scaled_font) ;

    cairo_glyph_path(cr(), cairo_glyphs, num_glyphs);

    fill_stroke_shape();

    cairo_restore(cr()) ;

    cairo_glyph_free(cairo_glyphs) ;

    cairo_scaled_font_destroy(scaled_font) ;
}


static void cairo_push_transform(cairo_t *cr, const Matrix2d &a)
{
    cairo_matrix_t matrix;
    cairo_matrix_init (&matrix, a.m1(), a.m2(), a.m3(), a.m4(), a.m5(), a.m6());
    cairo_transform (cr, &matrix);
}

void Canvas::fill(const Color &color)
{
    if ( color.a() == 1.0 )
        cairo_set_source_rgb(cr_, color.r(), color.g(), color.b()) ;
    else
        cairo_set_source_rgba(cr_, color.r(), color.g(), color.b(), color.a()) ;

    cairo_paint(cr_) ;
}

void Canvas::setBlendMode(BlendMode mode) {
    switch ( mode ) {
    case SRC_OVER:  cairo_set_operator(cr_, CAIRO_OPERATOR_OVER) ; break ;
    case SRC_IN:    cairo_set_operator(cr_, CAIRO_OPERATOR_IN) ; break ;
    case SRC_OUT:   cairo_set_operator(cr_, CAIRO_OPERATOR_OUT) ; break ;
    case SRC_ATOP:  cairo_set_operator(cr_, CAIRO_OPERATOR_ATOP) ; break ;
    case SRC:       cairo_set_operator(cr_, CAIRO_OPERATOR_SOURCE) ; break ;
    case DST:       cairo_set_operator(cr_, CAIRO_OPERATOR_DEST) ; break ;
    case DST_OVER:  cairo_set_operator(cr_, CAIRO_OPERATOR_DEST_OVER) ; break ;
    case DST_IN:    cairo_set_operator(cr_, CAIRO_OPERATOR_DEST_IN) ; break ;
    case DST_OUT:   cairo_set_operator(cr_, CAIRO_OPERATOR_DEST_OUT) ; break ;
    case DST_ATOP:  cairo_set_operator(cr_, CAIRO_OPERATOR_DEST_ATOP) ; break ;
    case CLEAR:     cairo_set_operator(cr_, CAIRO_OPERATOR_CLEAR) ; break ;
    case XOR:       cairo_set_operator(cr_, CAIRO_OPERATOR_XOR) ; break ;
    case ADD:       cairo_set_operator(cr_, CAIRO_OPERATOR_ADD) ; break ;
    case SATURATE:  cairo_set_operator(cr_, CAIRO_OPERATOR_SATURATE) ; break ;
    case MULTIPLY:  cairo_set_operator(cr_, CAIRO_OPERATOR_MULTIPLY) ; break ;
    case SCREEN:    cairo_set_operator(cr_, CAIRO_OPERATOR_SCREEN) ; break ;
    case OVERLAY:   cairo_set_operator(cr_, CAIRO_OPERATOR_OVERLAY) ; break ;
    case DARKEN:    cairo_set_operator(cr_, CAIRO_OPERATOR_DARKEN) ; break ;
    case LIGHTEN:    cairo_set_operator(cr_, CAIRO_OPERATOR_LIGHTEN) ; break ;
 //       case DODGE:    cairo_set_operator(cr_, CAIRO_OPERATOR_DODGE) ; break ;
 //       case BURN:    cairo_set_operator(cr_, CAIRO_OPERATOR_BURN) ; break ;
    case HARD_LIGHT:    cairo_set_operator(cr_, CAIRO_OPERATOR_HARD_LIGHT) ; break ;
    case SOFT_LIGHT:    cairo_set_operator(cr_, CAIRO_OPERATOR_SOFT_LIGHT) ; break ;
    case DIFFERENCE:    cairo_set_operator(cr_, CAIRO_OPERATOR_DIFFERENCE) ; break ;
    case EXCLUSION:     cairo_set_operator(cr_, CAIRO_OPERATOR_EXCLUSION) ; break ;
    case HSL_HUE:       cairo_set_operator(cr_, CAIRO_OPERATOR_HSL_HUE) ; break ;
    case HSL_COLOR:     cairo_set_operator(cr_, CAIRO_OPERATOR_HSL_COLOR) ; break ;
    case HSL_LUMINOSITY:cairo_set_operator(cr_, CAIRO_OPERATOR_HSL_LUMINOSITY) ; break ;
    default:            cairo_set_operator(cr_, CAIRO_OPERATOR_OVER) ;
    }

}

void Canvas::setTextAlign(int flags) {
    state_.top().text_align_flags_ = flags ;
}

void Canvas::setTextDirection(TextDirection dir) {
    state_.top().text_direction_ = dir ;
}

void Canvas::setTextDecoration(TextDecoration dec) {
    state_.top().text_decoration_ = dec ;
}

void Canvas::setClipRect(const Rectangle2d &r)
{
    setClipRect(r.x(), r.y(), r.width(), r.height()) ;
}

void Canvas::setClipRect(double x0, double y0, double w, double h)
{
    cairo_rectangle(cr(), x0, y0, w, h) ;
    cairo_clip(cr()) ;
}

void Canvas::setClipPath(const Path &p, FillRule rule)
{
    path(p) ;

    if ( rule == FillRule::EvenOdd )
        cairo_set_fill_rule(cr(), CAIRO_FILL_RULE_EVEN_ODD) ;
    else
        cairo_set_fill_rule(cr(), CAIRO_FILL_RULE_WINDING) ;

    cairo_clip(cr()) ;
}

void Canvas::drawLine(double x0, double y0, double x1, double y1) {
    line_path(x0, y0, x1, y1) ;
    fill_stroke_shape() ;
}

void Canvas::drawLine(const Point2d &p1, const Point2d &p2)
{
    drawLine(p1.x(), p1.y(), p2.x(), p2.y()) ;
}

void Canvas::drawRect(double x0, double y0, double w, double h) {
    rect_path(x0, y0, w, h) ;
    fill_stroke_shape() ;
}

void Canvas::drawRect(const Rectangle2d &r) {
    drawRect(r.x(), r.y(), r.width(), r.height()) ;
}

void Canvas::drawPolyline(double *pts, int nPts) {
    polyline_path(pts, nPts, false) ;
    fill_stroke_shape() ;
}

void Canvas::drawPolygon(double *pts, int nPts)
{
    polyline_path(pts, nPts, true) ;
    fill_stroke_shape() ;
}

void Canvas::drawPath(const Path &p) {
    path(p) ;
    fill_stroke_shape() ;
}

void Canvas::drawCircle(double cx, double cy, double r)
{
    cairo_arc (cr(), cx, cy, r, 0.0, 2*M_PI) ;
    fill_stroke_shape() ;
}

void Canvas::drawCircle(const Point2d &center, double r) {
    drawCircle(center.x(), center.y(), r) ;
}

#define SVG_ARC_MAGIC ((double) 0.5522847498)

static void cairo_elliptical_arc_to(cairo_t *cr, double x2, double y2)
{
    double x1, y1 ;
    double cx, cy, rx, ry ;

    cairo_get_current_point (cr, &x1, &y1);
    rx = x2 - x1 ;
    ry = y2 - y1 ;

    if ( rx > 0 && ry > 0 )
    {
        cx = x1 ;  	cy = y2 ;

        cairo_curve_to(cr,
                       cx + SVG_ARC_MAGIC * rx, cy - ry,
                       cx + rx, cy - SVG_ARC_MAGIC * ry,
                       x2, y2) ;
    }
    else if ( rx < 0 && ry > 0 )
    {
        rx = -rx ;	cx = x2 ;	cy = y1 ;

        cairo_curve_to(cr,
                       cx + rx, cy + SVG_ARC_MAGIC * ry,
                       cx + SVG_ARC_MAGIC * rx,  cy + ry,
                       x2, y2) ;
    }
    else if ( rx < 0 && ry < 0 )
    {
        rx = -rx ; ry = -ry ;	cx = x1 ;	cy = y2 ;

        cairo_curve_to(cr,
                       cx - rx*SVG_ARC_MAGIC, cy + ry,
                       cx - rx,  cy + SVG_ARC_MAGIC *ry,
                       x2, y2) ;
    }
    else {
        ry = -ry ;	cx = x2 ;	cy = y1 ;
        cairo_curve_to(cr,
                       cx - rx, cy - ry*SVG_ARC_MAGIC,
                       cx - rx*SVG_ARC_MAGIC,  cy - ry,
                       x2, y2) ;
    }

}

void Canvas::drawEllipse(double xp, double yp, double rxp, double ryp) {

    cairo_move_to(cr(), xp, yp - ryp) ;
    cairo_elliptical_arc_to(cr(), xp + rxp, yp) ;
    cairo_elliptical_arc_to(cr(), xp, yp + ryp) ;
    cairo_elliptical_arc_to(cr(), xp - rxp, yp) ;
    cairo_elliptical_arc_to(cr(), xp , yp - ryp) ;

    fill_stroke_shape() ;
}


Canvas::~Canvas()
{

}

Canvas::Canvas(Surface &surface): RenderingContext(), surface_(surface) {
    cr_ = cairo_create(surface.surf_) ;
    state_.emplace() ;
}


void Canvas::drawImage(const Image &im,  double opacity )
{
    cairo_surface_t *imsurf = detail::cairo_create_image_surface(im) ;

    cairo_save(cr()) ;

    cairo_set_source_surface(cr(), (cairo_surface_t *)imsurf, 0, 0);

    cairo_paint_with_alpha (cr(), opacity);

    cairo_surface_destroy(imsurf) ;
}


void Canvas::setTransform(const Matrix2d &tr) {
    cairo_push_transform(cr(), tr) ;
}

void Canvas::setAntialias(bool anti_alias)
{
    if ( anti_alias ) {
        cairo_set_antialias (cr(), CAIRO_ANTIALIAS_BEST);


    }
    else
        cairo_set_antialias (cr(), CAIRO_ANTIALIAS_DEFAULT);
}
/*
PatternCanvas::PatternCanvas(double width, double height): Canvas(width, height, 92, 92) {
    cairo_rectangle_t r{0, 0, width, height} ;
    surf_ = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &r) ;
    source_cr_ = cairo_create(surf_) ;
}
*/

} }

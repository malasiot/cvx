#include <cvx/gfx/plot/plot.hpp>

using namespace std ;

namespace cvx { namespace gfx {

void Plot::draw(Canvas &c, double w, double h)
{
    if ( !x_axis_.has_range_ )
        x_axis_.setRange(data_bounds_.minX(), data_bounds_.maxX()) ;
    if ( !y_axis_.has_range_ )
        y_axis_.setRange(data_bounds_.minY(), data_bounds_.maxY()) ;

    x_axis_.computeLayout(w, 1.0) ;
    y_axis_.computeLayout(h, 1.0) ;

    x_axis_.draw(c, w, h, 1.0);
    y_axis_.draw(c, w, h, 1.0);

    c.drawLine(0, -h, w, -h) ;
    c.drawLine(w, 0, w, -h) ;

    c.save();
    c.setClipRect(0, 0, w, -h) ;
    for( auto &g: graphs_ ) {
        g->draw(c) ;
    }
    c.restore() ;

    if ( !title_.empty() ) {
        c.save() ;
        Rectangle2d layout(w/2- max_title_width_/2, -h - title_offset_, max_title_width_, -100 ) ;
        c.setFont(title_font_) ;
        c.setBrush(SolidBrush(NamedColor::black())) ;
        c.setTextAlign(TextAlignHCenter|TextAlignBottom) ;
        c.drawText(title_, layout) ;
        c.restore() ;
    }

    legend_.draw(graphs_, c, w, h) ;

}

}}

#include <cvx/gfx/plot/plot.hpp>

namespace cvx { namespace gfx {

void Plot::draw(Canvas &c, double w, double h)
{
    x_axis_.setRange(data_bounds_.minX(), data_bounds_.maxX()) ;
    y_axis_.setRange(data_bounds_.minY(), data_bounds_.maxY()) ;

    x_axis_.computeLayout(w, 1.0) ;
    y_axis_.computeLayout(h, 1.0) ;


    x_axis_.draw(c, w, h, 1.0);
    y_axis_.draw(c, w, h, 1.0);


    c.drawLine(0, -h, w, -h) ;
    c.drawLine(w, 0, w, -h) ;

    for( auto &g: graphs_ ) {
        g->draw(c) ;
    }

}


}}

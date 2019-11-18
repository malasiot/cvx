#include <cvx/gfx/plot/line_graph.hpp>
#include <cvx/gfx/plot/plot.hpp>

using namespace std ;
namespace cvx { namespace gfx {

LineGraph::LineGraph(const vector<double> &x, const vector<double> &y): x_(x), y_(y) {
    assert(x.size() == y.size()) ;
}

BoundingBox LineGraph::getDataBounds() {
    double minx = numeric_limits<double>::max(),
            miny = numeric_limits<double>::max() ,
            maxx = -numeric_limits<double>::max(),
            maxy = -numeric_limits<double>::max() ;

    for( size_t i = 0 ; i<x_.size() ; i++ ) {
        minx = std::min(minx, x_[i]) ;
        miny = std::min(miny, y_[i]) ;
        maxx = std::max(maxx, x_[i]) ;
        maxy = std::max(maxy, y_[i]) ;
    }

    return { minx, miny, maxx, maxy } ;
}

void LineGraph::draw(Canvas &c)
{
    auto &xaxis = plot_->xAxis() ;
    auto &yaxis = plot_->yAxis() ;

    Path p ;

    for( int i=0 ; i<x_.size() ; i++ ) {
        double x = xaxis.transform(x_[i]) ;
        double y = yaxis.transform(y_[i]) ;

        if ( i == 0 ) p.moveTo(x, y) ;
        else p.lineTo(x, y) ;
    }


    c.save() ;
    c.setPen(pen_) ;
    c.drawPath(p) ;
    c.restore() ;

    if ( marker_ ) {
        for( int i=0 ; i<x_.size() ; i++ ) {
            double x = xaxis.transform(x_[i]) ;
            double y = yaxis.transform(y_[i]) ;

            c.save() ;
            c.setTransform(Matrix2d().translate(x, y)) ;
            marker_->draw(c) ;
            c.restore() ;
        }
    }
}

void LineGraph::drawLegend(Canvas &c, double width, double height)
{

}





}}

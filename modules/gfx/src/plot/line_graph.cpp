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

    double sx = xaxis.getScale();
    double tx = xaxis.getOffset() ;

    double sy = yaxis.getScale() ;
    double ty = yaxis.getOffset() ;

    Path p ;

    for( int i=0 ; i<x_.size() ; i++ ) {
        double x = sx * x_[i] + tx ;
        double y = sy * y_[i] + ty ;

        if ( i == 0 ) p.moveTo(x, -y) ;
        else p.lineTo(x, -y) ;
    }

    c.save() ;
    c.setPen(Pen(NamedColor::red(), 2)) ;
    c.drawPath(p) ;
    c.restore() ;

}

void LineGraph::drawLegend(Canvas &c, double width, double height)
{

}





}}

#ifndef CVX_GFX_PLOT_LINEGRAPH_HPP
#define CVX_GFX_PLOT_LINEGRAPH_HPP

#include <cvx/gfx/plot/graph.hpp>
#include <cvx/gfx/plot/markers.hpp>

namespace cvx { namespace gfx {

class LineGraph: public Graph {
public:
    LineGraph(const std::vector<double> &x, const std::vector<double> &y) ;

    Pen &pen() { return pen_ ; }
    void setMarker(Marker *marker) { marker_.reset(marker) ; } ;

    BoundingBox getDataBounds() override;
    void draw(Canvas &c) override;
    void drawLegend(Canvas &c, double width, double height) override;

private:

    std::vector<double> x_, y_ ;
    Pen pen_ ;
    std::unique_ptr<Marker> marker_ ;
} ;


}}



#endif

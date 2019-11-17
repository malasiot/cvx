#ifndef CVX_GFX_PLOT_LINEGRAPH_HPP
#define CVX_GFX_PLOT_LINEGRAPH_HPP

#include <cvx/gfx/plot/graph.hpp>

namespace cvx { namespace gfx {

class LineGraph: public Graph {
public:
    LineGraph(const std::vector<double> &x, const std::vector<double> &y) ;

    BoundingBox getDataBounds() override;
    void draw(Canvas &c) override;
    void drawLegend(Canvas &c, double width, double height) override;

private:

    std::vector<double> x_, y_ ;
} ;


}}



#endif

#ifndef CVX_GFX_PLOT_HPP
#define CVX_GFX_PLOT_HPP

#include <vector>
#include <cvx/gfx/plot/axis.hpp>
#include <cvx/gfx/plot/line_graph.hpp>

namespace cvx { namespace gfx {

class Graph ;
class LineGraph ;

class Plot {
public:

    Plot() = default;

    LineGraph &lines(const std::vector<double> &x, const std::vector<double> &y) {
        LineGraph *g = new LineGraph(x, y) ;
        addGraph(g) ;
        return *g ;
    }

    Plot &addGraph(Graph *g) {
        graphs_.push_back(std::unique_ptr<Graph>(g)) ;
        data_bounds_.extend(g->getDataBounds());
        g->plot_ = this ;
        return *this ;
    }

    Axis &xAxis() { return x_axis_ ; }
    Axis &yAxis() { return y_axis_ ; }

    void setTitle(const std::string &title) ;
    void showLegend(bool legend) ;

    void draw(Canvas &c, double w, double h) ;
    void updateLayout(double w, double h) ;

private:

    bool show_legend_ = false ;
    XAxis x_axis_ ;
    YAxis y_axis_ ;
    std::string title_ ;
    std::vector<std::unique_ptr<Graph>> graphs_ ;
    BoundingBox data_bounds_ ;


} ;




} }

#endif

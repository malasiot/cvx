#ifndef CVX_GFX_PLOT_HPP
#define CVX_GFX_PLOT_HPP

#include <vector>
#include <cvx/gfx/plot/axis.hpp>
#include <cvx/gfx/plot/line_graph.hpp>
#include <cvx/gfx/plot/legend.hpp>

namespace cvx { namespace gfx {

class Graph ;
class LineGraph ;
class Legend ;

class Plot {
public:

    Plot() = default;

    LineGraph &lines(const std::vector<double> &x, const std::vector<double> &y, const char *style = nullptr) {
        LineGraph *g = new LineGraph(x, y, style) ;
        addGraph(g) ;
        return *g ;
    }

    LineGraph &errorbars(const std::vector<double> &x,
                         const std::vector<double> &y,
                         const std::vector<double> &e,
                         const char *style = nullptr) {
        LineGraph *g = new LineGraph(x, y, style) ;
        g->setErrors(e) ;
        addGraph(g) ;
        return *g ;
    }

    Plot &addGraph(Graph *g) {
        graphs_.push_back(std::unique_ptr<Graph>(g)) ;
        data_bounds_.extend(g->getDataBounds());
        g->plot_ = this ;
        return *this ;
    }

    XAxis &xAxis() { return x_axis_ ; }
    YAxis &yAxis() { return y_axis_ ; }

    Legend &legend() { return legend_ ; }

    Plot &setTitle(const std::string &title) { title_ = title ; return *this ; }

    void draw(Canvas &c, double w, double h) ;


private:


    XAxis x_axis_ ;
    YAxis y_axis_ ;
    std::string title_ ;

    Legend legend_ ;

    double max_title_width_ = 200 ;
    double title_offset_ = 4 ;
    Font title_font_ = Font("Arial", 14);

    std::vector<std::unique_ptr<Graph>> graphs_ ;
    BoundingBox data_bounds_ ;

private:
    void drawLegend(Canvas &c, double w, double h);


} ;




} }

#endif

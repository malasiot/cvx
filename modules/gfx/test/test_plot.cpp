#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/image.hpp>
#include <cvx/gfx/vector.hpp>

#include <cvx/gfx/bbox.hpp>

#include <cvx/gfx/plot/axis.hpp>
#include <cvx/gfx/plot/plot.hpp>

#include <cmath>
using namespace cvx::gfx ;
using namespace std ;

int main(int argc, char *argv[]) {

    ImageSurface is(1024, 512) ;
    Canvas canvas(is) ;

    canvas.setPen(Pen()) ;
    canvas.fill(NamedColor::white()) ;

    Font f("Arial", 32) ;
    f.setStyle(FontStyle::Italic) ;
    canvas.setFont(f) ;

    Matrix2d tr ;
    tr.translate(100, 400) ;

    canvas.setTransform(tr);

    Plot plot ;
    plot.showLegend(true) ;

    vector<double> x = { 0.0, 0.1, 0.6 } ;
    vector<double> y = { 1.0, 3.1, -0.4 } ;

    LineGraph &graph = plot.lines(x, y) ;
    graph.setTitle("line 1") ;

    plot.xAxis().setGrid(true).setTitle("X") ;
    plot.yAxis().setGrid(true).setTitle("Y") ;
    plot.draw(canvas, 500, 300) ;

    is.flush() ;
    is.getImage().saveToPNG("/tmp/oo.png") ;
}

#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/image.hpp>
#include <cvx/gfx/vector.hpp>

#include <cvx/gfx/bbox.hpp>

#include <cvx/gfx/plot/axis.hpp>
#include <cvx/gfx/plot/plot.hpp>

#include <cmath>
using namespace cvx::gfx ;
using namespace std ;

class CustomMarker: public AdaptiveCircleMarker {
public:
    CustomMarker(uint ns) {


    }

    Color getOutlineColor(uint idx) const override{
        return Color(NamedColor::red(), 0.5) ;

    }
    Color getFillColor(uint idx) const override {
        if ( idx > 0)
            return Color(NamedColor::green(), 0.5) ;
        else
            return Color(NamedColor::red(), 0.5) ;

    }

    double getRadius(uint i) const override {
        return  i * 2 + 1 ;
    }

};

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

    vector<double> x1 = { 0.0, 0.1, 0.6 } ;
    vector<double> y1 = { 1.0, 3.1, -0.4 } ;

    vector<double> x2 = { 0.0, 0.4, 0.9 } ;
    vector<double> y2 = { -1.0, 1.1, -0.4 } ;

    vector<double> e = {0.3, 0.3, 0.2} ;

    auto &graph1 = plot.lines(x1, y1, "r--") ;
    graph1.setTitle("line 1") ;
    graph1.pen().setLineWidth(2) ;
    graph1.setMarker(new CustomMarker(2)) ;
    graph1.setErrors(e) ;
    graph1.setBrush(new SolidBrush(Color(0.5, 0.2, 0.1, 0.5)));

    auto &graph2 = plot.lines(x2, y2, "g+:") ;
     graph2.setTitle("line 2") ;
   // graph2.pen().setColor(NamedColor::green()) ;


    plot.xAxis().setGrid(true).setTitle("X") ;
    plot.yAxis().setGrid(true).setTitle("Y") ;
    plot.setTitle("Text plot") ;
    plot.legend().setPlacement(Legend::BottomRight) ;
    plot.draw(canvas, 500, 300) ;

    is.flush() ;
    is.getImage().saveToPNG("/tmp/oo.png") ;
}

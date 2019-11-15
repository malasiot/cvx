#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/image.hpp>
#include <cvx/gfx/vector.hpp>

#include <cvx/gfx/bbox.hpp>

#include <cvx/gfx/plot/axis.hpp>

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

    XAxis x_axis(0, 1) ;
    YAxis y_axis(-3.0e-4, 3.0e-4);

    Matrix2d tr ;
    tr.translate(100, 400) ;

    canvas.setTransform(tr);
    x_axis.setTitle("Seasons");
    x_axis.computeLayout(500, 1.0) ;
    x_axis.draw(canvas, 500, 1) ;

    y_axis.setTitle("Values") ;
    y_axis.computeLayout(300, 1.0) ;
    y_axis.draw(canvas, 300, 1) ;


    is.flush() ;
    is.getImage().saveToPNG("/tmp/oo.png") ;
}

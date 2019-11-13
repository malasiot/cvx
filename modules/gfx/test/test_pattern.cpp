#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/image.hpp>
#include <cvx/gfx/vector.hpp>

using namespace cvx::gfx ;
using namespace std ;

int main(int argc, char *argv[]) {

    RecordingSurface pattern(64, 64) ;

    Canvas pc(pattern) ;

    pc.setPen(Pen()) ;
    pc.setBrush(SolidBrush(NamedColor::blue())) ;

    pc.drawCircle(32, 32, 32) ;

    pattern.flush() ;

    ImageSurface is(512, 512, 92, 92) ;

    Canvas canvas(is) ;

    PatternBrush brush(pattern) ;
    brush.setSpread(SpreadMethod::Repeat) ;
    brush.setTransform(Matrix2d::rotation(10*M_PI/180)) ;
    canvas.setBrush(brush) ;
    canvas.drawRect(10, 10, 500, 500) ;

    is.flush() ;

    is.getImage().saveToPNG("/tmp/canvas.png") ;

}

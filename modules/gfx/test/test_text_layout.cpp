#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/text_layout.hpp>

#include <fstream>
#include <iostream>

using namespace cvx::gfx ;
using namespace std ;

int main(int argc, char *argv[]) {

    string text("This is a very short string") ;
    Font font("Arial", 32) ;

    TextLayout layout(text, font) ;
    layout.setWrapWidth(500) ;
    layout.setTextDirection(TextDirection::Auto);

    layout.compute() ;

    ImageSurface is(1024, 512) ;
    Canvas canvas(is) ;


    canvas.setFont(font) ;
    canvas.fill(NamedColor::white()) ;
    canvas.setBrush(SolidBrush(NamedColor::black())) ;
    canvas.setTextAlign(TextAlignVCenter|TextAlignRight);
    canvas.drawText(text, 0, 0, 300, 200) ;


    canvas.setBrush(EmptyBrush());
    canvas.drawRect(0, 0, 300, 200) ;

    is.flush() ;
    is.getImage().saveToPNG("/tmp/oo.png") ;


}

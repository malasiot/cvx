#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/text_layout.hpp>

#include <fstream>
#include <iostream>

using namespace cvx::gfx ;
using namespace std ;

int main(int argc, char *argv[]) {

    string text("This is a very short string") ;
    Font font("Arial", 32) ;

    Text layout(text) ;
    layout.setFont(font) ;
    layout.setWrapWidth(500) ;
    layout.setTextDirection(TextDirection::Auto);

    ImageSurface is(1024, 512) ;
    Canvas canvas(is) ;

    canvas.setFont(font) ;
    canvas.fill(NamedColor::white()) ;
    canvas.setBrush(SolidBrush(NamedColor::black())) ;
    canvas.setTextAlign(TextAlignVCenter|TextAlignRight);
    canvas.setTextDecoration(TextDecorationStrikeThrough) ;
    canvas.drawText(layout, 0, 0, 300, 200) ;

    canvas.setBrush(EmptyBrush());
    canvas.setPen(Pen(NamedColor::red())) ;
    canvas.drawRect(layout.box(Rectangle2d(0, 0, 300, 200), TextAlignVCenter|TextAlignRight)) ;

    is.flush() ;
    is.getImage().saveToPNG("/tmp/oo.png") ;
}

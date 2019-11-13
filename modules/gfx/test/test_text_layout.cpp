#include <cvx/gfx/canvas.hpp>
#include <cvx/gfx/text_layout.hpp>
#include <cvx/gfx/text.hpp>

#include <fstream>
#include <iostream>

using namespace cvx::gfx ;
using namespace std ;

int main(int argc, char *argv[]) {

    string text("This is a very short string") ;
    Font font("Arial", 64) ;

    TextLayout layout(text, font) ;
    layout.setWrapWidth(500) ;
    layout.setTextDirection(TextDirection::Auto);

    layout.compute() ;

    ImageSurface is(1024, 512) ;
    Canvas canvas(is) ;

    canvas.setBrush(SolidBrush(Color(NamedColor::white(), 1.0))) ;
    canvas.drawRect(0, 0, 1024, 512) ;

    is.flush() ;
    is.getImage().saveToPNG("/tmp/oo.png") ;

    StyledText st(text) ;

    st.addSpan(new FontSpan(font, 5, 6)) ;
    st.addSpan(new FontSpan(font, 10, 13)) ;

    auto x = st.split() ;

}

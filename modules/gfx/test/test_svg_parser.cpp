#include <cvx/gfx/canvas.hpp>

#include <fstream>
#include <iostream>


using namespace cvx::gfx ;
using namespace std ;

int main(int argc, char *argv[]) {


    SVGDocument doc ;

    ifstream strm("/home/malasiot/Downloads/adobe.svg") ;
    try {
        doc.readStream(strm) ;

        ImageCanvas canvas(1024, 512, 96) ;

       canvas.setBrush(SolidBrush(Color(NamedColor::white(), 0))) ;
        canvas.drawRect(0, 0, 1024, 512) ;
        canvas.drawSVG(doc) ;

        canvas.getImage().saveToPNG("/tmp/oo.png") ;

    }
    catch ( SVGLoadException &e ) {

        cout << e.what() << endl ;
    }

}

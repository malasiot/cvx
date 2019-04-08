#include "openni_grabber.hpp"
#include "rgbd_viewer.hpp"

#include <thread>
#include <chrono>

#include <QApplication>
#include <QMainWindow>
#include <QAction>
#include <QToolBar>

#include <cvx/viz/image/view.hpp>
#include <cvx/viz/image/widget.hpp>

using namespace cvx::viz ;
using namespace std ;


int main(int argc, char **argv)
{

    qRegisterMetaType< cv::Mat >("cv::Mat");

    OpenNI2Grabber grabber ;

    grabber.init() ;

    QApplication app(argc, argv);

    QMainWindow win ;

    RGBDWidget iw(&win) ;

    win.setCentralWidget(&iw);

    win.show() ;

    win.resize(640, 480) ;

    iw.connect(&iw, SIGNAL(updateImage(cv::Mat)), &iw, SLOT(setImage(cv::Mat))) ;

    grabber.setColorStreamCallback([&] ( const cv::Mat &clr ){
        if ( iw.getChannel() == 1 ) emit iw.updateImage(clr) ;
    }) ;
    grabber.startColorStream() ;

    grabber.setDepthStreamCallback([&] ( const cv::Mat &depth ){
        if ( iw.getChannel() == 2 ) emit iw.updateImage(depth) ;
    }) ;
    grabber.startDepthStream() ;


    return app.exec();
}
/*

int main(int argc, char *argv[]) {

    OpenNI2Grabber grabber ;

    grabber.init() ;

    int clr_counter = 0, depth_counter = 0 ;
    grabber.setColorStreamCallback([&] ( const cv::Mat &clr ){
        std::cout << "color " << clr_counter++ << std::endl ;
    }) ;
    grabber.startColorStream() ;


    while(1) {
         std::this_thread::sleep_for(chrono::milliseconds(1000));
    }

}
*/

#include "openni_grabber.hpp"
#include <thread>
#include <chrono>

using namespace std ;
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

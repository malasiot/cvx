#ifndef OPENNI_GRABBER_HPP
#define OPENNI_GRABBER_HPP

#include <OpenNI.h>
#include <string>
#include <opencv2/opencv.hpp>


class OpenNI2Grabber {
public:

    OpenNI2Grabber() ;
    ~OpenNI2Grabber() ;

    using Callback = std::function<void (const cv::Mat &)> ;

    bool init(const char *uri = openni::ANY_DEVICE);

    bool startColorStream() ;
    bool startDepthStream() ;
    bool registerColorAndDepth() ;

    void setColorStreamCallback(Callback cb) ;
    void setDepthStreamCallback(Callback cb) ;

    cv::Mat grabColor() ;
    cv::Mat grabDepth();

private:

    class ColorFrameListener ;
    class DepthFrameListener ;

    openni::Device device_;
    openni::VideoStream depth_, color_;

    std::unique_ptr<ColorFrameListener> color_cb_ ;
    std::unique_ptr<DepthFrameListener> depth_cb_ ;

    bool initDepthStream();
    bool initColorStream() ;



};




















#endif

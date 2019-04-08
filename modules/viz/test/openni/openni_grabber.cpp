#include "openni_grabber.hpp"

#include <cvx/util/misc/logger.hpp>

#include <iostream>

using namespace std ;

class OpenNI2Initializer {
public:

    bool init() {
        if ( initialized_ ) return true ;
        openni::Status rc = openni::OpenNI::initialize();
        if ( rc != openni::STATUS_OK ) return false ;
        initialized_ = true ;
        return true ;
    }

    ~OpenNI2Initializer() {
        if ( initialized_ ) {
            openni::OpenNI::shutdown();
            initialized_ = false ;
        }
    }

    bool initialized_ = false ;
};

static OpenNI2Initializer sInit ;


static cv::Mat convertColor(const openni::VideoFrameRef &frame) {
    cv::Mat rgb_image = cv::Mat(frame.getHeight(), frame.getWidth(), CV_8UC3,
                         const_cast<void*>(frame.getData()));

    cv::cvtColor(rgb_image, rgb_image, cv::COLOR_RGB2BGR) ;

    return rgb_image ;
}
static cv::Mat convertDepth(const openni::VideoFrameRef &frame) {

    openni::DepthPixel* pData = (openni::DepthPixel*)frame.getData();
    cv::Mat depth_image = cv::Mat(frame.getHeight(),
                                 frame.getWidth(),
                                 CV_16UC1,
                                 pData);

    return depth_image ;
}


class OpenNI2Grabber::ColorFrameListener: public openni::VideoStream::NewFrameListener
{
public:

    ColorFrameListener(OpenNI2Grabber::Callback cb): cb_(cb) {}

    void onNewFrame(openni::VideoStream& stream) {
        stream.readFrame(&frame_);
        cv::Mat img = convertColor(frame_) ;
        cb_(img) ;
    }
private:
    openni::VideoFrameRef frame_;
    OpenNI2Grabber::Callback cb_ ;
};

class OpenNI2Grabber::DepthFrameListener: public openni::VideoStream::NewFrameListener
{
public:

    DepthFrameListener(OpenNI2Grabber::Callback cb): cb_(cb) {}

    void onNewFrame(openni::VideoStream& stream) {
        stream.readFrame(&frame_);
        cv::Mat img = convertDepth(frame_) ;
        cb_(img) ;
    }

private:
    openni::VideoFrameRef frame_;
    OpenNI2Grabber::Callback cb_ ;
};

OpenNI2Grabber::~OpenNI2Grabber() {
    color_.stop() ;
    depth_.stop() ;
    color_.destroy() ;
    depth_.destroy() ;
}
    ;
OpenNI2Grabber::OpenNI2Grabber() = default ;

bool OpenNI2Grabber::init(const char *device_uri) {

    if ( !sInit.init() ) return false ;

    openni::Status rc = device_.open(device_uri);

    if ( rc != openni::STATUS_OK )  {
        LOG_ERROR("Device open failed:\n" << openni::OpenNI::getExtendedError() << "\n") ;
        return false ;
    }

    if ( !initDepthStream() ) return false ;
    if ( !initColorStream() ) return false ;

    device_.setDepthColorSyncEnabled(true) ;

    if ( !device_.isImageRegistrationModeSupported(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR) ||
          device_.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR) != openni::STATUS_OK ) {
        LOG_ERROR("Synchorization not supported:\n" << openni::OpenNI::getExtendedError() << "\n") ;
        return false ;
    }

    return true ;
}


bool OpenNI2Grabber::initDepthStream() {
    openni::Status rc = depth_.create(device_, openni::SENSOR_DEPTH);

    if ( rc == openni::STATUS_OK )  {
        openni::VideoMode depth_mode = depth_.getSensorInfo().getSupportedVideoModes()[4];

        LOG_INFO("The wished depth mode is " << depth_mode.getResolutionX() << " x " << depth_mode.getResolutionY() << " at "
                 << depth_mode.getFps() << " FPS. Pixel format " << depth_mode.getPixelFormat() ) ;

        if ( depth_.setVideoMode(depth_mode) != openni::STATUS_OK )  {
            LOG_WARN("Can't apply depth-videomode");
            depth_mode = depth_.getVideoMode();
            LOG_INFO("The wished depth mode is " << depth_mode.getResolutionX() << " x " << depth_mode.getResolutionY() << " at "
                     << depth_mode.getFps() << " FPS. Pixel format " << depth_mode.getPixelFormat() ) ;
        }
    }
    else  {
        LOG_ERROR("Couldn't find depth stream:\n" << openni::OpenNI::getExtendedError() << "\n") ;
        return false ;
    }

    return true ;
}

bool OpenNI2Grabber::initColorStream() {
    openni::Status rc = color_.create(device_, openni::SENSOR_COLOR);

    if (rc == openni::STATUS_OK) {
        openni::VideoMode mMode ;
        // set video mode
        mMode.setResolution(640, 480);
        mMode.setFps(30);
        mMode.setPixelFormat(openni::PIXEL_FORMAT_RGB888);

        if ( color_.setVideoMode(mMode) != openni::STATUS_OK )  {
            LOG_WARN("Can't apply videomode");
            LOG_INFO("The wished color mode is " << mMode.getResolutionX() << " x " << mMode.getResolutionY() << " at "
                     << mMode.getFps() << " FPS. Pixel format " << mMode.getPixelFormat() ) ;
            return false ;
        }

    }
    else  {
        LOG_ERROR("Couldn't find color stream:\n" << openni::OpenNI::getExtendedError() << "\n") ;
        return false ;
    }

    return true ;

}

bool OpenNI2Grabber::startColorStream() {
    openni::Status rc = color_.start();

    if (rc != openni::STATUS_OK) {
        LOG_ERROR("Couldn't start color stream:\n" << openni::OpenNI::getExtendedError() << "\n") ;
        return false ;
    }

    return true ;
}

bool OpenNI2Grabber::startDepthStream() {
    openni::Status rc = depth_.start();

    if (rc != openni::STATUS_OK) {
        LOG_ERROR("Couldn't start depth stream:\n" << openni::OpenNI::getExtendedError() << "\n") ;
        return false ;
    }

    return true ;
}

cv::Mat OpenNI2Grabber::grabColor() {

    openni::VideoFrameRef rgb_frame ;

    if ( color_.readFrame(&rgb_frame) == openni::STATUS_OK )
        return convertColor(rgb_frame) ;

    return cv::Mat() ;
}

cv::Mat OpenNI2Grabber::grabDepth() {

    openni::VideoFrameRef depth_frame ;

    if ( depth_.readFrame(&depth_frame) == openni::STATUS_OK && depth_frame.isValid() )
        return convertDepth(depth_frame)  ;

    return cv::Mat() ;
}

void OpenNI2Grabber::setColorStreamCallback(Callback cb) {
    color_cb_.reset(new ColorFrameListener(cb)) ;
    color_.addNewFrameListener(color_cb_.get()) ;
}

void OpenNI2Grabber::setDepthStreamCallback(Callback cb) {
    depth_cb_.reset(new DepthFrameListener(cb)) ;
    depth_.addNewFrameListener(color_cb_.get()) ;
}


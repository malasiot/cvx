#ifndef CVX_VIZ_TEXTURE_HPP
#define CVX_VIZ_TEXTURE_HPP

#include <Eigen/Geometry>
#include <opencv2/opencv.hpp>

namespace cvx { namespace viz {

// texture and its parameters

class Texture2D {
public:

    enum Format { FORMAT_RGB, FORMAT_RGBA, FORMAT_DEPTH } ;

    Texture2D() = delete ;
    Texture2D(const std::string &url): image_url_(url) {} // loaded from file lazely
    Texture2D(const cv::Mat &im): im_(im) {} // loaded from image in memory

    void read() ;
    void upload() ;

private:
    std::string image_url_ ;       // url should be file://<absolute path>
    std::string wrap_s_, wrap_t_ ;
    cv::Mat im_ ;

    uint texture_id_ = 0 ;
};



} // namespace viz
} // namespave cvx
#endif

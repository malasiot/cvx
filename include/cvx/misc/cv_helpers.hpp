#ifndef CVX_CV_HELPERS_HPP
#define CVX_CV_HELPERS_HPP

#include <opencv2/opencv.hpp>

// various opencv helpers

namespace cvx {

// use fprintf format pattern to create filename then call cv::imwrite
void imwritef(const cv::Mat &im, const char *format, ...) ;

// return all pixels on line
void getScanLine(const cv::Point &p1, const cv::Point &p2, std::vector<cv::Point> &pts) ;

}
#endif

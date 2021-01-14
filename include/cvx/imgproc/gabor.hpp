#ifndef CVX_IMGPROC_GABOR_HPP
#define CVX_IMGPROC_GABOR_HPP

#include <opencv2/opencv.hpp>

namespace cvx {

// Gabor filter-bank creation
void makeGaborFilterBank(int nOctaves, int nAngles, double sigma, std::vector<cv::Mat> &kernels) ;

// filter image with filter-bank
void applyGaborFilterBank(const cv::Mat &src, std::vector<cv::Mat> &kernels, std::vector<cv::Mat> &responses) ;

}

#endif

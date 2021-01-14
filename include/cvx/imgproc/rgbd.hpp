#ifndef CVX_IMGPROC_RGBD_HPP
#define CVX_IMGPROC_RGBD_HPP

#include <opencv2/opencv.hpp>

#include <cvx/camera/camera.hpp>
#include <cvx/geometry/point_list.hpp>

namespace cvx {

// convert depth image (16bit) to colored mapped image (maps depth value scaled tp [0, 1] to color Hue).
cv::Mat depthViz(const cv::Mat &depth, ushort minv = 0, ushort maxv = 0) ;

// Safely samples depth map to obtain depth value at (x, y) and a region of size [-ws, ws] around it.
// If none valid value found returns false.
bool sampleNearestNonZeroDepth(const cv::Mat &dim, int x, int y, ushort &z, int ws=1) ;

// same as above but also performs bilinear interpolation
bool sampleBilinearDepth(const cv::Mat &dim, float x, float y, float &z,int ws=1) ;

// create point cloud from depth image

void depthToPointCloud(const cv::Mat &depth, const PinholeCamera &model_, PointList3f &coords, uint sampling = 1) ;

}

#endif

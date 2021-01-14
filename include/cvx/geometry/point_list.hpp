#ifndef CVX_POINT_LIST_HPP
#define CVX_POINT_LIST_HPP

#include <cvx/geometry/point.hpp>

namespace cvx {

template <typename T, int D>
using pl_point_t = Eigen::Matrix<T, D, 1> ;

template <typename T, int D, typename alloc>
using pl_container_t = std::vector<pl_point_t<T, D>, alloc> ;

template <class T, int D, bool aligned = false>
struct pl_alloc_traits ;

template <class T, int D>
struct pl_alloc_traits<T, D, true> {
      using type = Eigen::aligned_allocator<pl_point_t<T, D>> ;
};

template <class T, int D>
struct pl_alloc_traits<T, D, false> {
      using type = std::allocator<pl_point_t<T, D>> ;
};

template <class T, int D, bool aligned>
using PointList = std::vector<pl_point_t<T, D>, typename pl_alloc_traits<T, D, aligned>::type> ;


template <typename T, int D, bool aligned = false>
PointList<T, D, aligned> makePointList(T *data, size_t n, bool row_major = true) {
    PointList<T, D, aligned> pl ;
    pl.resize(n) ;
    if ( row_major )
        for (uint i=0 ; i<n ; i++, data += D ) pl[i] = *reinterpret_cast<pl_point_t<T, D> *>(data) ;
    else {
        for( uint j=0 ; j<D ; j++ )
            for (uint i=0 ; i<n ; i++, data++ )
                pl[i][j] = *data ;
    }

    return pl ;
}

template <class T, int D, bool aligned = false>
PointList<T, D, aligned> makePointList(const Eigen::Matrix<T, Eigen::Dynamic, D> &x) {
    assert(x.cols() == D);
    PointList<T, D, aligned> pl ;
    pl.resize(x.rows()) ;
    for( uint idx = 0 ; idx<x.rows() ; idx++ )
        pl[idx] = x.row(idx) ;
    return pl ;
}

template <class T, int D, bool aligned = false>
PointList<T, D, aligned> makePointList(const cv::Mat &src) {
    assert(src.cols == D);
    PointList<T, D, aligned> pl ;
    pl.resize(src.rows) ;
    cv::Mat dst(src.rows, src.cols, cv::DataType<T>::type, &pl[0], D*(size_t)(sizeof(T)));
    src.convertTo(dst, dst.type());
    assert( dst.data == (uchar*)pl.data());
    return pl ;
}

template <class T, int D>
using Map = Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, D, Eigen::RowMajor>>;

template <class T, int D>
using ConstMap = Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, D, Eigen::RowMajor>>;


template <class T, int D, bool aligned>
Map<T, D> asEigenMap(PointList<T, D, aligned> &v) {
     return Map<T, D>(reinterpret_cast<T *>(v.data()->data()), v.size(), D) ;
}

template <class T, int D, bool aligned>
ConstMap<T, D> asEigenMap(const PointList<T, D, aligned> &v) {
     return ConstMap<T, D>(reinterpret_cast<const T *>(v.data()->data()), v.size(), D) ;
}

template <class T, int D, bool aligned>
Point<T, D> center(const PointList<T, D, aligned> &v) {
    return asEigenMap(v).colwise().mean();
}

template <class T, int D, bool aligned, int Mode>
void transform( PointList<T, D, aligned> &v, const Eigen::Transform<T, D, Mode> &xf) {
    for(uint i=0 ;i <v.size() ; i++ ) v[i] = xf * v[i] ;
}

template <class T, int D, bool aligned>
std::pair< Point<T, D>, Point<T, D> > bbox(const PointList<T, D, aligned> &v) {
    auto a = asEigenMap(v) ;
    return std::make_pair(a.colwise().minCoeff(), a.colwise().maxCoeff()) ;
}

template <class T, int D, bool aligned>
double norm(const PointList<T, D, aligned> &v) { return asEigenMap(v).norm() ; }

template <class T, int D, bool aligned>
cv::Mat toCVMat(const PointList<T, D, aligned> &v)  {
    return cv::Mat(v.size(), 1, cv::DataType< cv::Vec<T, D> >::type, (void*)v.data(), D * sizeof(T));
}

using PointList2f = PointList<float, 2, false> ;
using PointList3f = PointList<float, 3, false> ;
using PointList4f = PointList<float, 4, true> ;

using PointList2d = PointList<double, 2, true> ;
using PointList3d = PointList<double, 3, false> ;
using PointList4d = PointList<double, 4, true> ;

}


#endif

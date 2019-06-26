#ifndef CVX_VIZ_BONE_HPP
#define CVX_VIZ_BONE_HPP

#include <string>

#include <Eigen/Geometry>

namespace cvx { namespace viz {

struct Bone {
    std::string name_ ;
    Eigen::Affine3f offset_ ;
};

} // namespace viz
} // namespace cvx
#endif

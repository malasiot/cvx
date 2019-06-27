#ifndef CVX_VIZ_BONE_HPP
#define CVX_VIZ_BONE_HPP

#include <cvx/viz/scene/scene_fwd.hpp>
#include <string>
#include <Eigen/Geometry>

namespace cvx { namespace viz {

struct Bone {
    std::string name_ ;
    Eigen::Affine3f offset_ ;
    NodePtr node_ ;
};

} // namespace viz
} // namespace cvx
#endif

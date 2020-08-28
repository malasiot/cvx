#include <cvx/viz/robot/urdf_robot.hpp>
#include <cvx/viz/robot/urdf_loader.hpp>

namespace cvx { namespace viz { namespace urdf {

Robot Robot::load(const std::string &filename, const std::map<std::string, std::string> packages, bool load_collision_geometry) {
    urdf::Loader loader(packages, load_collision_geometry) ;
    urdf::Robot rb = loader.parse(filename) ;

    return rb ;
}


}
}
}

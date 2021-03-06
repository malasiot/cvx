#ifndef __URDF_LOADER_HPP__
#define __URDF_LOADER_HPP__

#include <map>
#include <pugixml/pugixml.hpp>
#include <cvx/viz/robot/urdf_robot.hpp>
#include <Eigen/Geometry>

namespace cvx { namespace viz {

namespace urdf {

class Loader {
public:
    Loader(const std::map<std::string, std::string> package_map, bool load_collision_geometry): package_map_(package_map),
        parse_collision_geometry_(load_collision_geometry) {}

    Robot parse(const std::string &urdf_file) ;

private:
    void parseRobot(const pugi::xml_node &node, Robot &rb, const std::string &path) ;
    void parseLink(const pugi::xml_node &node, Robot &rb, const std::string &path) ;
    void parseJoint(const pugi::xml_node &node, Robot &rb) ;
    bool buildTree(Robot &rb);

 //   RobotScenePtr exportScene() ;

    Eigen::Isometry3f parseOrigin(const pugi::xml_node &node) ;
    Geometry *parseGeometry(const pugi::xml_node &node, const std::string &mat, Eigen::Vector3f &sc, const std::string &path) ;
    void parseMaterial(const pugi::xml_node &node, Robot &rb, const std::string &path) ;
    std::string resolveUri(const std::string &uri, const std::string &path);
    Eigen::Matrix3f parseInertia(const pugi::xml_node &node);

    std::map<std::string, std::string> package_map_ ;

    bool parse_collision_geometry_ ;

};

} // namespace urdf

}}
#endif

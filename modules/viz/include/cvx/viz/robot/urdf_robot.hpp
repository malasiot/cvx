#ifndef __URDF_ROBOT_HPP__
#define __URDF_ROBOT_HPP__

#include <map>
#include <vector>
#include <Eigen/Geometry>
#include <memory>

namespace cvx { namespace viz {

namespace urdf {

struct Joint {
    std::string parent_, child_, type_, mimic_joint_, name_ ;
    Eigen::Vector3f axis_ ;
    Eigen::Isometry3f origin_ = Eigen::Isometry3f::Identity() ;
    float upper_, lower_, mimic_offset_, mimic_multiplier_, effort_, velocity_ ;
    float damping_, friction_ ;
    float position_ = 0;

    Eigen::Isometry3f getMatrix() const ;
};

struct Material ;

struct Geometry {
    virtual ~Geometry() {}
    std::string material_ref_ ;
    Eigen::Isometry3f origin_ = Eigen::Isometry3f::Identity() ;
} ;

struct MeshGeometry: public Geometry {
    std::string path_ ;
    Eigen::Vector3f scale_ ;
};

struct BoxGeometry: public Geometry {
    BoxGeometry(const Eigen::Vector3f &he): he_(he) {}
    Eigen::Vector3f he_ ;
};

struct SphereGeometry: public Geometry {
    SphereGeometry(float r): radius_(r) {}
    float radius_ ;
};

struct CylinderGeometry: public Geometry {
    CylinderGeometry(float radius, float height): radius_(radius), height_(height) {}
    float radius_, height_ ;
};

struct Material {
    std::string texture_path_ ;
    Eigen::Vector4f diffuse_color_ ;
};

struct Inertial {
   Eigen::Isometry3f origin_ = Eigen::Isometry3f::Identity() ;
   float mass_ ;
   Eigen::Matrix3f inertia_ ;
} ;

struct Link {
    std::string name_ ;

    std::unique_ptr<Inertial> inertial_ ;
    std::unique_ptr<Geometry> visual_geom_, collision_geom_ ;

    std::vector<Link *> child_links_ ;
    std::vector<Joint *> child_joints_ ;
    Link *parent_link_ = nullptr ;
    Joint *parent_joint_ = nullptr ;
};

struct Robot {

    static Robot load(const std::string &fname, const std::map<std::string, std::string> package_map, bool load_collision_geometry) ;

    Link *getLink(const std::string &name)  ;
    Joint *findJoint(const std::string &name);

    float setJointPosition(const std::string &jname, float pos) ;

    std::string name_ ;
    std::map<std::string, Joint> joints_ ;
    std::map<std::string, Link> links_ ;
    std::map<std::string, std::shared_ptr<Material>> materials_ ;
    Link *root_ ;

public:

    void computeLinkTransforms(std::map<std::string, Eigen::Isometry3f> &transforms) const ;
    void computeLinkTransformRecursive(std::map<std::string, Eigen::Isometry3f> &transforms, const Link *link, const Eigen::Isometry3f &parent) const;
};

class LoadException: public std::runtime_error {
public:
    LoadException(const std::string &msg): std::runtime_error(msg) {}
};
} // namespace urdf

}}
#endif

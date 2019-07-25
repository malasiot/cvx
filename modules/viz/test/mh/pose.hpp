#ifndef POSE_HPP
#define POSE_HPP

#include <Eigen/Geometry>
#include <map>

class Pose {
public:

    Pose(const std::map<std::string, Eigen::Matrix4f> &channels): channels_(channels) {}

    void setBoneTransform(const std::string &name, const Eigen::Matrix4f &t) ;
    void setBoneTransform(const std::string &name, const Eigen::Quaterniond &t) ;
    bool getBoneTransform(const std::string &name, Eigen::Matrix4f &t) const ;

    void setGlobalTransform(const Eigen::Matrix4f &g) {
        global_trans_ = g ;
    }

    const Eigen::Matrix4f &getGlobalTransform() const { return global_trans_ ; }

private:
    std::map<std::string, Eigen::Matrix4f> channels_ ;
    Eigen::Matrix4f global_trans_ = Eigen::Matrix4f::Identity() ;

};


#endif

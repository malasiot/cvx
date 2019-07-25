#ifndef SKELETON_HPP
#define SKELETON_HPP

#include <eigen3/Eigen/Geometry>
#include <vector>
#include <map>

typedef std::vector<Eigen::Vector4f,Eigen::aligned_allocator<Eigen::Vector4f> > EVVector4f ;
typedef std::vector<Eigen::Matrix4f,Eigen::aligned_allocator<Eigen::Matrix4f> > EVMatrix4f ;

struct Pose ;
struct Joint ;

struct Bone {
    Bone() = default ;

    std::string name_ ;
    Eigen::Matrix4f offset_ ; // aligns the bone with the armature
    Eigen::Matrix4f mat_ ; // relative transform between current node and its parent
    float length_ ;

    Bone *parent_ = nullptr ;
    std::vector<Bone *> children_ ;

    bool isChildOf(const Bone *other) const ;

    void addChild(Bone *other) {
        children_.push_back(other) ;
        other->parent_ = this ;
    }

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct Joint {
    std::string name_ ;
    int bone_ ; // The bone that affects this joint
    Eigen::Vector4f coords_ ; // coordinates of joint in default pose
};

struct Skeleton {

    std::vector<Bone> bones_ ;
    std::vector<Joint> joints_ ;
    std::map<std::string, uint> bone_map_ ;
    std::vector<Bone *> root_bones_ ;
    std::map<std::string, uint> joint_map_ ;

    int getBoneIndex(const std::string &name) const ;

    // accumulate transforms for each bone based on given pose
    void computeBoneTransforms(const Pose &pose, EVMatrix4f& transforms) const ;

    // compute the accumulated transformation of a bone given the pose
    Eigen::Matrix4f computeBoneTransform(const Pose &p, const std::string &name) const ;

    // compute derivatives of the bone transformation with respect to quaternion parameters of another bone
    // idx is the index of the bone. idx_var is the index of the bone with respect to which we take the derivative
    void computeBoneTransformDerivatives(const Pose &pose, uint idx, uint idx_var, const double q[4], Eigen::Matrix4f dr[4]) const ;

    void computeBoneTransformGlobalDerivatives(const Pose &pose, uint idx, const double v[8], Eigen::Matrix4f dr[8]) const ;

    // get head and tail of each bone given the pose
    void getGeometry(const Pose &p, std::vector<Eigen::Vector3f> &bones) ;

    void readJointList(const std::string &fileName) ;

    // a joint is a point on a bone (loc = 0 means head and loc = 1 means tail)
    void addJoint(const std::string &name, const std::string &bone, float loc = 0.0) ;

    // get skeleton joint position given the pose
    void getJointsPosition(const Pose &p, std::vector<Eigen::Vector3f> &joints) ;

    void computeTransforms() ;

    void getJointCoordinates(const Pose &p, std::vector<Eigen::Vector3f> &joints) const ;


private:
    void computeTransformRecursive(Bone *, Bone *) ;
    void getGeometryRecursive(Bone *b, const Pose &p, std::vector<Eigen::Vector3f> &bones ) ;
    void getJointsRecursive(Bone *b, const Pose &p, const Eigen::Vector3f &ptail, const Eigen::Matrix4f& parentTransform, std::vector<Eigen::Vector3f> &joints ) ;
    void readNodeHierarchy(const Bone* pNode, const Pose &pose, const Eigen::Matrix4f& parentTransform,
                           EVMatrix4f &final_bone_trans) const ;
    Eigen::Matrix4f computeBoneTransformRecursive(const Pose &pose, const Bone *b) const ;
    void computeBoneTransformDerivativesRecursive(const Pose &pose, const Bone *b, const Bone *bv, const double q[4], Eigen::Matrix4f dr[4]) const ;
    void computeBoneTransformGlobalDerivativesRecursive(const Pose &pose, const Bone *b, const double v[8], Eigen::Matrix4f dr[8]) const ;

};


#endif

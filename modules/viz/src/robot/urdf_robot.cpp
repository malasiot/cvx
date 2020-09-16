#include <cvx/viz/robot/urdf_robot.hpp>
#include <cvx/viz/robot/urdf_loader.hpp>

using namespace Eigen ;

namespace cvx { namespace viz { namespace urdf {

Robot Robot::load(const std::string &filename, const std::map<std::string, std::string> packages, bool load_collision_geometry) {
    urdf::Loader loader(packages, load_collision_geometry) ;
    urdf::Robot rb = loader.parse(filename) ;

    return rb ;
}

Link *Robot::getLink(const std::string &name)  {
    auto it = links_.find(name) ;
    if ( it == links_.end() ) return nullptr ;
    else return &(it->second) ;
}

Joint *Robot::findJoint(const std::string &name) {
    auto it = joints_.find(name) ;
    if ( it == joints_.end() ) return nullptr ;
    else return &(it->second) ;
}

float Robot::setJointPosition(const std::string &name, float pos)
{
    Joint *j = findJoint(name) ;
    if ( j != nullptr && j->mimic_joint_.empty() ) {
        pos = std::max(pos, j->lower_) ;
        pos = std::min(pos, j->upper_) ;
        j->position_ = pos ;

        for( auto &jp: joints_ ) {
            Joint &joint = jp.second ;
            if ( joint.mimic_joint_ == name ) {
                joint.position_ = pos * joint.mimic_multiplier_ + joint.mimic_offset_ ;
            }
        }
    }

    return pos ;
}

void Robot::computeLinkTransforms(std::map<std::string, Isometry3f> &transforms) const
{
    Isometry3f parent ;
    parent.setIdentity() ;
    computeLinkTransformRecursive(transforms, root_, parent) ;
}

void Robot::computeLinkTransformRecursive(std::map<std::string, Isometry3f> &transforms, const Link *link, const Isometry3f &parent) const
{
    Isometry3f p2j ;
    p2j.setIdentity() ;

    Joint *parent_joint = link->parent_joint_ ;

    if ( parent_joint ) {
        p2j = parent_joint->getMatrix() ;
    }

    Isometry3f tr = parent * p2j ;

  //  if ( link->visual_geom_ )
 //       transforms.emplace(link->name_, tr.matrix() * link->visual_geom_->origin_.matrix()) ;
  //  else
        transforms.emplace(link->name_, tr) ;

    for( const Link *l: link->child_links_ ) {
        computeLinkTransformRecursive(transforms, l, tr) ;
    }
}


Isometry3f Joint::getMatrix() const {
    Isometry3f tr ;
    tr.setIdentity() ;

    if ( type_ == "revolute" || type_ == "continuous" ) {
        tr.rotate(AngleAxisf(position_, axis_)) ;
    } else if ( type_ == "prismatic" ) {
        tr.translate(axis_* position_) ;
    }

    return origin_ * tr ;
}


}
}
}

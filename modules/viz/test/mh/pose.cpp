#include "pose.hpp"

using namespace std ;
using namespace Eigen ;

void Pose::setBoneTransform(const std::string &name, const Matrix4f &t) {
    channels_[name] = t ;
}

void Pose::setBoneTransform(const std::string &name, const Quaterniond &t) {

    Matrix4f rot = Matrix4f::Identity() ;
    rot.block(0, 0, 3, 3) = t.normalized().toRotationMatrix().cast<float>();

    map<string, Matrix4f>::const_iterator it = channels_.find(name) ;

    if ( it == channels_.end() )
        channels_.insert(make_pair(name,rot)) ;
    else
        channels_[name] = rot ;
}

bool Pose::getBoneTransform(const std::string &name, Matrix4f &t) const  {
    auto it = channels_.find(name) ;

    if ( it == channels_.end() ) {
        t = Matrix4f::Identity() ;
        return false ;
    }
    else {
        t = it->second ;
        return true ;
    }
}



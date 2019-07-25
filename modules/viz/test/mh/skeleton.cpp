#include "skeleton.hpp"
#include "pose.hpp"

using namespace std ;
using namespace Eigen ;

void Skeleton::computeTransformRecursive(Bone *bone, Bone *pbone)
{
    if ( pbone != nullptr )
        bone->mat_ =  pbone->offset_ * bone->offset_.inverse();
    else
        bone->mat_ = bone->offset_.inverse().eval()  ;

    for(int i=0 ; i<bone->children_.size() ; i++)  {
        computeTransformRecursive(bone->children_[i], bone) ;
    }

}

void Skeleton::computeTransforms()
{
    for(int i=0 ; i<root_bones_.size() ; i++ )
        computeTransformRecursive(root_bones_[i], nullptr) ;
}


int Skeleton::getBoneIndex(const std::string &name) const {
    std::map<std::string, uint>::const_iterator it = bone_map_.find(name) ;
    if ( it == bone_map_.end() ) return -1 ;
    else return it->second ;
}

void Skeleton::getGeometry(const Pose &p, std::vector<Eigen::Vector3f> &bones )
{
   EVMatrix4f bmat ;
   computeBoneTransforms(p, bmat) ;

   for(int i=0 ; i<bones_.size() ; i++)
   {
       const Bone &b = bones_[i] ;

       Vector4f head = bmat[i] * b.offset_.inverse() * Vector4f(0, 0, 0, 1) ;
       Vector4f tail = bmat[i] * b.offset_.inverse() * Vector4f(0, b.length_, 0, 1) ;

       bones.push_back(Vector3f(head.x(), head.y(), head.z())) ;
       bones.push_back(Vector3f(tail.x(), tail.y(), tail.z())) ;

   }

}

void Skeleton::readNodeHierarchy(const Bone* pNode, const Pose &pose, const Matrix4f& parentTransform, EVMatrix4f &final_bone_trans) const
{
    string nodeName(pNode->name_);

    Matrix4f nodeTransformation = pNode->mat_, poseTransformation;

    if ( pose.getBoneTransform(nodeName, poseTransformation) )
        nodeTransformation = nodeTransformation * poseTransformation ;

    Matrix4f globalTransformation = parentTransform * nodeTransformation;

    map<string, uint>::const_iterator it = bone_map_.find(nodeName) ;
    if ( it != bone_map_.end()) {
        uint boneIndex = it->second;
        final_bone_trans[boneIndex] =  globalTransformation  * bones_[boneIndex].offset_;
    }

    for (uint i = 0 ; i < pNode->children_.size() ; i++) {
        readNodeHierarchy(pNode->children_[i], pose, globalTransformation, final_bone_trans );
    }
}

void Skeleton::computeBoneTransforms(const Pose &pose, EVMatrix4f &transforms) const
{
    transforms.resize(bones_.size());

    for(int i=0 ; i<root_bones_.size() ; i++ )
        readNodeHierarchy(root_bones_[i], pose, pose.getGlobalTransform(), transforms);
}

Matrix4f Skeleton::computeBoneTransformRecursive(const Pose &pose, const Bone *b) const
{
    Matrix4f nodeTransformation = b->mat_, poseTransformation;

    if ( pose.getBoneTransform(b->name_, poseTransformation) )
        nodeTransformation = nodeTransformation * poseTransformation ;

    if ( b->parent_ == nullptr ) return pose.getGlobalTransform() * nodeTransformation ;
    else return computeBoneTransformRecursive(pose, b->parent_) * nodeTransformation  ;
}

Matrix4f Skeleton::computeBoneTransform(const Pose &pose, const std::string &boneName) const
{
    int idx = getBoneIndex(boneName) ;
    const Bone *b = &bones_[idx] ;

    return computeBoneTransformRecursive(pose, b) * b->offset_  ;
}

void Skeleton::computeBoneTransformDerivatives(const Pose &pose, uint idx, uint idxVar, const double q[4], Matrix4f dr[4]) const
{
    const Bone *b = &bones_[idx] ;
    const Bone *bn = &bones_[idxVar] ;

    if ( !bn->isChildOf(b) ) {
        for(uint i=0 ; i<4 ; i++) dr[i] = Matrix4f::Zero() ;
        return ;
    }

    computeBoneTransformDerivativesRecursive(pose, b, bn, q, dr)  ;

    for(int i=0 ; i<4 ; i++ ) dr[i] *= b->offset_ ;
}

void Skeleton::computeBoneTransformGlobalDerivatives(const Pose &pose, uint idx, const double v[8], Matrix4f dr[8]) const
{
    const Bone *b = &bones_[idx] ;

    computeBoneTransformGlobalDerivativesRecursive(pose, b, v, dr)  ;

    for(int i=0 ; i<8 ; i++ ) dr[i] *= b->offset_ ;
}

static Matrix4f Rq(double x, double y, double z, double w)
{
    Matrix4f res ;

    float n22 = sqrt(x*x + y*y + z*z + w*w) ;

    x /= n22 ; y /= n22 ; z /= n22 ; w /= n22 ;

    res << 1 - 2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w), 0,
           2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w), 0,
           2*(x*z-y*w), 2*(y*z+x*w),   1-2*(x*x+y*y), 0,
           0, 0, 0, 1 ;

    return res ;

}

static void diffR(Matrix4f rd[4], const double q[4])
{
    Matrix4f r[4] ;

    double x = q[0], y = q[1], z = q[2], w = q[3] ;
    float n2 = x*x + y*y + z*z + w*w ;
    float n22 = sqrt(n2) ;
    float n32 = n2 * n22 ;

//    x/=n22 ; y/=n22 ; z/=n22 ; w/=n22 ;

    r[0] <<  0,      2*y,    2*z,    0,
             2*y,   -4*x,    -2*w,    0,
             2*z,    2*w,   -4*x,    0,
             0,      0,      0,      0 ;

    r[1] << -4*y,    2*x,    2*w,    0,
             2*x,    0,      2*z,    0,
            -2*w,    2*z,   -4*y,    0,
             0,      0,      0,      0 ;

    r[2] << -4*z,   -2*w,    2*x,    0,
             2*w,   -4*z,    2*y,    0,
             2*x,    2*y,    0,      0,
             0,      0,      0,      0 ;

    r[3] <<  0,     -2*z,    2*y,    0,
             2*z,    0,     -2*x,    0,
            -2*y,    2*x,    0,      0,
             0,      0,      0,      0 ;

    for(uint j=0 ; j<4 ; j++)
    {
        Matrix4f td = Matrix4f::Zero() ;

        for(uint m=0 ; m<4 ; m++)  {
            float nf = ( j == m ) ? 1/n22 : 0;
            nf -= q[m] * q[j] / n32 ;
            td += r[m] * nf ;
        }

        rd[j] = td ;

    }

}

void Skeleton::computeBoneTransformDerivativesRecursive(const Pose &pose, const Bone *b, const Bone *bv, const double q[4],  Matrix4f dr[4]) const
{
    if ( b == nullptr ) {
        for (uint i=0 ; i<4 ; i++) dr[i] = pose.getGlobalTransform() ;
    }
    else {
        computeBoneTransformDerivativesRecursive(pose, b->parent_, bv, q, dr) ;

        Matrix4f poseTransformation ;

        pose.getBoneTransform(b->name_, poseTransformation) ;

        if ( b == bv )
        {
            Matrix4f rr[4] ;
            diffR(rr, q) ;

            for(int i=0 ; i<4 ; i++)
                dr[i] = dr[i] * b->mat_ * rr[i] ;
        }
        else   {
            for(int i=0 ; i<4 ; i++)
                dr[i] = dr[i] * b->mat_ * poseTransformation ;
        }


    }
}


void Skeleton::computeBoneTransformGlobalDerivativesRecursive(const Pose &pose, const Bone *b, const double v[8],  Matrix4f dr[8]) const
{
    if ( b == 0 ) {
        // derivative with respect to rotation component

        Matrix4f T, S, R, dS ;

        T << 1, 0, 0, v[4],
             0, 1, 0, v[5],
             0, 0, 1, v[6],
             0, 0, 0, 1 ;

        S << v[7], 0, 0, 0,
             0, v[7], 0, 0,
             0,  0,  v[7], 0,
             0, 0, 0, 1 ;

        dS << 1, 0, 0, 0,
              0, 1, 0, 0,
              0, 0, 1, 0,
              0, 0, 0, 0 ;

        R = Rq(v[0], v[1], v[2], v[3]) ;

        diffR(dr, v) ;

        for (uint i=0 ; i<4 ; i++) dr[i] = T * dr[i] * S ;
        for (uint i=0 ; i<3 ; i++) {
            Matrix4f dT = Matrix4f::Zero() ;
            dT(i, 3) = 1.0 ;
            dr[4+i] = dT * R * S ;
        }

        dr[7] = T * R * dS ;

    }
    else {
        computeBoneTransformGlobalDerivativesRecursive(pose, b->parent_, v, dr) ;

        Matrix4f poseTransformation ;

        pose.getBoneTransform(b->name_, poseTransformation) ;

        for(int i=0 ; i<8 ; i++)
           dr[i] = dr[i] * b->mat_ * poseTransformation ;

    }
}

void Skeleton::getGeometryRecursive(Bone *bone, const Pose &p, std::vector<Eigen::Vector3f> &bones)
{

    Matrix4f lb = Matrix4f::Identity() ;
    p.getBoneTransform(bone->name_, lb) ;

    Vector4f head = bone->offset_.inverse() * Vector4f(0, 0, 0, 1) ;
    Vector4f tail = bone->offset_.inverse() * Vector4f(0, bone->length_, 0, 1) ;

    Matrix4f globalTransformation =  bone->mat_ * lb ;

   head = globalTransformation * head ;
   tail = globalTransformation * tail ;

    bones.push_back(Vector3f(head.x(), head.y(), head.z())) ;
    bones.push_back(Vector3f(tail.x(), tail.y(), tail.z())) ;

    for(int i=0 ; i<bone->children_.size() ; i++)
        getGeometryRecursive(bone->children_[i], p, bones) ;
}

bool Bone::isChildOf(const Bone *other) const {
    const Bone *p = other ;
    while ( p != this ) {
        if ( p == nullptr ) return false ;
        p = p->parent_ ;
    }
    return true ;
}

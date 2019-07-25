#include "skinned_mesh.hpp"


using namespace std ;
using namespace Eigen ;
using namespace cvx::util ;

void SkinnedMesh::VertexBoneData::addBoneData(uint boneID, float w) {
  id_.push_back(boneID) ;
  weight_.push_back(w) ;
}

void SkinnedMesh::VertexBoneData::normalize()
{
    float w = 0.0 ;

    for(int i=0 ; i<id_.size() ; i++) {
        w += weight_[i] ;
    }

    if ( w == 0.0 ) return ;

    for(int i=0 ; i<id_.size() ; i++) {
        weight_[i] /= w  ;
    }
}

void SkinnedMesh::getTransformedVertices(const Pose &p, std::vector<Eigen::Vector3f> &mpos, std::vector<Eigen::Vector3f> &mnorm) const
{
    EVMatrix4f bones ;
    skeleton_.computeBoneTransforms(p, bones) ;

    for(int i=0 ; i<positions_.size() ; i++)
    {
        const Vector3f &pos = positions_[i] ;
        Vector3f normal ;

        if ( !normals_.empty() ) normal = normals_[i] ;

        const VertexBoneData &bdata = bone_weights_[i] ;

        Matrix4f boneTransform ;

        for( size_t j=0 ; j<bdata.id_.size() ; j++)
        {
            uint idx = bdata.id_[j] ;

            if ( j == 0 ) boneTransform = bones[idx] * bdata.weight_[j] ;
            else boneTransform += bones[idx] * (double)bdata.weight_[j] ;
        }

        Vector4f p = boneTransform * Vector4f(pos.x(), pos.y(), pos.z(), 1.0) ;
        mpos.push_back(Vector3f(p.x(), p.y(), p.z())) ;

        if ( !normals_.empty() )
        {
            Vector4f n = boneTransform * Vector4f(normal.x(), normal.y(), normal.z(), 0.0) ;
            Vector3f nrm(n.block<3, 1>(0, 0)) ;
            mnorm.push_back(nrm.normalized()) ;
        }
    }

}

void SkinnedMesh::getTransformedVertices(const Pose &p, vector<Vector3f> &mpos, vector<Vector3f> &mnorm, Vector3f& bboxMin, Vector3f& bboxMax) const
{
    EVMatrix4f bones ;
    skeleton_.computeBoneTransforms(p, bones) ;

    const float MAXV = std::numeric_limits<float>::max() ;

    bboxMin = Vector3f(MAXV, MAXV, MAXV);
    bboxMax = Vector3f(-MAXV,-MAXV,-MAXV);

    for(size_t i=0 ; i<positions_.size() ; i++)  {
        const Vector3f &pos = positions_[i] ;
        Vector3f normal ;

        if ( !normals_.empty() ) normal = normals_[i] ;

        const VertexBoneData &bdata = bone_weights_[i] ;

        Matrix4f boneTransform ;

        float w = 0.0 ;

        for( int j=0 ; j<bdata.id_.size() ; j++)
        {
            int idx = bdata.id_[j] ;
            if ( idx < 0 ) break ;

            if ( j == 0 ) boneTransform = bones[idx] * bdata.weight_[j] ;
            else boneTransform += bones[idx] * (double)bdata.weight_[j] ;
        }

        Vector4f p = boneTransform * Vector4f(pos.x(), pos.y(), pos.z(), 1.0) ;
        mpos.push_back(Vector3f(p.x(), p.y(), p.z())) ;

        if (p.x() >= bboxMax.x()) bboxMax.x()=p.x();
        if (p.y() >= bboxMax.y()) bboxMax.y()=p.y();
        if (p.z() >= bboxMax.z()) bboxMax.z()=p.z();
        if (p.x() <= bboxMin.x()) bboxMin.x()=p.x();
        if (p.y() <= bboxMin.y()) bboxMin.y()=p.y();
        if (p.z() <= bboxMin.z()) bboxMin.z()=p.z();

        if ( !normals_.empty() )
        {
            Vector4f n = boneTransform * Vector4f(normal.x(), normal.y(), normal.z(), 0.0) ;
            Vector3f nrm(n.block<3, 1>(0, 0)) ;
            mnorm.push_back(nrm.normalized()) ;
        }
    }

}

void SkinnedMesh::getTransformedVertices(const Pose &p, const std::vector<uint32_t> indices, std::vector<Eigen::Vector3f> &mpos,Eigen::Vector3f& bboxMin, Eigen::Vector3f& bboxMax) const
{
    EVMatrix4f bones ;
    skeleton_.computeBoneTransforms(p, bones) ;

    const float MAXV = std::numeric_limits<float>::max() ;

    bboxMin = Vector3f(MAXV, MAXV, MAXV);
    bboxMax = Vector3f(-MAXV,-MAXV,-MAXV);

    for(int i=0 ; i<indices.size() ; i++)
    {
        const Vector3f &pos = positions_[indices[i]] ;

        const VertexBoneData &bdata = bone_weights_[indices[i]] ;

        Matrix4f boneTransform ;

        for( int j=0 ; j<bdata.id_.size() ; j++)
        {
            uint idx = bdata.id_[j] ;

            if ( j == 0 ) boneTransform = bones[idx] * bdata.weight_[j] ;
            else boneTransform += bones[idx] * (double)bdata.weight_[j] ;
        }

        Vector4f p = boneTransform * Vector4f(pos.x(), pos.y(), pos.z(), 1.0) ;
        mpos.push_back(Vector3f(p.x(), p.y(), p.z())) ;

        if (p.x() >= bboxMax.x()) bboxMax.x()=p.x();
        if (p.y() >= bboxMax.y()) bboxMax.y()=p.y();
        if (p.z() >= bboxMax.z()) bboxMax.z()=p.z();
        if (p.x() <= bboxMin.x()) bboxMin.x()=p.x();
        if (p.y() <= bboxMin.y()) bboxMin.y()=p.y();
        if (p.z() <= bboxMin.z()) bboxMin.z()=p.z();

    }

}

void SkinnedMesh::getTransformedVertices(const Pose &p, PointList3f &mpos) const
{
    EVMatrix4f bones ;
    skeleton_.computeBoneTransforms(p, bones) ;

    for(int i=0 ; i<positions_.size() ; i++)
    {
        const Vector3f &pos = positions_[i] ;

        const VertexBoneData &bdata = bone_weights_[i] ;

        Matrix4f boneTransform ;

        float w = 0.0 ;

        for( int j=0 ; j<bdata.id_.size() ; j++)
        {
            uint idx = bdata.id_[j] ;

            if ( j == 0 ) boneTransform = bones[idx] * bdata.weight_[j] ;
            else boneTransform += bones[idx] * (double)bdata.weight_[j] ;
        }

        Vector4f p = boneTransform * Vector4f(pos.x(), pos.y(), pos.z(), 1.0) ;
        mpos.push_back(Vector3f(p.x(), p.y(), p.z())) ;
    }
}


void SkinnedMesh::getTransformedVertices(const Pose &p, const std::vector<uint32_t> indices, std::vector<Vector3f> &mpos, std::vector<Vector3f> &mnorm) const
{
    EVMatrix4f bones ;
    skeleton_.computeBoneTransforms(p, bones) ;

    for(int i=0 ; i<indices.size(); i++)
    {
        const Vector3f &pos = positions_[indices[i]] ;
        Vector3f normal ;

        if ( !normals_.empty() ) normal = normals_[indices[i]] ;

        const VertexBoneData &bdata = bone_weights_[indices[i]] ;

        Matrix4f boneTransform ;

        for( uint j=0 ; j<bdata.id_.size() ; j++)
        {
            uint idx = bdata.id_[j] ;

            if ( j == 0 ) boneTransform = bones[idx] * bdata.weight_[j] ;
            else boneTransform += bones[idx] * bdata.weight_[j] ;
        }

        Vector4f p = boneTransform * Vector4f(pos.x(), pos.y(), pos.z(), 1.0) ;
        mpos.push_back(Vector3f(p.x(), p.y(), p.z())) ;

        if ( !normals_.empty() )
        {
            Vector4f n = boneTransform * Vector4f(normal.x(), normal.y(), normal.z(), 0.0) ;
            Vector3f nrm(n.block<3, 1>(0, 0)) ;
            mnorm.push_back(nrm.normalized()) ;
        }
    }

}

void SkinnedMesh::getTransformedVertices(const Pose &p, const std::vector<uint32_t> indices, std::vector<Vector3f> &mpos) const
{
    EVMatrix4f bones ;
    skeleton_.computeBoneTransforms(p, bones) ;

    for(int i=0 ; i<indices.size(); i++)
    {
        const Vector3f &pos = positions_[indices[i]] ;
        Vector3f normal ;

        if ( !normals_.empty() ) normal = normals_[indices[i]] ;

        const VertexBoneData &bdata = bone_weights_[indices[i]] ;

        Matrix4f boneTransform ;

        for( size_t j=0 ; j<bdata.id_.size() ; j++)
        {
            uint idx = bdata.id_[j] ;

            if ( j == 0 ) boneTransform = bones[idx] * bdata.weight_[j] ;
            else boneTransform += bones[idx] * bdata.weight_[j] ;
        }

        Vector4f p = boneTransform * Vector4f(pos.x(), pos.y(), pos.z(), 1.0) ;
        mpos.push_back(Vector3f(p.x(), p.y(), p.z())) ;
    }

}


void SkinnedMesh::makeColorMap(std::vector<Vector3f> &clrs)
{
    float minX = positions_[0].x(), maxX = positions_[0].x(),
          minY = positions_[0].y(), maxY = positions_[0].y(),
          minZ = positions_[0].z(), maxZ = positions_[0].z() ;

    for(uint i=1 ; i<positions_.size() ; i++) {
        minX = std::min(minX, positions_[i].x()) ;
        maxX = std::max(maxX, positions_[i].x()) ;
        minY = std::min(minY, positions_[i].y()) ;
        maxY = std::max(maxY, positions_[i].y()) ;
        minZ = std::min(minZ, positions_[i].z()) ;
        maxZ = std::max(maxZ, positions_[i].z()) ;
    }


    for(uint i=0 ; i<positions_.size() ; i++ )
    {
        float sx = (positions_[i].x() - minX)/(maxX - minX) ;
        float sy = (positions_[i].y() - minY)/(maxY - minY) ;
        float sz = (positions_[i].z() - minZ)/(maxZ - minZ) ;

        Vector3f clr(sx, sy, sz) ;
        clrs.push_back(clr) ;
    }


}

uint SkinnedMesh::getDominantBone(const uint vtx_idx) const
{
    const VertexBoneData &bdata = bone_weights_[vtx_idx] ;

    uint bone_idx ;
    float maxw = 0.0 ;

    for( size_t j=0 ; j<bdata.id_.size() ; j++) {
        uint idx = bdata.id_[j] ;

        if ( bdata.weight_[j] > maxw )  {
            maxw = bdata.weight_[j] ;
            bone_idx = idx ;
        }
    }

    return bone_idx ;
}


/////////////////////////////////////////////////////////////////////////////////////////////////

MakeHumanSkinnedMesh::MakeHumanSkinnedMesh(const MHModel &model)
{
    createSkeleton(model) ;
    createMesh(model) ;
}


void MakeHumanSkinnedMesh::createSkeleton(const MHModel &model) {

    // make skeleton

    for( const auto &kv: model.bones_ ) {
        const MHBone &bone = kv.second ;
        string boneName = kv.first ;

        Bone b ;
        b.name_ = boneName ;
        b.offset_ = bone.bmat_.inverse() ;

        b.length_ = (bone.tail_ - bone.head_).norm() ;

        skeleton_.bone_map_.emplace(boneName, skeleton_.bones_.size()) ;
        skeleton_.bones_.emplace_back(b) ;
    }

    // build hierarchy

    for( const auto &kv: model.bones_ ) {
        const MHBone &bone = kv.second ;
        string name = kv.first ;

        uint idx = skeleton_.bone_map_[name] ;
        Bone *b = &skeleton_.bones_[idx] ;

        if ( bone.parent_.empty() ) skeleton_.root_bones_.push_back(b) ;
        else {
            uint pidx = skeleton_.bone_map_[bone.parent_] ;
            skeleton_.bones_[pidx].addChild(b) ;
        }
    }

    skeleton_.computeTransforms() ;
}

static void compute_normals(const vector<Vector3f> &vertices, const vector<uint> &indices, vector<Vector3f> &vtx_normals) ;


void MakeHumanSkinnedMesh::createMesh(const MHModel &model) {
    auto it = model.geometries_.find("Human:Body") ;
    if ( it == model.geometries_.end() ) return ;

    const MHGeometry &geom = it->second ;
    const MHMesh &mesh = geom.mesh_ ;

    vector<int> indices ;
    vector<int> face_vertices ;

    for( const Vector3f &v: mesh.vertices_ ) {
        positions_.push_back(v + geom.offset_) ;
    }

    // add faces

    for(int i=0 ; i<mesh.faces_.size() ; i++) {
        const MHFace &f = mesh.faces_[i] ;

        for(size_t k=0 ; k<f.num_vertices_ ; k++) {
            int idx = (int)f.indices_[k] ;

            indices.push_back(idx) ;
        }

        indices.push_back(-1) ;
    }

    // bone weights

    size_t n = positions_.size() ;

    bone_weights_.resize(n) ;

    // collect weights per vertex, we need then to normalize and keep only max 4 weights

    vector<vector<int>> vg_idxs(n) ;
    vector<vector<float>> vg_weights(n) ;

    for ( const auto &kv: mesh.groups_ ) {
        const string &name = kv.first ;
        const MHVertexGroup &group = kv.second ;

        int bidx = skeleton_.getBoneIndex(name);

        if ( bidx != -1 )  {

            for( int i=0 ; i<group.idxs_.size() ; i++ )  {
                int idx = group.idxs_.at(i) ;

                if ( idx < 0 ) continue ;

                vg_idxs[idx].push_back(bidx) ;
                vg_weights[idx].push_back(group.weights_[i]) ;

            }
        }
    }

    for( size_t i=0 ; i<n ; i++ ) {
        auto &idxs = vg_idxs[i] ;
        auto &weights = vg_weights[i] ;

        auto &dest = bone_weights_[i] ;

        // normalize

        if ( i == 8872 ) {
            cout << "break" << endl ;
        }
        float w = 0.0 ;
        for ( float weight: weights ) w += weight ;
        for ( float &weight: weights ) weight /= w ;

        // copy to destination

        for( size_t k=0 ; k<idxs.size() ; k++ ) {
            dest.id_.push_back(idxs[k]) ;
            dest.weight_.push_back(weights[k]) ;
        }

    }


    // split faces into triangles

    for( int i=0 ; i<indices.size() ; i++ ) {
        int idx = indices[i] ;

        if ( idx == -1 ) {
            for(int j=1 ; j<face_vertices.size() - 1 ; j++) {
                indices_.push_back(face_vertices[0]) ;
                indices_.push_back(face_vertices[j+1]) ;
                indices_.push_back(face_vertices[j]) ;
            }

            face_vertices.clear() ;
        }
        else {
            face_vertices.push_back(idx) ;
        }
    }

    compute_normals(positions_, indices_, normals_) ;

}


static float saasin(float fac)
{
    if      (fac <= -1.0f) return (float)-M_PI / 2.0f;
    else if (fac >=  1.0f) return (float) M_PI / 2.0f;
    else return asinf(fac);
}

static float angle_normalized_v3v3(const Vector3f &v1, const Vector3f &v2)
{
    /* this is the same as acos(dot_v3v3(v1, v2)), but more accurate */
    if ( v1.dot(v2) < 0 ) {

        Vector3f vec(-v2) ;
        return (float)M_PI - 2.0f * (float)saasin((vec -v1).norm() / 2.0f);
    }
    else
        return 2.0f * (float)saasin((v2-v1).norm() / 2.0f);
}

static Vector3f normal_triangle(const Vector3f &v1, const Vector3f &v2, const Vector3f &v3)
{
    Vector3f n1, n2 ;

    n1 = v1 - v2 ;
    n2 = v1 - v3 ;
    return  n1.cross(n2).normalized() ;

}

static Vector3f normal_quad(const Vector3f &v1, const Vector3f &v2, const Vector3f &v3, const Vector3f &v4)
{
    Vector3f n1, n2 ;

    n1 = v1 - v3 ;
    n2 = v2 - v4 ;

    return  n1.cross(n2).normalized() ;
}

/* Newell's Method */
static Vector3f calc_ngon_normal(const vector<Vector3f> &vtx)
{
    const int nverts = vtx.size() ;

    Vector3f normal = Vector3f::Zero(), v_curr, v_prev = vtx.back() ;

    for (int i = 0; i < nverts; i++) {
        v_curr = vtx[i] ;
        normal += (v_prev - v_curr).cross(v_prev + v_curr) ;
        v_prev = v_curr;
    }

    if ( normal.norm() == 0.0f ) return Vector3f(0, 0, 1.0f);
    else  return normal.normalized() ;
}

static Vector3f calc_face_normal(vector<Vector3f> &vtx)
{
    if ( vtx.size() > 4 ) return calc_ngon_normal(vtx);
    else if ( vtx.size() == 3 ) return normal_triangle(vtx[0], vtx[1], vtx[2]) ;
    else if ( vtx.size() == 4 ) return normal_quad(vtx[0], vtx[1], vtx[2], vtx[3]) ;
    else return Vector3f(0, 0, 1.0) ;
}

static void compute_normals(const vector<Vector3f> &vertices, const vector<uint> &indices, vector<Vector3f> &vtx_normals)
{
    vtx_normals.resize(vertices.size()) ;
    for( int i=0 ; i<vertices.size() ; i++ ) vtx_normals[i] = Vector3f::Zero() ;

    for( int i=0 ; i<indices.size() ; i+=3 )
    {
        uint idx0 = indices[i] ;
        uint idx1 = indices[i+1] ;
        uint idx2 = indices[i+2] ;
        Vector3f n = normal_triangle(vertices[idx0], vertices[idx1], vertices[idx2]) ;

        vtx_normals[idx0] += n ;
        vtx_normals[idx1] += n ;
        vtx_normals[idx2] += n ;
    }

    for( int i=0 ; i<vertices.size() ; i++ ) vtx_normals[i].normalize() ;

}

#ifndef CVX_VIZ_MESH_HPP
#define CVX_VIZ_MESH_HPP

#include <cvx/viz/scene/scene_fwd.hpp>
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/util/geometry/point_list.hpp>
#include <cvx/viz/scene/bone.hpp>

#include <string>
#include <vector>
#include <memory>
#include <map>


#include <Eigen/Core>

namespace cvx { namespace viz {

namespace detail {
    class Octree ;
}

#define MAX_TEXTURES 4

template<typename T, int D>
class VertexBuffer {
public:

    using data_container_t = cvx::util::PointList<T, D> ;
    using index_container_t = std::vector<uint32_t> ;

    VertexBuffer() = default ;
    VertexBuffer(std::initializer_list<T> &vdata, std::initializer_list<T> &indices = {}): data_(data), indices_(indices) {}

    // indicates that the attribute is dynamically updated
    void setDynamicData(bool d) {
        is_dynamic_ = d ;
    }

    bool isDynamic() const { return is_dynamic_ ; }

    data_container_t &data() { return data_ ; }
    index_container_t &indices() { return indices_ ; }

    const data_container_t &data() const { return data_ ; }
    const index_container_t &indices() const { return indices_ ; }

    bool hasIndices() const { return !indices_.empty() ; }

protected:

    bool is_dynamic_ = false ;
    data_container_t data_ ;
    index_container_t indices_ ;
};

class Mesh: public std::enable_shared_from_this<Mesh> {
public:

    enum PrimitiveType { Triangles, Lines, Points } ;

    Mesh(PrimitiveType t): ptype_(t) {}
     ~Mesh();

    using vb3_t = VertexBuffer<float, 3> ;
    using vb2_t = VertexBuffer<float, 2> ;

    vb3_t &vertices() { return vertices_ ; }
    vb3_t &normals() { return normals_ ; }
    vb3_t &colors() { return colors_ ; }
    vb2_t &texCoords(uint t) {
        assert(t<MAX_TEXTURES) ;
        return tex_coords_[t] ;
    }

    const vb3_t &vertices() const { return vertices_ ; }
    const vb3_t &normals() const { return normals_ ; }
    const vb3_t &colors() const { return colors_ ; }
    const vb2_t &texCoords(uint t) const {
        assert(t<MAX_TEXTURES) ;
        return tex_coords_[t] ;
    }

    static const int MAX_BONES_PER_VERTEX = 4 ;

    struct BoneWeight {
        int bone_[MAX_BONES_PER_VERTEX] ;
        float weight_[MAX_BONES_PER_VERTEX] ;

        BoneWeight() {
            std::fill(bone_, bone_ + MAX_BONES_PER_VERTEX, -1) ;
            std::fill(weight_, weight_ + MAX_BONES_PER_VERTEX, 0.f) ;
        }

        void add(int bone, float w) {
            int idx = 0 ;
            while ( bone_[idx] != -1 && idx < MAX_BONES_PER_VERTEX ) ++idx ;
            bone_[idx] = bone ;
            weight_[idx] = w ;
        }

        void normalize() {
            float w = 0.0 ;

               for(int i=0 ; i<MAX_BONES_PER_VERTEX ; i++) {
                   if ( bone_[i] < 0 ) break ;
                   w += weight_[i] ;
               }

               if ( w == 0.0 ) return ;

               for(int i=0 ; i<MAX_BONES_PER_VERTEX ; i++) {
                   if ( bone_[i] < 0 ) break ;
                   weight_[i] /= w  ;
               }
        }
    };

    const std::vector<Bone> &skeleton() const { return skeleton_ ; }
    std::vector<Bone> &skeleton() { return skeleton_ ; }

    Eigen::Affine3f &skeletonInverseGlobalTransform() { return skeleton_inverse_global_transform_ ; }

    const std::vector<BoneWeight> &weights() const { return weights_ ; }
    std::vector<BoneWeight> &weights() { return weights_ ; }

    bool hasSkeleton() const { return !skeleton_.empty() ; }

    PrimitiveType ptype() const { return ptype_ ; }

    // it is a simple triangle mesh with per-pertex attributes
    bool isSimpleIndexed() const ;

    // primitive shape factories

    static MeshPtr createWireCube(const Eigen::Vector3f &hs) ;
    static MeshPtr createSolidCube(const Eigen::Vector3f &hs) ;

    static MeshPtr createWireSphere(float radius, size_t slices, size_t stacks) ;
    static MeshPtr createSolidSphere(float radius, size_t slices, size_t stacks) ;

    // the base of the cone is on (0, 0, 0) aligned with the z-axis and pointing towards positive z

    static MeshPtr createWireCone(float radius, float height, size_t slices, size_t stacks) ;
    static MeshPtr createSolidCone(float radius, float height, size_t slices, size_t stacks) ;

    static MeshPtr createWireCylinder(float radius, float height, size_t slices, size_t stacks) ;
    static MeshPtr createSolidCylinder(float radius, float height, size_t slices, size_t stacks) ;

    static MeshPtr makePointCloud(const cvx::util::PointList3f &pts) ;
    static MeshPtr makePointCloud(const cvx::util::PointList3f &coords, const cvx::util::PointList3f &clrs) ;

    void computeNormals() ;
    void computeBoundingBox(Eigen::Vector3f &bmin, Eigen::Vector3f &bmax) const ;
    void makeOctree() ;

//    bool intersect(const cvx::viz::Ray &ray, float &t) const override ;

    // create a new mesh without indices
    static MeshPtr flatten(const MeshPtr &src) ;

    // get underlying OpenGL buffer data

    detail::MeshData* getMeshData() {
        if ( !data_ )
            makeMeshData() ;
        return data_.get() ;
    }

     bool intersect(const Ray &ray, float &t) const;


private:

    vb3_t vertices_, normals_, colors_ ;
    vb2_t tex_coords_[MAX_TEXTURES] ;
    std::vector<BoneWeight> weights_ ;
    std::vector<Bone> skeleton_ ;
    Eigen::Affine3f skeleton_inverse_global_transform_ = Eigen::Affine3f::Identity() ;

    PrimitiveType ptype_ ;
    detail::Octree *octree_ = nullptr ;

    void makeMeshData()  ;

    std::shared_ptr<detail::MeshData> data_ = nullptr ;

};

} // namespavce viz
} // namespace cvx
#endif

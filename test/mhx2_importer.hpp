#ifndef __MHX2_IMPORTER__
#define __MHX2_IMPORTER__

#include <vector>
#include <map>
#include <deque>
#include <Eigen/Core>

#include <cvx/misc/json_reader.hpp>

struct MHX2Bone {
    Eigen::Vector3f head_, tail_ ;
    Eigen::Matrix4f bmat_ ; // rotation to align Y axis with the bone
    std::string parent_ ;
    float roll_ ;
};

struct MHX2VertexGroup {
    std::vector<int> idxs_ ;
    std::vector<float> weights_ ;
};

#define MAX_VERTICES_PER_FACE 4
#define UNKNOWN_MATERIAL_INDEX 0xffff

struct MHX2Face {
    MHX2Face(const std::vector<uint> &idx): material_index_(UNKNOWN_MATERIAL_INDEX) {
        assert(idx.size() <= MAX_VERTICES_PER_FACE) ;
        num_vertices_ = idx.size() ;
        for(int i=0 ; i<num_vertices_ ; i++)
            indices_[i] = idx[i] ;
    }

    uint num_vertices_ ;
    uint indices_[MAX_VERTICES_PER_FACE] ;
    uint material_index_ ;
    Eigen::Vector2f tex_coords_[MAX_VERTICES_PER_FACE] ;

};

struct MHX2Mesh {
    std::vector<Eigen::Vector3f> vertices_ ;
    std::vector< MHX2Face > faces_ ;
    std::vector<Eigen::Vector2f> tex_coords_ ;
    std::map<std::string, MHX2VertexGroup> groups_ ;
};

class Mhx2Importer {
public:
    Mhx2Importer() = default ;

    bool load(const std::string &fname, const std::string &meshName) ;

private:

    using JSONReader = cvx::JSONReader ;

    bool parseSkeleton(JSONReader &r) ;
    bool parseGeometries(JSONReader &v, const std::string &meshName) ;
    bool parseMesh(const std::string &name, JSONReader &v, const Eigen::Vector3f &of, float scale) ;
    bool parseVertexGroups(MHX2Mesh &mesh, JSONReader &v) ;

    std::map<std::string, MHX2Bone> bones_ ;
    std::map<std::string, MHX2Mesh> meshes_ ;
};

#endif

#ifndef MHX2_MODEL_HPP
#define MHX2_MODEL_HPP

#include <Eigen/Core>
#include <vector>
#include <map>
#include <string>

struct MHBone {
    Eigen::Vector3f head_, tail_ ;
    Eigen::Matrix4f bmat_ ; // rotation to align Y axis with the bone
    std::string parent_ ;
    float roll_ ;
};

struct MHVertexGroup {
    std::vector<int> idxs_ ;
    std::vector<float> weights_ ;
};

#define MAX_VERTICES_PER_FACE 4
#define UNKNOWN_MATERIAL_INDEX 0xffff

struct MHFace {
    MHFace(const std::vector<uint> &idx): material_index_(UNKNOWN_MATERIAL_INDEX) {
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

struct MHMesh {
    std::vector<Eigen::Vector3f> vertices_ ;
    std::vector<MHFace> faces_ ;
    std::vector<Eigen::Vector2f> tex_coords_ ;
    std::map<std::string, MHVertexGroup> groups_ ;
};

struct MHModel {
    std::map<std::string, MHBone> bones_ ;
    std::map<std::string, MHMesh> meshes_ ;
};

#endif

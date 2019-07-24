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

struct MHGeometry {
    Eigen::Vector3f offset_ ;
    float scale_ ;
    std::string material_ ;
    MHMesh mesh_ ;
};

struct MHMaterial {
    Eigen::Vector3f diffuse_color_ ;
    float diffuse_map_intensity_ ;
    Eigen::Vector3f specular_color_ ;
    float specular_map_intensity_ ;
    float shininess_ ;
    float opacity_ ;
    float translucency_ ;
    Eigen::Vector3f emissive_color_ ;
    Eigen::Vector3f ambient_color_ ;
    float transparency_map_intensity_ ;
    bool shadeless_ ;
    bool wireframe_ ;
    bool transparent_ ;
    bool alpha_to_coverage_  ;
    bool backface_cull_ ;
    bool depthless_ ;
    bool cast_shadows_ ;
    bool recieve_shadows_ ;
    bool sss_enabled_ ;
    float sss_R_scale_, sss_G_scale_, sss_B_scale_ ;
};

struct MHModel {
    std::map<std::string, MHBone> bones_ ;
    std::map<std::string, MHGeometry> geometries_ ;
    std::map<std::string, MHMaterial> materials_ ;
};


#endif

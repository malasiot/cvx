#include "mh_scene.hpp"
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/viz/scene/mesh.hpp>
#include <cvx/viz/scene/material.hpp>

#include <numeric>

using namespace cvx::viz ;
using namespace std ;
using namespace Eigen ;

static void computeTransformsRecursive(NodePtr n, Affine3f pmat)
{
    Node *parent = n->getParent() ;
    Affine3f ioffset = n->matrix() ;
    if ( parent == nullptr )
        n->matrix() = ioffset ;
    else
        n->matrix() =  pmat.inverse() * ioffset  ;

    for( NodePtr child: n->children())
    {
        computeTransformsRecursive(child, ioffset) ;
    }

}

MHNode::MHNode(const MHModel &model) {
    createGeometry(model) ;
}

void MHNode::createGeometry(const MHModel &model)
{


    // make skeleton

    std::map<string, int> bone_map ;
    vector<Bone> bones ;
    vector<int> root_bones ;

    for( const auto &kv: model.bones_ ) {
        const MHBone &bone = kv.second ;
        bone_map[kv.first] = bones.size()  ;

        Bone b ;
        b.name_ = kv.first ;
        b.offset_ = bone.bmat_.inverse() ;
        b.node_ = make_shared<Node>() ;
        b.node_->setName(b.name_) ;
        b.node_->matrix() = bone.bmat_ ;

        bones.push_back(b) ;
    }

    // build hierarchy

    for( const auto &kv: model.bones_ ) {
        const MHBone &bone = kv.second ;
        string name = kv.first ;

        int idx = bone_map[name] ;

        if ( bone.parent_.empty() ) root_bones.push_back(idx) ;
        else {
            int pidx = bone_map[bone.parent_] ;
            bones[pidx].node_->addChild(bones[idx].node_) ;
        }
    }

    // compute relative bone transforms

    for( int i: root_bones ) {
        computeTransformsRecursive(bones[i].node_, Affine3f::Identity()) ;
        addChild(bones[i].node_) ;
    }

    // iterate over all meshes

    for( const auto &mesh_kv: model.meshes_) {
        const MHMesh &mesh = mesh_kv.second ;

        std::shared_ptr<Mesh> m(new Mesh(Mesh::Triangles)) ;

        vector<int> indices ;
        vector<Vector2f> tcoords ;
        vector<int> face_vertices ;
        vector<Vector2f> face_tcoords ;

        m->vertices().data().insert( m->vertices().data().end(), mesh.vertices_.begin(), mesh.vertices_.end()  ) ;

        // add faces

        for(int i=0 ; i<mesh.faces_.size() ; i++)
        {
            const MHFace &f = mesh.faces_[i] ;


            for(int k=0 ; k<f.num_vertices_ ; k++)
            {
                uint idx = f.indices_[k] ;

                indices.push_back(idx) ;
                tcoords.push_back(f.tex_coords_[k]);
            }

            indices.push_back(-1) ;
        }

        // bone weights

        size_t n = m->vertices().data().size() ;

        m->weights().resize(n) ;

        m->skeleton().insert(m->skeleton().end(), bones.begin(), bones.end()) ;

        // collect weights per vertex, we need then to normalize and keep only max 4 weights

        vector<vector<int>> vg_idxs(n) ;
        vector<vector<float>> vg_weights(n) ;

        for ( const auto &kv: mesh.groups_ ) {
            const string &name = kv.first ;
            const MHVertexGroup &group = kv.second ;

            int bidx ;
            auto it = bone_map.find(name) ;
            if ( it == bone_map.end() ) continue ;
            else bidx = it->second ;

            if ( bidx != -1 )  {

                for( int i=0 ; i<group.idxs_.size() ; i++ )  {
                    int idx = group.idxs_.at(i) ;

                    if ( idx < 0 ) continue ;

                    vg_idxs[idx].push_back(bidx) ;
                    vg_weights[idx].push_back(group.weights_[i]) ;

                }
            }
        }

        for( int i=0 ; i<n ; i++ ) {
            auto &idxs = vg_idxs[i] ;
            auto &weights = vg_weights[i] ;

            auto &dest = m->weights()[i] ;

            // trim if neccessery
            if ( weights.size() > Mesh::MAX_BONES_PER_VERTEX ) {
                // sort

                // initialize original index locations
                  vector<size_t> sort_idx(weights.size());
                  std::iota(sort_idx.begin(), sort_idx.end(), 0);

                  // sort indexes based on comparing values in v
                  std::sort(sort_idx.begin(), sort_idx.end(),
                       [&weights](size_t i1, size_t i2) {
                           return weights[i1] > weights[i2];});

                  vector<int> trimmed_idxs ;
                  vector<float> trimmed_weights ;
                  for( size_t k=0 ; k<Mesh::MAX_BONES_PER_VERTEX ; k++ ) {
                      trimmed_idxs.push_back(idxs[sort_idx[k]]) ;
                      trimmed_weights.push_back(weights[sort_idx[k]]) ;
                  }

                  idxs = trimmed_idxs ;
                  weights = trimmed_weights ;
            }

            // normalize

            float w = 0.0 ;
            for ( float weight: weights ) w += weight ;
            for ( float &weight: weights ) weight /= w ;

            // copy to destination

            for( size_t k=0 ; k<idxs.size() ; k++ ) {
                dest.bone_[k] = idxs[k] ;
                dest.weight_[k] = weights[k] ;
            }

        }


        // split faces into triangles
        int c = 0 ;

        for( int i=0 ; i<indices.size() ; i++ )
        {
            int idx = indices[i] ;

            if ( idx == -1 )
            {
                for(int j=1 ; j<face_vertices.size() - 1 ; j++)
                {
                    m->vertices().indices().push_back(face_vertices[0]) ;
                    m->vertices().indices().push_back(face_vertices[j+1]) ;
                    m->vertices().indices().push_back(face_vertices[j]) ;

                    m->texCoords(0).data().push_back(face_tcoords[0]) ;
                    m->texCoords(0).data().push_back(face_tcoords[j+1]) ;
                    m->texCoords(0).data().push_back(face_tcoords[j]) ;
                }

                face_vertices.clear() ;
                face_tcoords.clear() ;

            }
            else {
                face_vertices.push_back(idx) ;
                face_tcoords.push_back(tcoords[c++]) ;
            }
        }

        m->computeNormals();

        auto *material = new PhongMaterialInstance() ;
        material->setDiffuse({0, 1.0, 0.0, 1.0}) ;
       material->setFlags(USE_SKINNING) ;
        MaterialInstancePtr material_instance(material) ;

        std::shared_ptr<MeshGeometry> geom(new MeshGeometry(m)) ;

        std::shared_ptr<Drawable> drawable(new Drawable(geom, material_instance)) ;
        addDrawable(drawable);
    }



}


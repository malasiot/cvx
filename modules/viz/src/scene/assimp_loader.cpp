#include <cvx/viz/scene/scene.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/drawable.hpp>
#include <cvx/viz/scene/mesh.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/material.hpp>

#include <cvx/viz/animation/animation.hpp>

#include <cvx/util/misc/path.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <Eigen/Dense>

#include <cvx/util/misc/optional.hpp>

using namespace std ;
using namespace Eigen ;
using cvx::util::optional ;
using cvx::util::Path ;

static Vector4f color4_to_float4(const aiColor4D &c) {
    return Vector4f(c.r, c.g, c.b, c.a) ;
}

namespace cvx { namespace viz { namespace impl {


class AssimpImporter {
public:
    AssimpImporter(Node &sc, int options, float scale): scene_(sc), options_(options), scale_(scale) {}

    MaterialInstancePtr importMaterial(const struct aiMaterial *mtl, const string &model_path) ;

    Node &scene_ ;

    map<const aiMesh *, MeshPtr> meshes_ ;
    map<const aiMaterial *, MaterialInstancePtr> materials_ ;
    map<string, LightPtr> lights_ ;
    map<string, CameraPtr> cameras_ ;
    map<string, NodePtr> node_map_ ;
    int options_ ;
    float scale_ ;

    bool importMaterials(const string &mpath, const aiScene *sc);
    bool importMeshes(const aiScene *sc);
    bool importLights(const aiScene *sc);
    bool importAnimations(const aiScene *sc) ;
    bool importNodes(NodePtr &pnode, const aiScene *sc, const aiNode *nd);
    bool import(const aiScene *sc, const std::string &fname);
    bool findSkeletonHierarchies();
};

static void getPhongMaterial(const struct aiMaterial *mtl,
                             optional<Vector4f> &vambient,
                             optional<Vector4f> &vdiffuse,
                             optional<Vector4f> &vspecular,
                             optional<float> &vshininess) {

    aiColor4D diffuse, specular, ambient;
    float shininess, strength;
    unsigned int max;

    if ( AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
        vdiffuse = color4_to_float4(diffuse);

    if ( AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular) )
        vspecular = color4_to_float4(specular) ;

    if ( AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient) )
        vambient = color4_to_float4(ambient) ;

    max = 1;
    aiReturn ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);

    if ( ret1 == AI_SUCCESS ) {
        max = 1;
     //   aiReturn ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
     //   if(ret2 == AI_SUCCESS) shininess = shininess * strength ;

        vshininess = shininess ;
    }
    else {
        vshininess = 0 ;
        vspecular = Vector4f(0, 0, 0, 1) ;
    }
}

static void getMaterialTexture(const struct aiMaterial *mtl, optional<Texture2D> &texture, const string &model_path) {
    Path mpath(model_path) ;

    aiString tex_path ;
    aiTextureMapping tmap ;
    aiTextureMapMode mode ;

    if ( AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &tex_path, &tmap, 0, 0, 0, &mode) ) {
        string file_name(tex_path.data, tex_path.length) ;

        string url = "file://" + (mpath.parentPath() / file_name).toString() ;
        Texture2D sampler(url) ;
        texture = sampler ;
    }
}


MaterialInstancePtr AssimpImporter::importMaterial(const struct aiMaterial *mtl, const string &model_path) {

    int shading_model ;
    mtl->Get((const char *)AI_MATKEY_SHADING_MODEL, shading_model);

    optional<Texture2D> diffuse_map ;
    optional<Vector4f> ambient, diffuse, specular ;
    optional<float> shininess ;

    getMaterialTexture(mtl, diffuse_map, model_path) ;
    getPhongMaterial(mtl, ambient, diffuse, specular, shininess) ;

    if ( diffuse_map ) {
        DiffuseMapMaterialInstance *mat = new DiffuseMapMaterialInstance(diffuse_map.value()) ;
        if ( ambient ) mat->setAmbient(ambient.value()) ;
        mat->setDiffuse(diffuse_map.value()) ;
        if ( specular ) mat->setSpecular(specular.value()) ;
        if ( shininess ) mat->setShininess(shininess.value()) ;
        return MaterialInstancePtr(mat) ;
    } else {
        PhongMaterialInstance *mat = new PhongMaterialInstance() ;
        if ( ambient ) mat->setAmbient(ambient.value()) ;
        if ( diffuse ) mat->setDiffuse(diffuse.value()) ;
        if ( specular ) mat->setSpecular(specular.value()) ;
        if ( shininess ) mat->setShininess(shininess.value()) ;
        return MaterialInstancePtr(mat) ;
    }

}

bool AssimpImporter::importMaterials(const string &mpath, const aiScene *sc) {
    for( uint m=0 ; m<sc->mNumMaterials ; m++ ) {
        const aiMaterial *material = sc->mMaterials[m] ;
        MaterialInstancePtr smat = importMaterial(material, mpath) ;
        materials_[material] = smat ;
    }

    return true ;
}

bool AssimpImporter::importMeshes(const aiScene *sc) {

    for( uint m=0 ; m<sc->mNumMeshes ; m++ ) {
        const aiMesh *mesh = sc->mMeshes[m] ;

        //   if ( mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE ) continue ;

        MeshPtr smesh ;

        if ( mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE ) smesh.reset(new Mesh(Mesh::Triangles)) ;
        else if ( mesh->mPrimitiveTypes == aiPrimitiveType_LINE ) smesh.reset(new Mesh(Mesh::Lines)) ;
        else if ( mesh->mPrimitiveTypes == aiPrimitiveType_POINT ) smesh.reset(new Mesh(Mesh::Points)) ;
        else continue ;

        if ( mesh->HasPositions() ) {
            uint n = mesh->mNumVertices ;
            smesh->vertices().data().resize(n) ;

            for(int i = 0; i < n; ++i)
                smesh->vertices().data()[i] = Vector3f(scale_ * mesh->mVertices[i].x, scale_ * mesh->mVertices[i].y, scale_ * mesh->mVertices[i].z) ;
        }

        if ( mesh->HasNormals() ) {
            uint n = mesh->mNumVertices ;
            smesh->normals().data().resize(n) ;

            for(int i = 0; i < n; ++i)
                smesh->normals().data()[i] = Vector3f(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) ;
        }

        if ( mesh->HasVertexColors(0) ) {
            uint n = mesh->mNumVertices ;
            smesh->colors().data().resize(n) ;

            for(int i = 0; i < n; ++i)
                smesh->colors().data()[i] = Vector3f(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b) ;
        }

        for( uint t=0 ; t<MAX_TEXTURES ; t++ ) {
            uint n = mesh->mNumVertices ;
            if ( mesh->HasTextureCoords(t) ) {
                smesh->texCoords(t).data().resize(n) ;

                for(int i = 0; i < n; ++i)
                    smesh->texCoords(t).data()[i] = Vector2f(mesh->mTextureCoords[t][i].x, mesh->mTextureCoords[t][i].y) ;
            }
        }

        if ( mesh->HasFaces() ) {
            uint n = mesh->mNumFaces ;
            smesh->vertices().indices().resize(n * 3) ;

            for(int i = 0, k=0; i < n ; i++) {
                smesh->vertices().indices()[k++] = mesh->mFaces[i].mIndices[0];
                smesh->vertices().indices()[k++] = mesh->mFaces[i].mIndices[1];
                smesh->vertices().indices()[k++] = mesh->mFaces[i].mIndices[2];
            }
        }

        if ( smesh->ptype() == Mesh::Triangles && ( options_ & Scene::MAKE_PICKABLE ) )
            smesh->makeOctree() ;

        if ( ( options_ & Scene::IMPORT_SKELETONS ) && mesh->HasBones() ) {
            std::map<string, int> bone_map ;

            int num_bones = mesh->mNumBones ;
            smesh->skeleton().resize(num_bones) ;

            smesh->weights().resize(mesh->mNumVertices) ;

            for( size_t i=0 ; i<num_bones ; i++ ) {
                aiBone *bone = mesh->mBones[i] ;
                string name = bone->mName.C_Str() ;

                Bone &b = smesh->skeleton().at(i) ;
                b.name_.assign(std::move(name)) ;

                aiMatrix4x4 m = bone->mOffsetMatrix;

                b.offset_.matrix() << m.a1, m.a2, m.a3, m.a4,
                        m.b1, m.b2, m.b3, m.b4,
                        m.c1, m.c2, m.c3, m.c4,
                        m.d1, m.d2, m.d3, m.d4 ;

                // copy weights

                for( size_t j=0 ; j<bone->mNumWeights ; j++ ) {
                    aiVertexWeight &weight = bone->mWeights[j] ;
                    Mesh::BoneWeight &w =   smesh->weights().at(weight.mVertexId) ;
                    size_t idx ;
                    for( idx = 0 ; w.bone_[idx] != -1 && idx < Mesh::MAX_BONES_PER_VERTEX ; ++idx ) ;
                    assert(idx < Mesh::MAX_BONES_PER_VERTEX) ;
                    w.bone_[idx] = i ;
                    w.weight_[idx] = weight.mWeight ;
                }

            }

        }

        meshes_[mesh] = smesh ;
    }

    return true ;
}

bool AssimpImporter::importLights(const aiScene *sc) {
    for( uint m=0 ; m<sc->mNumLights ; m++ ) {
        const aiLight *light = sc->mLights[m] ;

        LightPtr slight ;

        switch ( light->mType ) {
        case aiLightSource_DIRECTIONAL:
        {
            DirectionalLight *dl = new DirectionalLight({light->mDirection.x, light->mDirection.y, light->mDirection.z}) ;
            dl->diffuse_color_ << light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b ;
            dl->specular_color_ << light->mColorSpecular.r, light->mColorSpecular.g, light->mColorSpecular.b ;
            dl->ambient_color_ << light->mColorAmbient.r, light->mColorAmbient.g, light->mColorAmbient.b ;

            slight.reset(dl) ;
            break ;
        }
        case aiLightSource_POINT:
        {
            PointLight *pl = new PointLight({light->mPosition.x, light->mPosition.y, light->mPosition.z}) ;

            pl->diffuse_color_ << light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b ;
            pl->specular_color_ << light->mColorSpecular.r, light->mColorSpecular.g, light->mColorSpecular.b ;
            pl->ambient_color_ << light->mColorAmbient.r, light->mColorAmbient.g, light->mColorAmbient.b ;

            pl->constant_attenuation_ = light->mAttenuationConstant ;
            pl->linear_attenuation_ = light->mAttenuationLinear ;
            pl->quadratic_attenuation_ = light->mAttenuationQuadratic ;

            slight.reset(pl) ;
            break ;

        }
        case aiLightSource_SPOT:
        {
            SpotLight *sl = new SpotLight({light->mPosition.x, light->mPosition.y, light->mPosition.z},
                                    {light->mDirection.x, light->mDirection.y, light->mDirection.z}) ;

            sl->diffuse_color_ << light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b ;
            sl->specular_color_ << light->mColorSpecular.r, light->mColorSpecular.g, light->mColorSpecular.b ;
            sl->ambient_color_ << light->mColorAmbient.r, light->mColorAmbient.g, light->mColorAmbient.b ;

            sl->constant_attenuation_ = light->mAttenuationConstant ;
            sl->linear_attenuation_ = light->mAttenuationLinear ;
            sl->quadratic_attenuation_ = light->mAttenuationQuadratic ;
            sl->falloff_angle_ = light->mAngleOuterCone ;
            sl->falloff_exponent_ = 0 ;

            slight.reset(sl) ;
            break ;

        }

        }

        if ( slight ) {
            slight->name_ = light->mName.C_Str() ;
            lights_[slight->name_] = slight ;
        }
    }

    return true ;
}

struct ChannelAffine {
    TimeLineChannel<Vector3f> translation_, scaling_ ;
    TimeLineChannel<Quaternionf> rotation_ ;
};

class NodeAnimation: public Animation {
public:

    void update(float t) override {
        Animation::update(t) ;

        if ( isRunning() ) {
            for( const auto &lp: node_map_ ) {
                const ChannelAffine &tr = *lp.first ;
                NodePtr node = lp.second ;

                Affine3f mat = Translation3f(tr.translation_.getValue()) * tr.rotation_.getValue() * Scaling(tr.scaling_.getValue()) ;

                node->matrix() = mat ;
            }
        }
    }


    std::vector<ChannelAffine> channels_ ;
    std::map<ChannelAffine *, NodePtr> node_map_ ;
};

bool AssimpImporter::importAnimations(const aiScene *sc)
{
    for( int i=0 ; i< sc->mNumAnimations ; i++ ) {
        aiAnimation *anim = sc->mAnimations[i] ;

        NodeAnimation *animation = new NodeAnimation() ;
        animation->setDuration(anim->mDuration * 1000.0/anim->mTicksPerSecond) ;

        scene_.addAnimation(animation) ;

        animation->channels_.resize(anim->mNumChannels) ;

        for( int j = 0 ; j<anim->mNumChannels ; j++ ) {
            aiNodeAnim *channel = anim->mChannels[j] ;

            string node_name(channel->mNodeName.C_Str()) ;
            auto it = node_map_.find(node_name) ;
            if ( it == node_map_.end() ) continue ;

            NodePtr node = it->second ;

            ChannelAffine &ca = animation->channels_[j] ;

            TimeLineChannel<Vector3f> &translation = ca.translation_ ;

            if ( channel->mNumPositionKeys == 1 ) {
                const aiVectorKey &key = channel->mPositionKeys[0] ;
                translation.getTimeLine().addKeyFrame(0.0, Vector3f(key.mValue.x, key.mValue.y, key.mValue.z)) ;
                translation.getTimeLine().addKeyFrame(1.0, Vector3f(key.mValue.x, key.mValue.y, key.mValue.z)) ;
            } else {
                for( uint k=0 ; k<channel->mNumPositionKeys ; k++ ) {
                    const aiVectorKey &key = channel->mPositionKeys[k] ;
                    translation.getTimeLine().addKeyFrame(key.mTime/anim->mDuration, Vector3f(key.mValue.x, key.mValue.y, key.mValue.z)) ;
                }
            }

            animation->addChannel(&ca.translation_) ;

            TimeLineChannel<Vector3f> &scaling = ca.scaling_ ;

            if ( channel->mNumScalingKeys == 1 ) {
                const aiVectorKey &key = channel->mScalingKeys[0] ;
                scaling.getTimeLine().addKeyFrame(0.0, Vector3f(key.mValue.x, key.mValue.y, key.mValue.z)) ;
                scaling.getTimeLine().addKeyFrame(1.0, Vector3f(key.mValue.x, key.mValue.y, key.mValue.z)) ;
            } else {
                for( uint k=0 ; k<channel->mNumScalingKeys ; k++ ) {
                    const aiVectorKey &key = channel->mScalingKeys[k] ;
                    scaling.getTimeLine().addKeyFrame(key.mTime/anim->mDuration, Vector3f(key.mValue.x, key.mValue.y, key.mValue.z)) ;
                }
            }

            animation->addChannel(&ca.scaling_) ;

            TimeLineChannel<Quaternionf> &rotation = ca.rotation_ ;

            if ( channel->mNumRotationKeys == 1 ) {
                const aiQuatKey &key = channel->mRotationKeys[0] ;
                rotation.getTimeLine().addKeyFrame(0.0, Quaternionf(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)) ;
                rotation.getTimeLine().addKeyFrame(1.0, Quaternionf(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)) ;
            } else {
                for( uint k=0 ; k<channel->mNumScalingKeys ; k++ ) {
                    const aiQuatKey &key = channel->mRotationKeys[k] ;
                    rotation.getTimeLine().addKeyFrame(key.mTime/anim->mDuration, Quaternionf(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)) ;
                }
            }

            animation->addChannel(&ca.rotation_) ;

            animation->node_map_.emplace(&ca, node) ;
        }

    }

    return true ;


}

bool AssimpImporter::importNodes(NodePtr &pnode, const struct aiScene *sc, const struct aiNode* nd)
{
    unsigned int i;
    unsigned int n = 0, t;
    aiMatrix4x4 m = nd->mTransformation;
    /* update transform */

    NodePtr snode(new Node) ;



    Matrix4f tf ;
    tf << m.a1, m.a2, m.a3, m.a4,
            m.b1, m.b2, m.b3, m.b4,
            m.c1, m.c2, m.c3, m.c4,
            m.d1, m.d2, m.d3, m.d4 ;

    snode->matrix() = tf.eval() ;

    string nname(nd->mName.C_Str()) ;
    snode->setName(nname);

     node_map_.emplace(nname, snode) ;

    /* draw all meshes assigned to this node */
    for (; n < nd->mNumMeshes; ++n) {

        const aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

        map<const aiMesh *, MeshPtr>::const_iterator mit = meshes_.find(mesh) ;

        if ( mit == meshes_.end() ) continue ;

        GeometryPtr geom(new MeshGeometry(mit->second)) ;

        const aiMaterial* material = sc->mMaterials[mesh->mMaterialIndex];

        map<const aiMaterial *, MaterialInstancePtr>::const_iterator cit = materials_.find(material) ;

        MaterialInstancePtr mat ;

        if ( cit != materials_.end() ) {
            mat = cit->second ;
            if ( mesh->HasBones() )
                mat->setFlags(USE_SKINNING) ;
        }

        DrawablePtr dr(new Drawable(geom, mat)) ;

        snode->addDrawable(dr) ;
    }

    auto lit = lights_.find(nname) ;
    if ( lit != lights_.end() )  // this is a light node
        snode->addLight(lit->second) ;

    auto camit = cameras_.find(nname) ;
    if ( camit != cameras_.end() )  ;// this is a camera node, no special handling

    if ( pnode ) pnode->addChild(snode) ;

    /* import all children */
    for (n = 0; n < nd->mNumChildren; ++n) {
        if ( !importNodes(snode, sc, nd->mChildren[n]) )
            return false ;
    }

    pnode->setPickable(options_ & Scene::MAKE_PICKABLE ) ;

    return true ;
}

// map bone names to nodes in the graph

bool AssimpImporter::findSkeletonHierarchies() {
    for( auto lp: meshes_ ) {
        MeshPtr &mesh = lp.second ;
        if ( mesh->hasSkeleton() ) {
            auto &skeleton = mesh->skeleton() ;
            std::set<Node *> nodes ;
            for ( Bone &b: skeleton ) {
                auto it = node_map_.find(b.name_) ;
                if ( it == node_map_.end() ) return false ;
                NodePtr node = it->second ;
                b.node_ = node ;
                nodes.insert(node.get()) ;
            }
            // find node parent
            for( Node *n: nodes ) {
                Node *parent = n->getParent() ;
                if ( parent == nullptr ) break ;
                if ( nodes.count(parent) == 0 ) {
                    mesh->skeletonInverseGlobalTransform() = parent->globalTransform().inverse() ;
                    break ;
                }
            }
        }

    }

    return true ;
}

bool AssimpImporter::import(const aiScene *sc, const std::string &fname) {

    if ( !importMeshes(sc) ) return false ;
    if ( !importMaterials(fname, sc) ) return false ;

    if ( options_ & Scene::IMPORT_LIGHTS ) {
        if ( !importLights(sc) ) return false ;
    }

    NodePtr root = scene_.shared_from_this() ;
    if ( !importNodes(root, sc, sc->mRootNode) ) return false ;

    if ( options_ & Scene::IMPORT_ANIMATIONS ) {
        if ( !importAnimations(sc) ) return false ;
    }

    if ( !findSkeletonHierarchies() ) return false ;

    return true ;
}

}

void Node::load(const std::string &fname, int options, float scale) {
  //  const aiScene *sc = aiImportFile(fname.c_str(), aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs | aiProcess_TransformUVCoords);
    const aiScene *sc = aiImportFile(fname.c_str(),
    aiProcess_GenNormals
  | aiProcess_Triangulate
  | aiProcess_JoinIdenticalVertices
  | aiProcess_SortByPType
  | aiProcess_OptimizeMeshes
  | aiProcess_LimitBoneWeights
  |  aiProcess_FlipUVs | aiProcess_TransformUVCoords
                                     ) ;
    if ( !sc ) {
        throw SceneLoaderException(aiGetErrorString(), fname) ;
    }

    load(sc, fname, options, scale) ;

    aiReleaseImport(sc) ;
}

void Node::load(const aiScene *sc, const std::string &fname, int options, float scale) {

    impl::AssimpImporter importer(*this, options, scale) ;

    bool res = importer.import(sc, fname) ;

    if ( !res ) {
        aiReleaseImport(sc) ;
        throw SceneLoaderException("Error while parsing assimp scene", fname) ;
    }

}


}}


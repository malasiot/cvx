#include <cvx/viz/robot/robot_scene.hpp>
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/robot/urdf_robot.hpp>

#include <cvx/viz/robot/urdf_loader.hpp>

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {

RobotScenePtr RobotScene::loadURDF(const string &filename, const map<string, string> &packages, bool load_collision_geometry) {

    urdf::Loader loader(packages, load_collision_geometry) ;
    urdf::Robot rb = loader.parse(filename) ;
    return parseRobotURDF(rb) ;
}

RobotScenePtr RobotScene::fromURDF(const urdf::Robot &r) {
     return parseRobotURDF(r) ;
}

static NodePtr createLinkGeometry(const urdf::Geometry *urdf_geom, MaterialInstancePtr mat, Vector3f &scale) {

    if ( const urdf::MeshGeometry *mesh = dynamic_cast<const urdf::MeshGeometry *>(urdf_geom) ) {
        NodePtr geom(new Node) ;

        geom->setName(mesh->path_) ;
        geom->load(mesh->path_, 0) ;

        // replace all materials in loaded model with that provided in urdf
        if ( mat ) {
            geom->visit([&](Node &n) {
                for( auto &dr: n.drawables() ) {
                    dr->setMaterial(mat) ;
                }
            }) ;
        }

        scale = mesh->scale_ ;

        return geom ;
    } else if ( const urdf::BoxGeometry *box = dynamic_cast<const urdf::BoxGeometry *>(urdf_geom) ) {
        NodePtr geom_node(new Node) ;

        GeometryPtr geom(new BoxGeometry(box->he_)) ;

        DrawablePtr dr(new Drawable(geom, mat)) ;

        geom_node->addDrawable(dr) ;

        return geom_node ;
    } else if ( const urdf::CylinderGeometry *cylinder = dynamic_cast<const urdf::CylinderGeometry *>(urdf_geom) ) {
        NodePtr geom_node(new Node) ;

        GeometryPtr geom(new CylinderGeometry(cylinder->radius_, cylinder->height_)) ;

        DrawablePtr dr(new Drawable(geom, mat)) ;

        geom_node->addDrawable(dr) ;

        return geom_node ;

    } else if ( const urdf::SphereGeometry *sphere = dynamic_cast<const urdf::SphereGeometry *>(urdf_geom) ) {
        NodePtr geom_node(new Node) ;

        GeometryPtr geom(new SphereGeometry(sphere->radius_)) ;

        DrawablePtr dr(new Drawable(geom, mat)) ;

        geom_node->addDrawable(dr) ;

        return geom_node ;

    }

    return nullptr ;

}

RobotScenePtr RobotScene::parseRobotURDF(const urdf::Robot &rb)
{
    RobotScenePtr scene(new RobotScene) ;

    map<string, NodePtr> link_nodes ;
    map<string, string> parent_link_tree ;
    map<string, MaterialInstancePtr> materials ;
    NodePtr root_node ;

    // create materials

    for( const auto &mp: rb.materials_ ) {
        const urdf::Material &mat = *mp.second ;
        const std::string &name = mp.first ;

        if ( mat.texture_path_.empty() ) {
            Vector4f clr = mat.diffuse_color_ ;
            PhongMaterialInstance *material = new PhongMaterialInstance ;
            material->setShininess(0);
            material->setSpecular({0, 0, 0, 1}) ;
            material->setDiffuse(clr) ;

            materials.emplace(name, MaterialInstancePtr(material)) ;
        } else {
            Texture2D s(mat.texture_path_) ;

            DiffuseMapMaterialInstance *material = new DiffuseMapMaterialInstance(s) ;
            material->setShininess(0);
            material->setSpecular({0, 0, 0, 1}) ;
            material->setDiffuse(s) ;

            materials.emplace(name, MaterialInstancePtr(material)) ;
        }
    }

    // create links

    for( const auto &lp: rb.links_ ) {
        const urdf::Link &link = lp.second ;

        if ( link.visual_geom_ ) {

            urdf::Geometry *geom = link.visual_geom_.get() ;

            const std::string &matref = geom->material_ref_ ;

            MaterialInstancePtr mat ;
            auto mat_it = materials.find(matref) ;
            if ( mat_it != materials.end() )
                mat = mat_it->second ;

            Vector3f scale{1, 1, 1} ;
            NodePtr geom_node = createLinkGeometry(geom, mat, scale) ;

            Isometry3f local_inertial_frame = Isometry3f::Identity() ;
            if ( link.inertial_ )
                local_inertial_frame = link.inertial_->origin_ ;

            geom_node->matrix() = local_inertial_frame.inverse() * geom->origin_ ;

            NodePtr link_node(new Node) ;
            link_node->addChild(geom_node) ;

            link_node->setName(link.name_) ;

            scene->addChild(link_node) ;


            link_nodes.emplace(link.name_, geom_node) ;
        }


    }


    map<string, Isometry3f> trs ;
    rb.computeLinkTransforms(trs) ;
    scene->updateTransforms(trs) ;
    return scene ;

}

float RevoluteJoint::setPosition(float pos)
{
    Isometry3f tr ;
    tr.setIdentity() ;
    tr.rotate(AngleAxisf(pos, axis_)) ;
    node_->matrix() = tr ;

    pos = std::max(pos, lower_limit_) ;
    pos = std::min(pos, upper_limit_) ;

    for( uint i=0 ; i<dependent_.size() ; i++ ) {
        float vpos = multipliers_[i] * pos + offsets_[i] ;
        if ( auto j = std::dynamic_pointer_cast<RevoluteJoint>(dependent_[i]) ) {
            j->setPosition(vpos) ;
        }
    }

    return pos ;
}

}}

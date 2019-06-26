#include <cvx/viz/robot/robot_scene.hpp>
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/robot/urdf_robot.hpp>

#include "urdf_loader.hpp"

using namespace std ;
using namespace Eigen ;

namespace cvx { namespace viz {

RobotScenePtr RobotScene::loadURDF(const string &filename, const map<string, string> &packages, bool load_collision_geometry) {

    urdf::Loader loader(packages, load_collision_geometry) ;
    urdf::Robot rb = loader.parse(filename) ;
    return parseRobotURDF(rb) ;
}

static NodePtr createLinkGeometry(const urdf::Geometry *urdf_geom, MaterialInstancePtr mat, Vector3f &scale) {

    if ( const urdf::MeshGeometry *mesh = dynamic_cast<const urdf::MeshGeometry *>(urdf_geom) ) {
        NodePtr geom(new Node) ;

        geom->setName(mesh->path_) ;

        ScenePtr scene(new Scene) ;
        scene->load(mesh->path_, geom) ;

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
            material->setSpecular(Vector4f(0, 0, 0, 1)) ;
            material->setDiffuse(clr) ;

            materials.emplace(name, MaterialInstancePtr(material)) ;
        } else {
            Texture2D s(mat.texture_path_) ;

            DiffuseMapMaterialInstance *material = new DiffuseMapMaterialInstance(s) ;
            material->setShininess(0);
            material->setSpecular(Vector4f(0, 0, 0, 1)) ;
            material->setDiffuse(s) ;

            materials.emplace(name, MaterialInstancePtr(material)) ;
        }
    }

    // create links

    for( const auto &lp: rb.links_ ) {
        const urdf::Link &link = lp.second ;

        NodePtr link_node(new Node) ;

        link_node->setName(link.name_) ;

        if ( link.visual_geom_ ) {
            urdf::Geometry *geom = link.visual_geom_.get() ;

            const std::string &matref = geom->material_ref_ ;

            MaterialInstancePtr mat ;
            auto mat_it = materials.find(matref) ;
            if ( mat_it != materials.end() )
                mat = mat_it->second ;

            Vector3f scale{1, 1, 1} ;
            NodePtr geom_node = createLinkGeometry(geom, mat, scale) ;

            Isometry3f tr ;
            tr = link.visual_geom_->tr_ ;
        //    tr.linear() *= scale.asDiagonal() ;
            geom_node->matrix() = tr ;

            link_node->addChild(geom_node) ;
        }


        link_nodes.emplace(link.name_, link_node) ;
    }

    // create joints and link into hierarchy

    map<string, JointNodePtr> mimic_joints ;

    for( const auto &jp: rb.joints_ ) {

        const urdf::Joint &j = jp.second ;

        NodePtr joint(new Node) ;
        joint->setName(j.name_) ;
        joint->matrix() = j.origin_ ;

        string parent_link_name = j.parent_ ;
        string child_link_name = j.child_ ;

        NodePtr parent_link, child_link ;

        auto pl_it = link_nodes.find(parent_link_name) ;
        if ( pl_it == link_nodes.end() ) continue ;
        else parent_link = pl_it->second ;

        auto cl_it = link_nodes.find(child_link_name) ;
        if ( cl_it == link_nodes.end() ) continue ;
        else child_link = cl_it->second ;

        NodePtr ctrl_node(new Node) ;
        ctrl_node->setName(j.name_ + "_ctrl") ;

        joint->addChild(ctrl_node) ;

        parent_link->addChild(joint) ;

        ctrl_node->addChild(child_link) ;

        parent_link_tree[child_link_name] = parent_link_name ;

        JointNodePtr jnode ;

        if ( j.type_ == "revolute" ) {
            RevoluteJoint *rj = new RevoluteJoint ;
            rj->lower_limit_ = j.lower_ ;
            rj->upper_limit_ = j.upper_ ;
            rj->axis_ = j.axis_ ;
            rj->node_ = ctrl_node ;
            jnode.reset(rj) ;
        }

        if ( jnode ) {
            if ( j.mimic_joint_.empty() )
                scene->joints_.emplace(j.name_, jnode) ;
            else
                mimic_joints[j.name_] = jnode ;
        }
    }

    // setup mimic joints

    for( const auto &jp: rb.joints_ ) {
            const urdf::Joint &j = jp.second ;

            if ( j.mimic_joint_.empty() ) continue ;

            auto it = scene->joints_.find(j.mimic_joint_) ;
            if ( it == scene->joints_.end() ) continue ;

            auto jit = mimic_joints.find(j.name_) ;
            if ( jit != mimic_joints.end() ) {
                JointNodePtr dof = it->second ;
                dof->multipliers_.push_back(j.mimic_multiplier_) ;
                dof->offsets_.push_back(j.mimic_offset_) ;
                dof->dependent_.push_back(jit->second) ;
            }
    }

    // find root

    for( const auto &lp: link_nodes ) {
        auto it = parent_link_tree.find(lp.first) ;
        if ( it == parent_link_tree.end() ) {
            root_node = lp.second ;
            break ;
        }
    }


    scene->addChild(root_node) ;

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

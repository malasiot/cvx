#include <cvx/viz/robot/urdf_loader.hpp>
#include <cvx/viz/scene/drawable.hpp>
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/util/misc/strings.hpp>
#include <cvx/util/misc/path.hpp>
#include <set>

using namespace std ;
using namespace pugi ;

using namespace cvx::util ;
using namespace Eigen ;

namespace cvx { namespace viz {

namespace urdf {


Robot Loader::parse(const string &urdf_file) {
    Robot robot ;

    xml_document doc ;

    xml_parse_result result = doc.load_file(urdf_file.c_str()) ;

    if ( !result )
        throw LoadException(result.description()) ;

    xml_node root = doc.child("robot") ;

    if ( !root )
        throw LoadException("No <robot> element found") ;

    robot.name_ = root.attribute("name").as_string() ;

    parseRobot(root, robot, urdf_file) ;

    buildTree(robot) ;

    return robot ;
}

void Loader::parseRobot(const xml_node &node, Robot &rb, const string &path) {

    for( const xml_node &n: node.children("material") )
        parseMaterial(n, rb, path) ;

    for( const xml_node &n: node.children("link") )
        parseLink(n, rb, path) ;

    for( const xml_node &n: node.children("joint") )
        parseJoint(n, rb) ;
}

void Loader::parseLink(const xml_node &node, Robot &rb, const string &path) {

    Link link ;

    string name = node.attribute("name").as_string() ;

    if ( name.empty() )
        throw LoadException("Attribute \"name\" missing from <link>") ;

    link.name_ = name ;

    if ( xml_node visual_node = node.child("visual") ) {
        Isometry3f tr ;
        tr.setIdentity() ;

        if ( xml_node origin_node = visual_node.child("origin") )
            tr = parseOrigin(origin_node) ;

        string matid ;

        if ( xml_node material_node = visual_node.child("material") ) {
            matid = material_node.attribute("name").as_string() ;
            parseMaterial(material_node, rb, path) ;
        }

        Vector3f scale{1, 1, 1} ;

        Geometry *geom = nullptr ;

        if ( xml_node geom_node = visual_node.child("geometry") )
            geom = parseGeometry(geom_node, matid, scale, path) ;
        else
            throw LoadException("<geometry> element is missing from <visual>") ;

        tr.linear() *= scale.asDiagonal() ;
        geom->origin_ = tr ;

        link.visual_geom_.reset(geom) ;
    }

    if ( xml_node collision_node = node.child("collision") ) {
        Isometry3f tr ;
        tr.setIdentity() ;

        if ( xml_node origin_node = collision_node.child("origin") )
            tr = parseOrigin(origin_node) ;

        Vector3f scale{1, 1, 1} ;

        Geometry *geom ;

        if ( xml_node geom_node = collision_node.child("geometry") )
            geom = parseGeometry(geom_node, std::string(), scale, path) ;
        else
            throw LoadException("<geometry> element is missing from <collision>") ;

        tr.linear() *= scale.asDiagonal() ;
        geom->origin_ = tr ;

        link.collision_geom_.reset(geom) ;
    }

    if ( xml_node inertial_node = node.child("inertial") ) {

        Inertial inr ;
        inr.origin_.setIdentity() ;
        inr.mass_ = 0.0;
        inr.inertia_.setIdentity() ;

        if ( xml_node origin_node = inertial_node.child("origin") )
            inr.origin_ = parseOrigin(origin_node) ;

        if ( xml_node mass_node = inertial_node.child("mass") )
            inr.mass_ = mass_node.attribute("value").as_float() ;

        if ( xml_node inertia_node = inertial_node.child("inertia") )
            inr.inertia_ = parseInertia(inertia_node) ;

        link.inertial_.reset(new Inertial(std::move(inr)));
    }



    rb.links_.insert(std::make_pair(name, std::move(link))) ;
}

static Vector3f parse_vec3(const std::string &s) {
    istringstream strm(s) ;
    float x, y, z ;
    strm >> x >> y >> z ;
    return {x, y, z} ;
}

static Vector4f parse_vec4(const std::string &s) {
    istringstream strm(s) ;
    float x, y, z, w ;
    strm >> x >> y >> z >> w;
    return {x, y, z, w} ;
}


void Loader::parseJoint(const xml_node &node, Robot &rb) {
    string name = node.attribute("name").as_string() ;
    string type = node.attribute("type").as_string() ;

    if ( name.empty() )
        throw LoadException("<joint> is missing \"name\" attribute") ;

    Joint j ;

    j.type_ = type ;
    j.name_ = name ;

    if ( xml_node origin_node = node.child("origin") )
        j.origin_ = parseOrigin(origin_node) ;

    if ( xml_node parent_node = node.child("parent") ) {
        string link_name = parent_node.attribute("link").as_string() ;
        j.parent_ = link_name ;
    }

    if ( xml_node child_node = node.child("child") ) {
        string link_name = child_node.attribute("link").as_string() ;
        j.child_ = link_name ;
    }

    if ( xml_node axis_node = node.child("axis") ) {
        string axis_str = axis_node.attribute("xyz").as_string() ;
        j.axis_ = parse_vec3(axis_str) ;
    }

    if ( xml_node limits_node = node.child("limit") ) {
        j.lower_ = limits_node.attribute("lower").as_float(0) ;
        j.upper_ = limits_node.attribute("upper").as_float(0) ;
        j.effort_ = limits_node.attribute("effort").as_float(0) ;
        j.velocity_ = limits_node.attribute("velocity").as_float(0) ;
    }

    if ( xml_node mimic_node = node.child("mimic") ) {
        j.mimic_joint_ = mimic_node.attribute("joint").as_string() ;
        j.mimic_offset_ = mimic_node.attribute("offset").as_float(0.0) ;
        j.mimic_multiplier_ = mimic_node.attribute("multiplier").as_float(1.0) ;
    }

    if ( xml_node dynamics_node = node.child("dynamics") ) {
        j.friction_ = dynamics_node.attribute("friction").as_float() ;
        j.damping_ = dynamics_node.attribute("damping").as_float() ;
    }

    rb.joints_.emplace(name, j) ;

}

bool Loader::buildTree(Robot &rb) {
    map<string, string> parent_link_tree ;

    for( auto &jp: rb.joints_ ) {
        Joint &j = jp.second ;

        if ( j.child_.empty() || j.parent_.empty() ) return false ;

        Link *child_link = rb.getLink(j.child_) ;
        Link *parent_link = rb.getLink(j.parent_) ;

        if ( child_link == nullptr || parent_link == nullptr ) return false ;

        child_link->parent_link_ = parent_link ;
        child_link->parent_joint_ = &j ;
        parent_link->child_joints_.push_back(&j) ;
        parent_link->child_links_.push_back(child_link) ;
        parent_link_tree[child_link->name_] = j.parent_;
    }

    // find root

    rb.root_ = nullptr ;

    for ( const auto &lp: rb.links_ ) {
        const string &name = lp.first ;
        const Link &l = lp.second ;
        auto it = parent_link_tree.find(name) ;
        if ( it == parent_link_tree.end() ) { // no parent thus root
            if ( rb.root_ == nullptr ) {
                Link *l = rb.getLink(name) ;
                rb.root_ = l ;
            }
            else return false ;
        }
    }

    if ( rb.root_ == nullptr ) return false ;

    return true ;
}

Isometry3f Loader::parseOrigin(const xml_node &node) {

    string xyz = node.attribute("xyz").as_string() ;
    string rpy = node.attribute("rpy").as_string() ;

    Isometry3f tr ;
    tr.setIdentity() ;

    if ( !xyz.empty() ) {
        Vector3f t = parse_vec3(xyz) ;
        tr.translate(t) ;
    }

    if ( !rpy.empty() ) {
        Vector3f r = parse_vec3(rpy) ;
        Quaternionf q ;

        q = AngleAxisf(r.z(), Vector3f::UnitZ()) * AngleAxisf(r.y(), Vector3f::UnitY()) * AngleAxisf(r.x(), Vector3f::UnitX());

        tr.rotate(q) ;
    }

    return tr ;
}

Matrix3f Loader::parseInertia(const xml_node &node) {

    float ixx = node.attribute("ixx").as_float() ;
    float ixy = node.attribute("ixy").as_float() ;
    float ixz = node.attribute("ixz").as_float() ;
    float iyy = node.attribute("iyy").as_float() ;
    float iyz = node.attribute("iyz").as_float() ;
    float izz = node.attribute("izz").as_float() ;

    Matrix3f i ;
    i << ixx, ixy, ixz,
         ixy, iyy, iyz,
         ixz, iyz, izz ;

    return i ;
}

string Loader::resolveUri(const std::string &uri, const std::string &path) {

    string rpath ;

    if ( startsWith(uri, "package://") ) {
        size_t pos = uri.find_first_of('/', 10) ;
        if ( pos == string::npos ) return rpath ;
        string package_str = uri.substr(10, pos-10) ;
        string package_subpath = uri.substr(pos+1) ;

        auto it = package_map_.find(package_str) ;
        if ( it == package_map_.end() ) return rpath ;
        rpath = Path(it->second, package_subpath).toString() ;
    } else {
        Path rp = Path(path).parentPath() / uri ;
        if ( rp.exists() ) rpath = rp.toString() ;
    }

    return rpath;

}

Geometry *Loader::parseGeometry(const xml_node &node, const std::string &mat, Vector3f &sc, const string &path) {

    if ( xml_node mesh_node = node.child("mesh") ) {

        MeshGeometry *geom = new MeshGeometry() ;

        string uri = mesh_node.attribute("filename").as_string() ;

        string rpath = resolveUri(uri, path) ;

        if ( rpath.empty() ) return nullptr ;

        geom->path_ = rpath ;

        string scale = mesh_node.attribute("scale").as_string() ;

        if ( !scale.empty() ) {
            sc = parse_vec3(scale) ;
        }

        geom->scale_ = sc ;
        geom->material_ref_ = mat ;

        return geom ;
    } else if ( xml_node box_node = node.child("box") ) {
        string sz = box_node.attribute("size").as_string() ;
        Vector3f hs = parse_vec3(sz)/2 ;


        BoxGeometry *geom = new BoxGeometry(hs) ;
        geom->material_ref_ = mat ;

        return geom ;
    } else if ( xml_node cylinder_node = node.child("cylinder") ) {

        float radius = cylinder_node.attribute("radius").as_float(0) ;
        float length = cylinder_node.attribute("length").as_float(0) ;

        CylinderGeometry *geom = new CylinderGeometry(radius, length) ;

        geom->material_ref_ = mat ;

        return geom ;
    } else if ( xml_node cylinder_node = node.child("sphere") ) {

        float radius = cylinder_node.attribute("radius").as_float(0) ;

        SphereGeometry *geom = new SphereGeometry(radius) ;

        geom->material_ref_ = mat ;

        return geom ;
    }

    return nullptr ;
}

void Loader::parseMaterial(const xml_node &node, Robot &rb, const string &path)
{
    string name = node.attribute("name").as_string() ;
    if ( name.empty() ) return ;

    if ( xml_node clr_node = node.child("color") ) {
        string rgba = clr_node.attribute("rgba").as_string() ;
        if ( !rgba.empty() ) {
            Vector4f clr = parse_vec4(rgba) ;

            Material *mat = new Material ;
            mat->diffuse_color_ = clr ;
            rb.materials_.emplace(name, std::shared_ptr<Material>(mat)) ;
            return ;
        }
    }

    if ( xml_node texture_node = node.child("texture") ) {
        string uri = texture_node.attribute("filename").as_string() ;

        string rpath = resolveUri(uri, path) ;
        if ( !rpath.empty() ) {

            Material *mat = new Material ;
            mat->texture_path_ = rpath ;

            rb.materials_.emplace(name, std::shared_ptr<Material>(mat)) ;
            return ;
        }
    }


}

}
}}


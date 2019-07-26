#include <QApplication>
#include <QMainWindow>

#include <cvx/util/misc/filesystem.hpp>
#include <cvx/viz/gui/simple_qt_viewer.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/mesh.hpp>
#include <cvx/viz/scene/material.hpp>

#include "mhx2_importer.hpp"
#include "mh_scene.hpp"
#include "skinned_mesh.hpp"

using namespace cvx::util ;
using namespace cvx::viz ;
using namespace std ;
using namespace Eigen ;


struct Part {
    vector<string> bones_ ;
} ;

void read_parts(const string &fname, map<string, Part> &parts) {
    ifstream strm(fname) ;

    try {
        JSONReader reader(strm) ;

        reader.beginObject() ;

        while ( reader.hasNext() ) {
            string partName = reader.nextName() ;
            Part part ;

            reader.beginArray() ;
            while ( reader.hasNext() ) {
               string boneName = reader.nextString() ;
               part.bones_.push_back(boneName) ;
            }
            reader.endArray() ;

            parts.emplace(partName, std::move(part)) ;
        }

        reader.endObject() ;

    }
    catch ( JSONParseException &e ) {
        cerr << "Error parsing parts file: " << fname << " (" ;
        cerr << e.what() << ")" ;
    }

}

void make_part(const SkinnedMesh &orig, const string &part_name, const Part &part, const Matrix4f &tr,  uint pspts) {

    // select vertex indexes affected by the bones in the part

    set<uint> vtx_set, bone_set ;

    for( const string &b: part.bones_ ) {
         int tidx = orig.skeleton_.getBoneIndex(b) ;
         bone_set.insert(tidx) ;
    }

    for ( size_t i = 0 ; i<orig.positions_.size() ; i++ ) {

        const SkinnedMesh::VertexBoneData &bdata = orig.bone_weights_[i] ;

        for( size_t k = 0 ; k<bdata.id_.size() ; k++ ) {
            auto it = bone_set.find(bdata.id_[k]) ;
            if ( it == bone_set.end() ) continue ;

            // select those that are affected at least 25%
            if ( bdata.weight_[k] > 0.15 ) vtx_set.insert(i) ;

        }
    }

    // select triangles that contain all selected vertices

    vector<int> part_indices ;
    set<uint> vidxs ;

    for( size_t i = 0 ; i<orig.indices_.size() ; ) {
        uint v1 = orig.indices_[i++] ;
        uint v2 = orig.indices_[i++] ;
        uint v3 = orig.indices_[i++] ;

        if ( vtx_set.find(v1) == vtx_set.end()) continue ;
        if ( vtx_set.find(v2) == vtx_set.end()) continue ;
        if ( vtx_set.find(v3) == vtx_set.end()) continue ;

        part_indices.push_back(v1) ;
        part_indices.push_back(v2) ;
        part_indices.push_back(v3) ;
        vidxs.insert(v1) ;
        vidxs.insert(v2) ;
        vidxs.insert(v3) ;
    }

    Matrix4f ti = tr ;
    Matrix3f tn = ti.block<3, 3>(0, 0).transpose().inverse() ;
    vector<Vector3f> pts, normals ;
    map<uint, uint> index_map ;

    // make mesh consisting only of selected faces and transformed to bone space

    for( uint idx: vidxs )  {
        const Vector3f &pg = orig.positions_[idx] ;
        const Vector3f &ng = orig.normals_[idx] ;

        Vector3f pb = (ti * pg.homogeneous()).block<3,1>(0, 0) ;
        Vector3f nb = tn * ng ;
    /*
        bmin = std::min(bmin, pb.y()) ;
        bmax = std::max(bmax, pb.y()) ;

        max_x = std::max(max_x, fabs(pb.x())) ;
        max_y = std::max(max_y, fabs(pb.y())) ;
        max_z = std::max(max_z, fabs(pb.z())) ;
    */
        index_map[idx] = pts.size() ;
        pts.push_back(pb) ;
        normals.push_back(nb) ;
    }

    // make triangles of the submesh

       vector<uint> indices ;

       for (uint i=0 ; i<part_indices.size() ;  )
       {
           uint i0 = part_indices[i++] ;
           uint i1 = part_indices[i++] ;
           uint i2 = part_indices[i++] ;

           indices.push_back(index_map[i0]) ;
           indices.push_back(index_map[i1]) ;
           indices.push_back(index_map[i2]) ;
       }

       ofstream strm( "/home/malasiot/Downloads/" + part_name + ".obj") ;
       for( size_t i=0 ; i<pts.size() ; i++ ) {
           strm << "v " << pts[i].adjoint() << endl ;
       }

       for (size_t i=0 ; i<indices.size() ;  ) {
           uint v1 = indices[i++] + 1;
           uint v2 = indices[i++] + 1;
           uint v3 = indices[i++] + 1;
           strm << "f " << v1 << ' ' << v2 << ' ' << v3 << endl ;
       }

}

void make_parts(const SkinnedMesh &orig, const string &parts_file, uint pspts)
{
    map<string, Part> parts ;
    read_parts(parts_file, parts) ;

    for( const auto &kv: parts )  {
        string part_name = kv.first ;
        const Part &p = kv.second ;

        uint bidx = orig.skeleton_.getBoneIndex(p.bones_[0]) ;
        Matrix4f mat = orig.skeleton_.bones_[bidx].offset_ ;

        make_part(orig, part_name, p, mat, pspts ) ;
    }
}

ScenePtr g_scene(new Scene) ;
int main(int argc, char *argv[]) {

    string data_dir = get_data_folder("test_mh", argc, argv) ;

    Affine3f m ;
    m.setIdentity() ;
    m.rotate(AngleAxisf(0.25*M_PI, Vector3f::UnitX()));




    Mhx2Importer importer ;
    importer.load(data_dir + "/human.mhx2", "") ;

    MakeHumanSkinnedMesh sk(importer.getModel()) ;

    make_parts(sk, "/home/malasiot/Downloads/parts.json", 1000) ;

    Pose p({{"upperarm01.L", m.matrix()}}) ;

    PointList3f coords, clrs ;
    sk.getTransformedVertices(p, coords) ;

    for( auto c: coords ) clrs.push_back(Vector3f(1, 0, 0)) ;


    g_scene->addSimpleShapeNode(make_shared<MeshGeometry>(Mesh::makePointCloud(coords, clrs)), MaterialInstancePtr(new PerVertexColorMaterialInstance(1))) ;

    std::shared_ptr<MHNode> mh_node(new MHNode(importer.getModel())) ;
    mh_node->setVisible(false) ;
    g_scene->addChild(mh_node) ;

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    g_scene->addLight(LightPtr(dl)) ;

    NodePtr node = g_scene->findNodeByName("upperarm01.L") ;

    node->matrix() = node->matrix() * m ;

    QApplication app(argc, argv);

    SimpleQtViewer *viewer = new SimpleQtViewer();


    viewer->setScene(g_scene) ;
    viewer->initCamera({0, 0, 0}, 1.0f) ;

    QMainWindow window ;
    window.setCentralWidget(viewer) ;
    window.resize(512, 512) ;
    window.show() ;

    viewer->setAnimationCallback([&](float ) {
//        timeStep() ;
    }) ;

  //  viewer->startAnimations() ;

    return app.exec();


}

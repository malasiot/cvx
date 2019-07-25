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

ScenePtr g_scene(new Scene) ;
int main(int argc, char *argv[]) {

    string data_dir = get_data_folder("test_mh", argc, argv) ;

    Affine3f m ;
    m.setIdentity() ;
    m.rotate(AngleAxisf(0.25*M_PI, Vector3f::UnitX()));




    Mhx2Importer importer ;
    importer.load(data_dir + "/human.mhx2", "") ;

    MakeHumanSkinnedMesh sk(importer.getModel()) ;

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

#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/renderer/gl/shader.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>
#include <cvx/viz/scene/mesh.hpp>

#include <cvx/viz/gui/glfw_viewer.hpp>
#include <cvx/viz/gui/trackball.hpp>

#include <GLFW/glfw3.h>
#include <iostream>

using namespace cvx::viz ;

using namespace std ;
using namespace Eigen ;

class glfwGUI: public GLFWViewer {
public:

    glfwGUI(ScenePtr scene): GLFWViewer(scene) {
    }


    void onMouseMoved(double xpos, double ypos) override {
        ostringstream s ;
        s << xpos << ',' << ypos ;
        text_ = s.str() ;

        GLFWViewer::onMouseMoved(xpos, ypos) ;
    }


    void onUpdate(double delta) override {
        GLFWViewer::onUpdate(delta) ;
        rdr_.text(text_, 10, 10, Font("arial", 24), {1, 1, 0});
    }


    string text_ ;
};

int main(int argc, char *argv[]) {

    PhongMaterialInstance *mi  = new PhongMaterialInstance() ;
    mi->setDiffuse({1, 0, 0, 1}) ;
    MaterialInstancePtr custom_material(mi) ;

    ScenePtr scene(new Scene) ;
     //scene->load("/home/malasiot/Downloads/greek_column.obj") ;
  //   scene->load("/home/malasiot/Downloads/cube.obj") ;


    MeshPtr sphere = Mesh::flatten(Mesh::createCapsule(0.1, 0.5, 9, 2, 16)) ;

    //MeshPtr sphere = Mesh::createSolidSphere(0.1, 16, 16) ;

    scene->addSimpleShapeNode(make_shared<MeshGeometry>(sphere), custom_material) ;
        // add a light source

    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;



    glfwGUI gui(scene) ;

    gui.run(640, 480) ;

}

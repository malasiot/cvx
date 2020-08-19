#include <cvx/viz/renderer/renderer.hpp>

#include <cvx/viz/robot/robot_scene.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>

#include <cvx/viz/gui/glfw_viewer.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <thread>
using namespace cvx::viz ;

using namespace std ;
using namespace Eigen ;

class glfwGUI: public GLFWViewer {
public:

    glfwGUI(const RobotScenePtr scene): GLFWViewer(scene) {
        joint_ = std::dynamic_pointer_cast<RevoluteJoint>(scene->getJoint("r2_joint_u")) ;
        joint_pos_ = 0.0 ;
    }


    void onKeyPressed(size_t key, uint flags) override {
        GLFWViewer::onKeyPressed(key, flags) ;

        if (key == GLFW_KEY_Q ) {
            joint_pos_ -= 0.1 ;
            joint_pos_ = joint_->setPosition(joint_pos_) ;

        } else if ( key ==  GLFW_KEY_W ) {
            joint_pos_ += 0.1 ;
            joint_pos_ = joint_->setPosition(joint_pos_) ;
        }
    }

    std::shared_ptr<RevoluteJoint> joint_ ;
    float joint_pos_ ;

};

class HierarchyPrinter: public NodeVisitor {
public:
    void visit(Node &node) override {
        for( uint i=0 ; i<count_ ; i++ )
            cout << '\t' ;
        string name = node.name() ;
        if ( name.empty() )
            cout << "<empty>" << endl ;
        else
            cout << node.name() << endl ;

        for( uint i=0 ; i<count_ ; i++ )  cout << '\t' ;
       cout << node.globalTransform().translation().adjoint() << endl ;
         // cout << node.matrix().translation().adjoint() << endl ;
        for( uint i=0 ; i<count_ ; i++ )  cout << '\t' ;
        cout << Quaternionf(node.globalTransform().rotation()).coeffs().adjoint() << endl ;
     //      cout << Quaternionf(node.matrix().rotation()).coeffs().adjoint() << endl ;

        count_++ ;
        visitChildren(node) ;
        count_-- ;
    }

    size_t count_ = 0 ;
};

int main(int argc, char *argv[]) {

//   string package_path = "/home/malasiot/source/radioroso_ws/src/radioroso_certh/cvx_grasp_planner/" ;
//    RobotScenePtr scene = RobotScene::loadURDF(package_path + "urdf/radioroso-gripper.urdf", { { "cvx_grasp_planner", package_path } }) ;

    string package_path = "/home/malasiot/source/radioroso_ws/src/clopema_testbed/clopema_description/" ;
    RobotScenePtr scene = RobotScene::loadURDF(package_path + "robots/clopema.urdf", { { "clopema_description", package_path } }, false) ;

    vector<string> hidden_nodes = { "ctu_f_roof_1", "ctu_f_roof_2", "ctu_f_roof_3", "ctu_f_wall_1",
                                    "ctu_f_wall_2", "ctu_f_wall_3", "ctu_f_wall_4" };

    for( const auto &s: hidden_nodes )
        scene->findNodeByName(s)->setVisible(false) ;

    HierarchyPrinter pr ;
    pr.visit(*scene);
    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;

    glfwGUI gui(scene) ;

    gui.run(500, 500) ;

}

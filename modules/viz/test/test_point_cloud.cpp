#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/mesh.hpp>
#include <cvx/viz/scene/material.hpp>

#include <cvx/viz/gui/glfw_viewer.hpp>
#include <cvx/viz/gui/trackball.hpp>

#include <cvx/util/imgproc/rgbd.hpp>
#include <cvx/util/camera/camera.hpp>

#include <iostream>
#include <opencv2/opencv.hpp>

#include <GLFW/glfw3.h>

using namespace cvx::viz ;
using namespace cvx::util ;


using namespace std ;
using namespace Eigen ;


int main(int argc, char *argv[]) {


    ScenePtr scene(new Scene) ;
    // scene->load("/home/malasiot/Downloads/greek_column.obj") ;

    cv::Mat depth = cv::imread("/home/malasiot/Downloads/grab_rgbd/soap/depth_00001.png", -1) ;
    cv::Mat clr = cv::imread("/home/malasiot/Downloads/grab_rgbd/soap/rgb_00001.png") ;

    PinholeCamera cam(525, 525, 640/2.0, 480/2, cv::Size(640, 480)) ;

    PointList3f coords, clrs ;
    depthToPointCloud(depth, cam, coords) ;

    uint w = clr.cols, h = clr.rows ;

    for(uint i=0 ; i<h ; i++ )
        for(uint j=0 ; j<w ; j++ ) {
            if ( depth.at<ushort>(i, j) != 0 ) {
                const cv::Vec3b &c = clr.at<cv::Vec3b>(i, j) ;
                clrs.emplace_back(c[2]/255.0, c[1]/255.0, c[0]/255.0) ;
            }
        }


    scene->addSimpleShapeNode(make_shared<MeshGeometry>(Mesh::makePointCloud(coords, clrs)), MaterialInstancePtr(new PerVertexColorMaterialInstance(1))) ;

  /*
    DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
    dl->diffuse_color_ = Vector3f(1, 1, 1) ;
    scene->addLight(LightPtr(dl)) ;
*/
   GLFWViewer gui(scene) ;

    gui.run(640, 480) ;

}

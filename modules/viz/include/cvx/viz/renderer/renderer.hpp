#ifndef CVX_VIZ_RENDERER_HPP
#define CVX_VIZ_RENDERER_HPP

#include <memory>

#include <cvx/viz/scene/scene.hpp>
#include <cvx/viz/gui/offscreen.hpp>
#include <cvx/viz/renderer/font.hpp>
#include <cvx/viz/renderer/text.hpp>
#include <opencv2/opencv.hpp>

namespace cvx { namespace viz {

namespace detail {
    class RendererImpl ;
}

class Renderer {
public:

    enum { RENDER_SHADOWS = 1 };

    Renderer(int flags = 0) ;
    ~Renderer() ;

    void setCamera(const CameraPtr &cam) ;

    void clearZBuffer();

    cv::Mat getColor(bool alpha = true);
    cv::Mat getColor(cv::Mat &bg, float alpha);
    cv::Mat getDepth();

    // render a scene hierarchy

    void render(const ScenePtr &scene) ;

    // Draws text on top of the scene using given font and color
    void text(const std::string &text, float x, float y, const Font &f, const Eigen::Vector3f &clr) ;
    void text(const std::string &text, const Eigen::Vector3f &pos, const Font &f, const Eigen::Vector3f &clr) ;

    // It returns a text object that may be cached and drawn several times by calling render function.
    // It uses OpenGL so it should be called after initializing GL context
    Text textObject(const std::string &text, const Font &f) ;

    // Draws text object on top of the scene using given font and color
    void text(const Text &text, float x, float y, const Font &f, const Eigen::Vector3f &clr) ;
    void text(const Text &text, const Eigen::Vector3f &pos, const Font &f, const Eigen::Vector3f &clr) ;

    // draw a line with given color and width. suitable for drawing a few lines for debug purpose
    void line(const Eigen::Vector3f &from, const Eigen::Vector3f &to, const Eigen::Vector4f &clr, float lineWidth = 1.);

    // draw a 3D elliptic arc with given center and normal
    void arc(const Eigen::Vector3f &center, const Eigen::Vector3f &normal, const Eigen::Vector3f &xaxis,
             float radiusA, float radiusB,
             float minAngle, float maxAngle,
             const Eigen::Vector4f &color, bool drawSect, float lineWidth = 1.f, float stepDegrees = 10.f);

    void circle(const Eigen::Vector3f &center, const Eigen::Vector3f &normal, float radius, const Eigen::Vector4f &color, float lineWidth = 1.0) ;

    void setDefaultFBO(uint fbo) ;
private:

    std::unique_ptr<detail::RendererImpl> impl_ ;
} ;

class OffscreenRenderer: public Renderer {
public:
    OffscreenRenderer(uint width, uint height): Renderer(), ow_(width, height) {}

protected:
    OffscreenRenderingWindow ow_ ;
} ;

}}

#endif

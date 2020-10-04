#ifndef VSIM_RENDERER_IMPL_HPP
#define VSIM_RENDERER_IMPL_HPP

#include <memory>

#include <cvx/viz/scene/scene.hpp>
#include <cvx/viz/renderer/ogl_shaders.hpp>
#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/material.hpp>

#include "GL/gl3w.h"

#include <Eigen/Geometry>
#include <opencv2/opencv.hpp>

#include "text_item.hpp"
#include "mesh_data.hpp"
#include "shadow_map.hpp"

namespace cvx { namespace viz { namespace detail {

class RendererImpl {
public:


    RendererImpl(int flags);
    ~RendererImpl() ;

    void setCamera(const CameraPtr &cam) ;

    void renderScene(const ScenePtr &scene) ;

    void render(const ScenePtr &scene, const NodePtr &node, const Eigen::Matrix4f &mat) ;
    void render(const ScenePtr &scene, const DrawablePtr &geom, const Eigen::Matrix4f &mat) ;

    void renderShadowMap(const ScenePtr &scene) ;
    void renderShadowMap(const ScenePtr &scene, const NodePtr &node, const Eigen::Matrix4f &mat) ;
    void renderShadowMap(const ScenePtr &scene, const DrawablePtr &geom, const Eigen::Matrix4f &mat) ;

    void clearZBuffer() ;

    void drawMeshData(MeshData &data, GeometryPtr geom);

    void setLights(const ScenePtr &scene, const MaterialInstancePtr &mat) ;
    void setLights(const NodePtr &node, const Eigen::Affine3f &parent_tf, const MaterialInstancePtr &mat) ;

    void setPose(const MeshPtr &mesh, const MaterialInstancePtr &material);

    void renderText(const std::string &text, float x, float y, const Font &face, const Eigen::Vector3f &clr) ;
    void renderText(const std::string &text, const Eigen::Vector3f &pos, const Font &face, const Eigen::Vector3f &clr) ;

    void renderTextObject(const Text  &text, float x, float y, const Font &face, const Eigen::Vector3f &clr) ;
    void renderTextObject(const Text &text, const Eigen::Vector3f &pos, const Font &face, const Eigen::Vector3f &clr) ;

    void drawLine(const Eigen::Vector3f &from, const Eigen::Vector3f &to, const Eigen::Vector4f &clr, float lineWidth);

    cv::Mat getColor(bool alpha);
    cv::Mat getColor(cv::Mat &bg, float alpha);
    cv::Mat getDepth();

    void setupShadows(const Eigen::Vector3f &ldir);

    void setDefaultFBO(GLuint fbo) {
        default_fbo_ = fbo ;
    }

    void enableShadows(bool shadows = true) {
        has_shadows_ = shadows ;
    }

private:

    Eigen::Matrix4f perspective_, proj_, ls_mat_ ;
    GLuint query_ ;
 //   Eigen::Vector4f bg_clr_= { 0, 0, 0, 1 } ;
    float znear_, zfar_ ;
    MaterialInstancePtr default_material_ ;
    CameraPtr cam_ ;

    uint light_index_ = 0 ;

    static detail::GlyphCacheMap g_glyphs ;

    OpenGLShaderProgram::Ptr line_shader_, shadow_map_shader_, shadow_debug_shader_ ;
    GLuint line_vao_, line_vbo_, line_idx_vbo_ ;
    GLint line_width_range_[2] ;
    int flags_ ;

    ShadowMap shadow_map_ ;
    GLuint default_fbo_ = 0 ;
    const float shadow_bias_ = 0.005f ;
    bool has_shadow_light_ = false ;
    bool has_shadows_ = true ;


} ;


}}}

#endif

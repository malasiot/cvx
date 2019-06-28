#ifndef CVX_VIZ_MATERIAL_HPP
#define CVX_VIZ_MATERIAL_HPP

#include <Eigen/Geometry>
#include <cvx/viz/renderer/ogl_shaders.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/texture.hpp>
#include <opencv2/opencv.hpp>

namespace cvx { namespace viz {

class Material ;
using MaterialPtr = std::shared_ptr<Material> ;


class MaterialInstance ;
using MaterialInstancePtr = std::shared_ptr<MaterialInstance> ;

// A material encapsulates the technique used render a mesh on screen

class Material {
public:
    Material(int flags): flags_(flags) {}

    // Should be overriden to return the shader program used to render the material.
    // Since this should be called after context creation it should create the required shaders on demand and cache it
    // either in the stock_shaders library or in some internal static variable

    virtual OpenGLShaderProgram::Ptr prog() = 0 ;

    static MaterialInstancePtr makeLambertian(const Eigen::Vector4f &clr) ;
    static MaterialInstancePtr makeConstant(const Eigen::Vector4f &clr) ;

protected:

    int flags_ ;
} ;


enum { USE_SKINNING = 1 } ;

class MaterialInstance {

public:

    virtual void instantiate() = 0;
    virtual void use() ;
    virtual void applyParameters() {}
    virtual void applyTransform(const Eigen::Matrix4f &cam, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model) {}
    virtual void applyLight(uint idx, const LightPtr &light, const Eigen::Affine3f &tf) {}
    virtual void applyBoneTransform(uint idx, const Eigen::Matrix4f &tf) ;

    void setFlags(int flags) {
        flags_ = flags ;
    }

protected:

    void applyDefaultPerspective(const Eigen::Matrix4f &cam, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model) ;
    void applyDefaultLight(uint idx, const LightPtr &light, const Eigen::Affine3f &tf) ;

    std::shared_ptr<Material> material_ ;
    int flags_ ;
};


class ConstantMaterialInstance: public MaterialInstance {
public:

    ConstantMaterialInstance(const Eigen::Vector4f &clr) ;

    void setColor(const Eigen::Vector4f &c) { clr_ = c ; }

protected:

    void instantiate() override ;

    void applyParameters() override ;

    void applyTransform(const Eigen::Matrix4f &cam, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model) override {
        applyDefaultPerspective(cam, view, model) ;
    }

    void applyLight(uint idx, const LightPtr &light, const Eigen::Affine3f &tf) override {
        applyDefaultLight(idx, light, tf) ;
    }

private:
    Eigen::Vector4f clr_ = { 1, 1, 1, 1} ;

};

class PhongMaterialInstance: public MaterialInstance {
public:
    PhongMaterialInstance() = default ;

    void setAmbient(const Eigen::Vector4f &a) { ambient_ = a ; }
    void setDiffuse(const Eigen::Vector4f &d) { diffuse_ = d ; }
    void setSpecular(const Eigen::Vector4f &s) { specular_ = s; }
    void setShininess(float s) { shininess_ = s ; }

protected:
    void instantiate() override ;

    void applyParameters() override ;

    void applyTransform(const Eigen::Matrix4f &cam, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model) override {
        applyDefaultPerspective(cam, view, model) ;
    }

    void applyLight(uint idx, const LightPtr &light, const Eigen::Affine3f &tf) override {
        applyDefaultLight(idx, light, tf) ;
    }

protected:

    Eigen::Vector4f ambient_ = { 0, 0, 0, 1},
    diffuse_ = { 0.5, 0.5, 0.5, 1.0 },
    specular_ = { 0, 0, 0, 1 };
    float shininess_  = 1.0 ;
};

class DiffuseMapMaterialInstance: public MaterialInstance {
public:
    DiffuseMapMaterialInstance(const Texture2D &tex) ;

    void setAmbient(const Eigen::Vector4f &a) { ambient_ = a ; }
    void setDiffuse(const Texture2D &t) { diffuse_map_ = t ; }
    void setSpecular(const Eigen::Vector4f &s) { specular_ = s; }
    void setShininess(float s) { shininess_ = s ; }

    void setTexture(const Texture2D &tex) { diffuse_map_ = tex ; }

protected:
    void instantiate() override ;

    void applyTransform(const Eigen::Matrix4f &cam, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model) override {
        applyDefaultPerspective(cam, view, model) ;
    }

    void applyParameters() override ;

    void applyLight(uint idx, const LightPtr &light, const Eigen::Affine3f &tf) override {
        applyDefaultLight(idx, light, tf) ;
    }

private:
    Eigen::Vector4f ambient_ = { 0, 0, 0, 1},
    specular_ = { 0, 0, 0, 1 };
    float shininess_  = 1.0 ;
    Texture2D diffuse_map_ ;
};

// This uses the colors specified per vertex as the ambient component and the specified diffuse color and normals for shading

class PerVertexColorMaterialInstance: public MaterialInstance {
public:

    PerVertexColorMaterialInstance(float opac) ;

    void setOpacity(float o) { opacity_ = o ; }

protected:
    void applyParameters() override ;

    void applyTransform(const Eigen::Matrix4f &cam, const Eigen::Matrix4f &view, const Eigen::Matrix4f &model) override {
        applyDefaultPerspective(cam, view, model) ;
    }

    void applyLight(uint idx, const LightPtr &light, const Eigen::Affine3f &tf) override {
        applyDefaultLight(idx, light, tf) ;
    }

    void instantiate() override ;
private:
      float opacity_ = 1.0 ;
};

} // namespace viz
} // namespave cvx
#endif

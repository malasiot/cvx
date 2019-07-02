#pragma once

#include "GL/gl3w.h"

namespace cvx { namespace viz { namespace detail {

class ShadowMap {
public:
    ShadowMap() = default ;
    ~ShadowMap();

    bool init(unsigned int width, unsigned int height);

    void bind();
    void unbind();

    void bindTexture(GLenum TextureUnit);

private:
    GLuint fbo_ = 0, texture_id_ = 0;
};

}}}

#pragma once

#include "GL/gl3w.h"

namespace cvx { namespace viz { namespace detail {

class ShadowMap {
public:
    ShadowMap() = default ;
    ~ShadowMap();

    bool init(unsigned int width, unsigned int height);

    void bind();
    void unbind(GLuint default_fbo);

    void bindTexture(GLenum TextureUnit);

    bool ready() const { return fbo_ != 0 ; }

private:
    GLuint fbo_ = 0, texture_id_ = 0;
};

}}}

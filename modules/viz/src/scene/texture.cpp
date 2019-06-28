#include <cvx/viz/scene/texture.hpp>

#include <cvx/util/misc/strings.hpp>
#include <GL/gl3w.h>

using namespace std ;
using namespace cvx::util ;

namespace cvx { namespace viz {

void Texture2D::read()
{
    if ( im_.data ) return ;

    cv::Mat image ;
    string ipath ;

    if ( startsWith(image_url_, "file://" ) ) {
        ipath = image_url_.substr(7) ;
        im_ = cv::imread(ipath) ;
        cv::flip(im_, im_, 0) ;
    }
}

void Texture2D::upload()
{
    if ( !im_.data ) return ;

    if ( texture_id_ == 0 ) {
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &texture_id_);
        glBindTexture(GL_TEXTURE_2D, texture_id_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Set texture clamping method
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
        glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
        glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
        glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );

        glTexImage2D(GL_TEXTURE_2D,     // Type of texture
                     0,                 // Pyramid level (for mip-mapping) - 0 is the top level
                     GL_RGB,            // Internal colour format to convert to
                     im_.cols,
                     im_.rows,
                     0,                 // Border width in pixels (can either be 1 or 0)
                     GL_BGR, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                     GL_UNSIGNED_BYTE,  // Image data type
                     im_.ptr());        // The actual image data itself

        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        glActiveTexture(GL_TEXTURE0) ;
        glBindTexture(GL_TEXTURE_2D, texture_id_) ;
    }

}





}}

#ifndef CVX_GFX_IMAGE_HPP
#define CVX_GFX_IMAGE_HPP

#include <memory>

namespace cvx { namespace gfx {

enum class ImageFormat { ARGB32, RGB24, A8 } ;

class Image {

public:

    Image() = default ;
    Image(unsigned int width, unsigned int height, ImageFormat fmt = ImageFormat::ARGB32) ;

    unsigned width() const { return width_ ; }
    unsigned height() const { return height_ ; }
    char *pixels() const { return pixels_.get(); }
    unsigned stride() const { return stride_; }

    ImageFormat format() const { return format_ ; }

    bool saveToPNG(const std::string &fileName) ;
    bool saveToPNGBuffer(std::string &data);

    static Image loadPNG(const std::string &fileName) ;
    static Image loadPNGBuffer(const std::string &data) ;

    ~Image() = default;

private:

    template<typename W>
    bool png_write(W &writer) ;

    template<typename R>
    static Image png_read(R &reader) ;

    std::shared_ptr<char> pixels_ ;
    unsigned width_ = 0, height_ = 0 , stride_ = 0 ;
    ImageFormat format_ = ImageFormat::ARGB32;
};

}}


#endif

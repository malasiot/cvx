#ifndef CVX_VIZ_KEYFRAME_HPP
#define CVX_VIZ_KEYFRAME_HPP

namespace cvx { namespace viz {

// a key frame is a value at a specific time. The time is in normalized coordinates [0, 1]

template<class T>
class KeyFrame {
public:
    KeyFrame(float t, T v): t_(t), value_(v) {}

    float getTimeStamp() const { return t_ ; }
    const T &getValue() const { return value_ ; }

    float t_ ;
    T value_ ;
};

} // namespace viz
} // namespace cvx

#endif

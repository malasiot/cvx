#ifndef CVX_VIZ_TIMELINE_HPP
#define CVX_VIZ_TIMELINE_HPP

#include <cvx/viz/animation/keyframe.hpp>
#include <vector>

namespace cvx { namespace viz {

// TimeLine class representing a sequence of keyframes over the animation interval

template<class T>
class TimeLine {
public:

    TimeLine() { }
    TimeLine(std::initializer_list<KeyFrame<T>> init): key_frames_(init) {
        assert(key_frames_.size() > 1) ;
    }

    void addKeyFrame(const KeyFrame<T> &kf) { key_frames_.emplace_back(kf) ; }
    void addKeyFrame(float t, T v) { key_frames_.emplace_back(t, v) ; }

    int numKeyFrames() const { return key_frames_.size() ; }

    const KeyFrame<T> &getKeyFrame(int idx) const { return key_frames_.at(idx) ; }

private:

    std::vector<KeyFrame<T>> key_frames_ ;
};


} // namespace viz
} // namespace cvx

#endif

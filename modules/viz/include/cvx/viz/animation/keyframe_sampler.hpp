#ifndef CVX_VIZ_KEYFRAME_SAMPLER_HPP
#define CVX_VIZ_KEYFRAME_SAMLPER_HPP

#include <cvx/viz/animation/timeline.hpp>
#include <vector>
#include <Eigen/Geometry>

namespace cvx { namespace viz {

// concept of interpolation function between key frames

template <class T> class KeyFrameInterpolator {
    public:
    static T interpolate(float t, const T &kf1, const T &kf2) ;
};

template <class T> class LinearKeyFrameInterpolator {
    public:
    static T interpolate(float t, const T &kf1, const T &kf2) {
        return t * kf2 + ( 1.0f - t ) * kf1 ;
    }
};

template <class S> class LinearKeyFrameInterpolator<Eigen::Quaternion<S>> {
public:
    static Eigen::Quaternion<S> interpolate(float t, const Eigen::Quaternion<S> &kf1, const Eigen::Quaternion<S> &kf2) {
        return kf1.slerp(t, kf2) ;
    }
};

template <typename T>
class KeyFrameSampler {
public:
    virtual T interpolate(const TimeLine<T> &data, float fraction) const = 0 ;
};

template <typename T, template<class S> class Interpolator = LinearKeyFrameInterpolator>
class LinearKeyFrameSampler: public KeyFrameSampler<T> {
public:

    virtual T interpolate(const TimeLine<T> &data, float fraction) const override {

        int numKeyFrames = data.numKeyFrames() ;

        assert(numKeyFrames > 1) ;

        const auto &firstKeyFrame = data.getKeyFrame(0) ;
        const auto &lastKeyFrame = data.getKeyFrame(numKeyFrames-1) ;

        if ( fraction <= firstKeyFrame.getTimeStamp() ) { // extrapolate using the first two key frames
            const auto &nextKeyFrame = data.getKeyFrame(1) ;

            float intervalFraction = ( fraction - firstKeyFrame.getTimeStamp() ) /
                      ( nextKeyFrame.getTimeStamp() - firstKeyFrame.getTimeStamp() );
            return Interpolator<T>::interpolate(intervalFraction, firstKeyFrame.getValue(),  nextKeyFrame.getValue());
        } else if ( fraction >= lastKeyFrame.getTimeStamp() ) {
            const auto &prevKeyFrame = data.getKeyFrame(numKeyFrames - 2);

            float intervalFraction = ( fraction - prevKeyFrame.getTimeStamp() ) /
                      ( lastKeyFrame.getTimeStamp() - prevKeyFrame.getTimeStamp());
            return Interpolator<T>::interpolate(intervalFraction, prevKeyFrame.getValue(),
                          lastKeyFrame.getValue());
        }

        KeyFrame<T> prevKeyFrame = firstKeyFrame ;

        for (int i = 1; i < numKeyFrames; ++i ) {
            const auto &nextKeyFrame = data.getKeyFrame(i);
            if ( fraction < nextKeyFrame.getTimeStamp() ) {
                 float intervalFraction = (fraction - prevKeyFrame.getTimeStamp()) /
                          ( nextKeyFrame.getTimeStamp() - prevKeyFrame.getTimeStamp()) ;
                 return Interpolator<T>::interpolate(intervalFraction, prevKeyFrame.getValue(),
                              nextKeyFrame.getValue());
            }
            prevKeyFrame = nextKeyFrame;
        }

        // shouldn't reach here
        return lastKeyFrame.getValue();

    }

};



} // namespace viz
} // namespace cvx

#endif

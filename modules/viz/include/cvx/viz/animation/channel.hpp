#ifndef CVX_VIZ_CHANNEL_HPP
#define CVX_VIZ_CHANNEL_HPP

#include <cvx/viz/animation/timeline.hpp>
#include <cvx/viz/animation/easing.hpp>
#include <cvx/viz/animation/keyframe_sampler.hpp>

namespace cvx { namespace viz {

// An abstract channel holds the value to be animated and updates its value its time the update function is called
// The time value t should be between 0 and 1 where 1 corresponds to animation cycle duration

class AbstractChannel {
public:
    virtual void update(float t) = 0 ;
};

// a standard channel over a specific type that intepolates key frames

template <class S> class TimeLineChannel: public AbstractChannel {
public:
    TimeLineChannel(const TimeLine<S> &f, const KeyFrameSampler<S> &sampler, const EasingCurve &ec):
    data_(f), sampler_(sampler), easing_curve_(ec) {}

    virtual void update(float ts) override {
        float tt = easing_curve_.value(ts) ;
        value_ = sampler_.interpolate(data_, tt) ;
    }

    const S &getValue() const { return value_ ; }

protected:

    S value_ ;
    const TimeLine<S> &data_ ;
    const KeyFrameSampler<S> &sampler_ ;
    const EasingCurve &easing_curve_ ;
};



} // namespace viz
} // namespace cvx

#endif

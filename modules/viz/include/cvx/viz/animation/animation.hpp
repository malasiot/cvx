#ifndef CVX_VIZ_ANIMATION_HPP
#define CVX_CIZ_ANIMATION_HPP

#include <memory>
#include <vector>
#include <limits>
#include <cmath>

namespace cvx { namespace viz {

template<class T>
class KeyFrame {
public:
    KeyFrame(float t, T v): t_(t), value_(v) {}

    float getTimeStamp() const { return t_ ; }
    const T &getValue() const { return value_ ; }

    float t_ ;
    T value_ ;
};

template<class T>
class KeyFrameSet {
public:


    KeyFrameSet(std::initializer_list<KeyFrame<T>> init): key_frames_(init) {
        assert(key_frames_.size() > 1) ;
        computeDuration() ;
    }

    float getDuration() const { duration_ ; }

    int numKeyFrames() const { return key_frames_.size() ; }

    const KeyFrame<T> &getKeyFrame(int idx) const { return key_frames_.get(idx) ; }

private:

    void computeDuration() {
        duration_ = key_frames_.back().t_ ;
    }

    std::vector<KeyFrame<T>> key_frames_ ;
    float duration_ ;
};

class EasingCurve {
public:
    virtual float value(float fraction) const = 0 ;
};

class LinearEasingCurve: public EasingCurve {
public:
    float value(float fraction) const override {
        return fraction ;
    }
};

template <class T> class KeyFrameInterpolator {
    public:
    static T interpolate(float t, KeyFrame<T> &kf1, const KeyFrame<T> &kf2) ;
};

template <class T> class LinearKeyFrameInterpolator {
    public:
    static T interpolate(float t, KeyFrame<T> &kf1, const KeyFrame<T> &kf2) {
        return t * kf1.getValue() + ( 1.0f - t ) * kf2.getValue() ;
    }
};

template <typename T>
class KeyFrameSampler {
    virtual T interpolate(const KeyFrameSet<T> &data, float fraction) = 0 ;
};

template <typename T, template<class S> class Interpolator = LinearKeyFrameInterpolator>
class LinearKeyFrameSampler {
public:

    virtual T interpolate(const KeyFrameSet<T> &data, float fraction) override {

        assert(data.size() > 2) ;

        int numKeyFrames = data.numKeyFrames() ;
        const auto &firstKeyFrame = data.getKeyFrame(0) ;
        const auto &lastKeyFrame = data.getKeyFrame(numKeyFrames-1) ;

        if ( fraction <= firstKeyFrame.getTimeStamp() ) { // extrapolate using the first two key frames
            const auto &nextKeyFrame = data.getKeyFrame(1) ;

            float intervalFraction = ( fraction - firstKeyFrame.getTimeStamp() ) /
                      ( nextKeyFrame.getTimeStamp() - firstKeyFrame.getTimeStamp() );
            return Interpolator<T>::interpolate(intervalFraction, firstKeyFrame.getValue(),  nextKeyFrame.getValue());
        } else if ( fraction >= lastKeyFrame.getTimeStamp() ) {
            const auto &prevKeyFrame = data.get(numKeyFrames - 2);

            float intervalFraction = ( fraction - prevKeyFrame.getTimeStamp() ) /
                      ( lastKeyFrame.getTimeStamp() - prevKeyFrame.getTimeStamp());
            return Interpolator<T>::interpolate(intervalFraction, prevKeyFrame.getValue(),
                          lastKeyFrame.getValue());
        }

        const auto & prevKeyFrame = firstKeyFrame ;

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


// An abstract channel holds the value to be animated and updates its value its time the update function is called
// The time value t should be between 0 and duration

class AbstractChannel {
public:
    virtual float getDuration() const = 0 ;
    virtual void update(float t) = 0 ;
};

// a standard channel over a specific type that intepolates key frames
template <class S> class KeyFrameBasedChannel: public AbstractChannel {
public:
    KeyFrameBasedChannel(const KeyFrameSet<S> &f, const KeyFrameSampler<S> &sampler, const EasingCurve &ti) {
        duration_ = f.getDuration() ;
    }

    virtual void update(float ts) override {
        float tt = ti_.value(ts/duration_) ;
        value_ = sampler_.interpolate(data_, tt * duration_) ;
    } ;

    float getDuration() const override { return duration_ ; }

protected:

    S value_ ;
    float duration_ ;
    const KeyFrameSet<S> &data_ ;
    const KeyFrameSampler<S> &sampler_ ;
    const EasingCurve &ti_ ;
};


// This should be overriden to return an api specific elapsed time in the same units as the animation data

class AnimationTimer {
public:
    AnimationTimer() = default ;

    virtual float getTime() const = 0 ;
};


class Animation {
public:
    enum RepeatMode {  RESTART, REVERSE  } ;
    enum State { RUNNING, PAUSED, STOPPED } ;

    // define an animation of specific duration for each repetition cycle

    Animation(AnimationTimer *timer): timer_(timer) {}

    // set/get the number of repetitions

    void setRepeatCount(int value) {
       repeat_count_ = value ;
    }

    int getRepeatCount() const {
       return repeat_count_ ;
    }

    // set/get the repetition mode when the animation reaches the end of the cycle
    // RESTART: start the animation from the beginning, REVERSE: start the animation from the target value towards the initial value

    void setRepeatMode(RepeatMode value) {
        repeat_mode_ = value ;
    }

    RepeatMode getRepeatMode() const {
        return repeat_mode_ ;
    }

    void addChannel(const AbstractChannel *e) {
        channels_.emplace_back(e) ;
        cycle_duration_ = std::max(cycle_duration_, e->getDuration()) ;
    }

    void update() {

        if ( state_ == RUNNING ) {
            float elapsed = timer_->getTime() - start_time_ ;
            bool done = false ;

            float fraction = cycle_duration_ > 0 ? elapsed / cycle_duration_ : 1.f;
            if ( fraction >= 1.f ) {
                if ( cycle_ < repeat_count_ || repeat_count_ == -1 ) {

                    if ( repeat_mode_ == REVERSE )
                        reverse = !reverse ;

                    cycle_ += (int)fraction ;
                    fraction = fmod(fraction, 1.0f) ;

                    start_time_ += cycle_duration_ ;
                } else {
                    done = true ;
                    fraction = std::min(fraction, 1.0f);
                }
             }

            if (  reverse )
                fraction = 1.f - fraction;

            for( AbstractChannel *channel: channels_ ) {
                channel->update(fraction) ;
            }

            if ( done ) stop() ;
        }
    }

    void play() {
        if ( state_ == PAUSED ) {
            start_time_ = paused_time_ ;
        } else if ( state_ == STOPPED ) {
            start_time_ = timer_->getTime() ;
        }
        state_ = RUNNING ;
    }

    void pause() {
        if ( state_ != PAUSED ) {
            paused_time_ = timer_->getTime() ;
            state_ = PAUSED ;
        }
    }
    void stop() {
        if ( state_ != STOPPED ) {
            state_ = STOPPED ;
        }
    }


private:
    AnimationTimer *timer_ ;
    float cycle_duration_ = 0.f ;
    float start_time_ = 0.f, paused_time_ = 0.0 ;
    float cycle_time_ = 0.0f ;
    bool reverse = false ;
    State state_ = STOPPED ;
    int repeat_count_ = -1 ; // infinite
    RepeatMode repeat_mode_ = REVERSE ; // repeat mode
    float fraction_ = 0; // the percentage of the animation [0, 1] within the current cycle
    int cycle_ = 0; // the current cycle ;
    std::vector<AbstractChannel *> channels_ ;
};





} // namespace viz
} // namespace cvx
#endif

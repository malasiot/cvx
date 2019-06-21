#ifndef CVX_VIZ_ANIMATION_HPP
#define CVX_CIZ_ANIMATION_HPP

#include <memory>
#include <vector>

namespace cvx { namespace viz {

class TimeInterpolator {
public:
    virtual float value(float fraction) const = 0 ;
};

class LinearInterpolator: public TimeInterpolator {
public:
    float value(float fraction) const override {
        return fraction ;
    }
};

template <class T> class Evaluator {
public:
    Evaluator(TimeInterpolator *ti) ;
    virtual T eval(float ts, KeyFrameSet<T> f) ;

protected:
    TimeInterpolator *ti_ ;
};

template<class T>
class KeyFrame {
public:
    KeyFrame(float t, T v): t_(t), value_(v) {}
    float t_ ;
    T value_ ;
};

template<class T>
class KeyFrameSet {
public:

    KeyFrameSet(T start, T stop) {
        key_frames_.emplace_back(0.0f, start) ;
        key_frames_.emplace_back(0.0f, stop) ;
    }

    KeyFrameSet(std::initializer_list<KeyFrame<T>> init): key_frames_(init) {
        assert(key_frames_.size() > 1) ;
    }

    std::vector<KeyFrame<T>> key_frames_ ;
};


template <class T>
class LinearScalarEvaluator: public Evaluator<T> {
public:
    T eval(float ts, KeyFrameSet<T> f) {
        for (int i = 1; i < f.key_frames_.size(); ++i) {
            const KeyFrame<T> &kf = f.key_frames_[i] ;
            if ( ts < kf.t_ ) {

            }
    }
};



template <class T>
class PropertyAnimation {
public:
    enum RepeatMode {  RESTART, REVERSE  } ;

    // define an animation of specific duration for each repetition cycle

    PropertyAnimation(T start, T stop, float duration): start_value_(start), stop_value_(stop), duration_(duration),
        interpolator_(new LinearInterpolator()),
        evaluator_(new LinearScalarEvaluator<T>() ){} ;

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

    // set/get the initial delay of the animation

    void setStartDelay(float delay) {
        start_delay_ = delay ;
    }

    float getStartDelay() const {
        return start_delay_ ;
    }

    void setTimeInterpolator(TimeInterpolator *i) {
        interpolator_.reset(i) ;
    }

    void setEvaluator(Evaluator<T> *e) ;

    // get the current value of the animated property
    T getValue() const ;

private:
    float duration_ ;
    float start_delay_ = 0.0f ;
    int repeat_count_ = -1 ; // infinite
    RepeatMode repeat_mode_ = REVERSE ; // repeat mode
    float fraction_ = 0; // the percentage of the animation [0, 1] within the current cycle
    int cycle_ = 0; // the current cycle ;

    std::unique_ptr<TimeInterpolator> interpolator_ ;
    std::unique_ptr<Evaluator<T>> evaluator_ ;
    T start_value_, stop_value_ ;
};





} // namespace viz
} // namespace cvx
#endif

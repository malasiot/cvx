#ifndef CVX_VIZ_ANIMATION_HPP
#define CVX_VIZ_ANIMATION_HPP

#include <memory>
#include <vector>
#include <limits>
#include <cmath>


#include <cvx/viz/animation/channel.hpp>

namespace cvx { namespace viz {



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

    void setDuration(float value) {
       cycle_duration_ = value ;
    }

    float getDuration() const {
       return cycle_duration_ ;
    }

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

    void addChannel(AbstractChannel *e) {
        channels_.emplace_back(e) ;
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
    bool reverse = false ;
    State state_ = STOPPED ;
    int repeat_count_ = -1 ; // infinite
    RepeatMode repeat_mode_ = REVERSE ; // repeat mode
    int cycle_ = 0; // the current cycle ;
    std::vector<AbstractChannel *> channels_ ;
};





} // namespace viz
} // namespace cvx
#endif

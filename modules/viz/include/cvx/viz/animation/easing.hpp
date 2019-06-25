#ifndef CVX_VIZ_EASING_HPP
#define CVX_VIZ_EASING_HPP

#include <cmath>

namespace cvx { namespace viz {

// easing curve that warps normalized time to create accelaration effects
class EasingCurve {
public:
    virtual float value(float fraction) const = 0 ;
};

class EaseLinear: public EasingCurve {
public:
    float value(float fraction) const override {
        return fraction ;
    }
};

class EaseInQuad: public EasingCurve {
public:
    float value(float t) const override {
        return t * t ;
    }
} ;

class EaseOutQuad: public EasingCurve {
public:
    float value(float t) const override {
        return t * (2.f - t) ;
    }
} ;

class EaseInOutQuad: public EasingCurve {
public:
    float value(float t) const override {
        return t <.5f ? 2*t*t : -1+(4-2*t)*t ;
    }
} ;

class EaseInCubic: public EasingCurve {
public:
    float value(float t) const override {
        return t*t*t ;
    }
} ;

class EaseOutCubic: public EasingCurve {
public:
    float value(float t) const override {
        return 1 - pow( 1 - t, 3);
    }
} ;

class EaseInOutCubic: public EasingCurve {
public:
    float value(float t) const override {
        return t < 0.5 ? 4 * t * t * t : 1 - pow( -2 * t + 2, 3 ) / 2;
    }
} ;

class EaseInSine: public EasingCurve {
public:
    float value(float t) const override {
        return 1 - cos( t * M_PI / 2.0 ) ;
    }
} ;

class EaseOutSine: public EasingCurve {
public:
    float value(float t) const override {
        return sin( t * M_PI / 2 );
    }
} ;

class EaseInOutSine: public EasingCurve {
public:
    float value(float t) const override {
        return -( cos( M_PI * t ) - 1 ) / 2;
    }
} ;

class EaseInElastic: public EasingCurve {
public:

    float value(float t) const override {
        static const float c4 =  2 * M_PI / 3 ;
        if ( t == 0.0f ) return 0.f ;
        if ( t == 1.0f ) return 1.f ;
        return -pow( 2.f, 10.f * t - 10.f ) * sin( ( t * 10.f - 10.75f ) * c4 );
    }
} ;

class EaseOutElastic: public EasingCurve {
public:
    float value(float t) const override {
        static const float c4 =  2 * M_PI / 3 ;
        if ( t == 0.0f ) return 0.f ;
        if ( t == 1.0f ) return 1.f ;
        return pow( 2.f, -10.f * t ) * sin( ( t * 10.f - 0.75f ) * c4 ) + 1.f;
    }
} ;

class EaseInOutElastic: public EasingCurve {
public:
    float value(float t) const override {
        static const float c5 =  2 * M_PI / 4.5 ;
        if ( t == 0.0f ) return 0.f ;
        if ( t == 1.0f ) return 1.f ;
        return t < 0.5f ?
             -( pow( 2, 20 * t - 10 ) * sin( ( 20 * t - 11.125 ) * c5 )) / 2 :
             pow( 2, -20 * t + 10 ) * sin( ( 20 * t - 11.125 ) * c5 ) / 2 + 1;
    }
} ;



} // namespace viz
} // namespace cvx

#endif

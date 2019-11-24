#include <cvx/gfx/plot/tics.hpp>
#include <cmath>

using namespace std ;

namespace cvx { namespace gfx {

double AutoTickLocator::sround(double x) {
    double s = 1.0 ;

    if ( fabs(x) < 1.0e-10 ) return 0.0 ;

    while ( x * s < 1.0 ) s *= 10 ;
    while ( x * s > 10.0 ) s /= 10 ;

    double sx = s * x ;

    if ( sx >= 1.0 && sx < 2.0 ) return 1.0/s ;
    else if ( sx >= 2.0 && sx < 5.0 ) return 2.0/s ;
    else return 5.0/s ;

    return s ;
}

void AutoTickLocator::bounds(double sx, double xu, double xl, unsigned &nTics, double &rxu, double &rxl)
{
    int i, n = (int)(xu/sx) ;

    for(i=n+1 ; i>=n-1 ; i--) {
        if ( i * sx - xu <= 1.0e-4 ) break ;
    }

    rxu = i * sx ;

    n = (int)(xl/sx) ;

    for(i=n-1 ; i<=n+1 ; i++) {
        if ( i * sx - xl >= -1.0e-4 ) break ;
    }

    rxl = i * sx ;

    nTics = (rxl - rxu)/sx + 1.5;
}


void AutoTickLocator::compute(double vmin, double vmax, uint maxTics, double minStep, double &label_min, double &label_max, std::vector<double> &loc)
{
    uint tics = maxTics ;
    uint numTics ;
    double step ;

    label_max = vmax ;
    while (1)
    {
       step = sround((vmax - vmin)/(tics-1)) ;
       if ( step < minStep ) step = minStep ;

        bounds(step, vmin, vmax, numTics, label_min, label_max) ;

        if ( numTics <= maxTics ) break ;
        else tics -- ;

        if ( tics == 1 ) {
            numTics = 2 ;
            step = label_max - label_min ;
            break ;
        }
    }

    // step_ = sround(ticStep*vscale_) ;
    // bounds(step_, _min*vscale_, _max*vscale_, numTics, min_label_v_, max_label_v_) ;

    double v = label_min ;
    for( uint i=0 ; i<numTics ; i++ ) {
        loc.push_back(v) ;
        v += step ;
    }
}

void FixedTickLocator::compute(double vmin, double vmax, uint maxTics, double minStep, double &label_min, double &label_max, std::vector<double> &loc)
{
    label_min = vmin ;
    label_max = vmax ;

    for ( int i=0 ; i<loc_.size() ; i++ ) {
        if ( loc_[i] < vmin || loc_[i] > vmax ) continue ;
        loc.push_back(loc_[i]) ;
    }
}



}}

#ifndef CVX_RNG_HPP
#define CVX_RNG_HPP

#include <random>
#include <algorithm>

namespace cvx {

class RNG {
public:
    RNG() ;
    RNG(uint64_t seed) ;


    template <class T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    uniform(T min_v, T max_v) {
        std::uniform_int_distribution<> dis(min_v, max_v);
        return dis(generator_) ;
    }

    template <class T>
    typename std::enable_if<std::is_floating_point<T>::value, T>::type
    uniform(T min_v, T max_v) {
        std::uniform_real_distribution<> dis(min_v, max_v);
        return dis(generator_) ;
    }


    // uniform random number in [0, 1)

    template <class T>
    T uniform() {
        return uniform(static_cast<T>(0), static_cast<T>(1)) ;
    }

    template <class T>
    void uniform(std::vector<typename std::enable_if<std::is_integral<T>::value, T>::type> &vec, T min_v, T max_v) {
        std::uniform_int_distribution<> dis(min_v, max_v);
        std::generate(vec.begin(), vec.end(), [&] { return dis(generator_);}) ;
    }

    template <class T>
    void uniform(std::vector<typename std::enable_if<std::is_floating_point<T>::value, T>::type> &vec, T min_v, T max_v) {
        std::uniform_real_distribution<> dis(min_v, max_v);
        std::generate(vec.begin(), vec.end(), [&] { return dis(generator_);}) ;
    }

    template <class T>
    T choice(const std::vector<T> &v) {
        assert(!v.empty()) ;
        std::uniform_int_distribution<> dis(0, v.size()-1);
        return v[dis(generator_)] ;
    }

    double gaussian(double mean, double sigma) {
        std::normal_distribution<double> dis(mean, sigma);
        return dis(generator_) ;
    }

    float gaussian(float mean, float sigma) {
        std::normal_distribution<float> dis(mean, sigma);
        return dis(generator_) ;
    }

    // gaussian with mean 0 and variance 1

    double gaussian() {
        std::normal_distribution<double> dis(0, 1);
        return dis(generator_) ;
    }

    // generate a random sequence of N unique integers

    template <class T>
    void sequence(uint64_t N, std::vector<T> &seq) {
        assert(std::is_integral<T>()) ;
        for( uint64_t i=0 ; i<N ; i++ ) seq.push_back(T(i)) ;
        shuffle(seq) ;
    }

    // random shuffling of a sequence
    template <class T>
    void shuffle(std::vector<T> &seq) {
        std::shuffle(seq.begin(), seq.end(), generator_) ;
    }

    // sample without replacement n numbers from the set [0, N)
    template<class T>
    void sample(uint32_t n, uint64_t N, std::vector<T> &subset) {

        uint64_t max_idx = N-1 ;

        std::vector<T> vidx ;
        for( uint64_t i=0 ; i<N ; i++ ) vidx.push_back(i) ;

        for ( int i=0; i<n ; i++ ) {
            std::uniform_int_distribution <> ud(0, max_idx) ;
            int index = ud(generator_) ;
            std::swap(vidx[index], vidx[max_idx]);
            subset.push_back(vidx[max_idx]);
            max_idx -- ;
        }
    }

    typedef std::mt19937_64 rng_t ;

    rng_t &generator() { return generator_ ; }

private:

    rng_t generator_ ;
};



}

#endif

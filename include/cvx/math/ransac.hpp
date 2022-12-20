#ifndef CVX_RANSAC_HPP
#define CVX_RANSAC_HPP

#include <cvx/math/rng.hpp>
#include <cassert>

namespace cvx {
/*
 *  The user has to define a model class that implements the following concept
 *  struct Model {
 *      int minSamples ; // should be set to the minimum number of samples required to fit the model
 *      bool fit(const vector<size_t> &subset, ModelParams &params) ; // fits the model to a subset of the data
 *              // may return false if the resulting model is not valid
 *      void findInliers(const ModelParams &params, vector<size_t> &inliers) ; // return the data indexes that are considered inliers
 *      float computeResidual(const vector<size_t> &inliers) ; // fit the model onto the inliers and compute resiudal
 *  };
 *
 *  Optionally you can supply a sampling function of the form:
 *  sampler(RNG &rng, int n, int total, vector<size_t> &samples)
 *
 */
class RANSAC {
public:

    // min_inliers is the minimum numbers of inliers to assume that the model fir is good
    explicit RANSAC(int max_trials, int min_inliers): max_trials_(max_trials), min_inliers_(min_inliers) {}
    RANSAC() = default ;

    using Sampler = std::function<bool(RNG&, int, int, std::vector<size_t>&)> ;

    static bool defaultSampler(RNG &rng, int n, int total, std::vector<size_t> &samples) {
        rng.sample(n, total, samples) ;
        return true ;
    }

    struct best_t{
        float residual_ = std::numeric_limits<double>::max();
        std::vector<size_t> subset_;
    } ;

    template <typename Model, typename Params>
    bool estimate(size_t n, Model &model, std::vector<size_t> &best_subset, Params &best_params, Sampler sampler = defaultSampler ) {
        size_t N = Model::minSamples ;

        assert( N <= n ) ;

#pragma omp declare reduction(get_min : struct best_t :\
    omp_out = (omp_in.residual_ < omp_out.residual_) ? omp_in : omp_out)\
    initializer (omp_priv=best_t{})

        best_t best ;
#pragma omp parallel for shared(model, sampler, N, n, rng_) reduction(get_min : best) schedule(static)
        for( size_t trial_count = 0 ; trial_count < max_trials_ ; trial_count ++)  {
            std::vector<size_t> subset, inliers ;
            Params params ;
            if ( sampler(rng_, N, n, subset) ) {

                if ( model.fit(subset, params) ) { // try to fit the model to the subset of measurements returning the obtained model parameters and a success flag

                    // compute inliers given the model parameters
                    model.findInliers(params, inliers) ;

                    if ( inliers.size() > min_inliers_ ) {

                        float residual = model.computeResidual(inliers) ;

                        if ( residual < best.residual_ ) {
                            best = best_t{residual, inliers};
                        }
                    }
                }
            }
        }


        if ( best.subset_.empty() ) return false ;

        best_subset = best.subset_ ;
        return model.fit(best_subset, best_params);
    }

protected:
    int min_inliers_ ;
    int max_trials_ ;
    RNG rng_ ;
};




}
#endif

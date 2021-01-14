#ifndef CVX_UTIL_ML_DATASET_HPP
#define CVX_UTIL_ML_DATASET_HPP

#include <vector>
#include <cvx/math/rng.hpp>
#include <Eigen/Core>

namespace cvx {


// dataset concept, S: sample type, L: target type (e.g. label for classification or abscissa for regression)

template<typename S, typename L>
class IDataSet
{
public:

    typedef uint64_t sample_idx_t ;
    typedef L target_t ;
    typedef S sample_t ;

    // Get number of samples available
    uint64_t size() const ;

    // Get target associated with the specific sample
    target_t getTarget(sample_idx_t idx) const ;

    sample_t getSample(sample_idx_t idx) const ;
} ;

template<class T, typename L>
class MatDataset
{
public:
    typedef uint64_t sample_idx_t ;
    typedef T coord_t ;
    typedef L target_t ;
    typedef typename Eigen::Matrix<T, Eigen::Dynamic, 1> sample_t ;

    // Get number of samples available
    uint64_t size() const {
        return data_.rows() ;
    }

    // Get dimension
    uint32_t dimensions() const {
        return data_.cols() ;
    }

    // this is the number of unique labels of the dataset in case of classification
    uint64_t numLabels() const { return labels_.size() ; }

    // Get label associated with the specific sample
    target_t getTarget(sample_idx_t idx) const { return labels_[targets_[idx]] ; }
    uint64_t getTargetIndex(sample_idx_t idx) const { return targets_[idx] ; }

    T getSampleCoordinate(sample_idx_t idx, uint32_t c) const { return data_(idx, c) ; }

    sample_t getSample(sample_idx_t idx) const { return data_.row(idx) ; }

    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> getMat() const { return data_ ; }

    void getTargets(std::vector<L> &targets) const {
        if ( !targets_.empty() ) {
            for(uint i=0 ; i<targets_.size() ; i++ )
                targets.push_back(labels_[targets_[i]]) ;
        }
        else
            targets = labels_ ;
    }

protected:

    std::vector<uint32_t> targets_ ; // points to labels
    Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> data_ ;
    std::vector<L> labels_ ;
} ;


template<typename D>
class ShuffledDataset {
public:

    typedef typename D::target_t target_t ;
    typedef typename D::sample_t sample_t ;
    typedef typename D::coord_t coord_t ;
    typedef typename D::sample_idx_t sample_idx_t ;

    ShuffledDataset( const D &base, uint64_t n ): base_(base) {
        assert(n <= base.size()) ;
        cvx::RNG rng ;
        rng.sample(n, base.size(), perm_) ;
    }

    ShuffledDataset( const D &base, uint64_t n, cvx::RNG &rng ): base_(base) {
        assert(n <= base.size()) ;
        rng.sample(n, base.size(), perm_) ;
    }

    ShuffledDataset( const D &base, const std::vector<uint64_t> &perm ): base_(base), perm_(perm) {
        assert(perm_.size() <= base.size()) ;
    }

    // Get number of samples available
    uint64_t size() const {
        return perm_.size() ;
    }

    // Get dimension
    uint32_t dimensions() const {
        return base_.dimensions() ;
    }

    // this is the number of unique labels of the dataset in case of classification
    uint64_t numLabels() const { return base_.numLabels() ; }

    // Get label associated with the specific sample
    target_t getTarget(sample_idx_t idx) const { return base_.getTarget(perm_[idx]) ; }
    uint64_t getTargetIndex(sample_idx_t idx) const { return base_.getTargetIndex(perm_[idx]) ; }

    coord_t getSampleCoordinate(sample_idx_t idx, uint32_t c) const {
        return base_.getSampleCoordinate(perm_[idx], c) ;
    }

    sample_t getSample(sample_idx_t idx) const {
        return base_.getSample(perm_[idx]) ;
    }

    const std::vector<uint64_t> &index() const { return perm_ ; }

protected:

    std::vector<uint64_t> perm_ ;
    const D &base_ ;
};

template<typename D>
class KFold: public ShuffledDataset<D> {
    typedef ShuffledDataset<D> base_t ;

public:

    KFold(uint32_t k, const D &base ): ShuffledDataset<D>(base, base.size()), K_(k) {}
    KFold(uint32_t k, const D &base, cvx::RNG &rng ): ShuffledDataset<D>(base, base.size(), rng), K_(k) {}

    ShuffledDataset<D> trainingSet(uint32_t k) {
        assert( k < K_) ;
        uint64_t n = base_t::base_.size() ;
        uint64_t fsize = n/K_ ;
        uint64_t idx = k * fsize ;
        std::vector<uint64_t> idxs ;
        for( uint64_t i=idx ; i< std::min(n, idx+fsize) ; i++ )
            idxs.push_back(base_t::perm_[i]) ;

        return ShuffledDataset<D>(base_t::base_, idxs) ;
    }

    ShuffledDataset<D> testSet(uint32_t k) {
        assert( k < K_ ) ;
        uint64_t n = base_t::base_.size() ;
        uint64_t fsize = n/K_ ;
        uint64_t idx1 = k * fsize ;
        uint64_t idx2 = std::min(n, idx1+fsize) ;
        std::vector<uint64_t> idxs ;
        for( uint64_t i=0 ; i< n ; i++ )
            if ( i < idx1 || i >= idx2 )
                idxs.push_back(base_t::perm_[i]) ;

        return ShuffledDataset<D>(base_t::base_, idxs) ;
    }


private:

    uint32_t K_ ;
};




}

#endif

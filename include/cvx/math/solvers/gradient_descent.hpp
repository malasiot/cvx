#ifndef CVX_GRADIENT_DESCENT_SOLVER_HPP
#define CVX_GRADIENT_DESCENT_SOLVER_HPP

#include <cvx/math/solvers/line_search.hpp>
#include <functional>
#include <Eigen/Core>

// Gradient descent optimizer
//
// The objective function is a functor of the form
//
// struct ObjFunc {
//    float value(const VectorXf &x) ;
//    void  gradient(const VectorXf &x, VectorXf &grad) ;
// } ;

namespace cvx {

template<typename T, typename ObjFunc, typename LS = MoreThuente<T, ObjFunc, 1> >
class GradientDescentSolver {
public:


    struct Parameters {
        T g_tol_, x_tol_ ;
        uint max_iter_ ;

        T rate_ ; // learning rate if zero then linesearch is performed to determine best rate

        LineSearchParams<T> ls_ ;

        Parameters(): g_tol_(1.0e-15), x_tol_(1.0e-7), max_iter_(30), rate_(0.0) {}
    };

    GradientDescentSolver() {}
    GradientDescentSolver(const Parameters &params): params_(params) {}

    typedef Eigen::Matrix<T, Eigen::Dynamic, 1> Vector;
    typedef Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> Matrix;
    using ProgressFunc = std::function<void (const Vector &x, const Vector &g, T f, uint iter)> ;

    void minimize(ObjFunc &obj_func, Vector &x0, ProgressFunc prog = nullptr) {

        const size_t d = x0.rows();
        size_t iter = 0;

        Vector grad(d) ;
        T grad_norm = 0 ;
        Vector x_old = x0;

        do {

            obj_func.gradient(x0, grad);

            if ( prog ) {
               T f = obj_func.value(x0) ;
               prog(x0, grad, f, iter) ;

            }

            Vector search_dir = -grad;

            T rate = params_.rate_ ;

            if ( rate == 0 )
                rate = LS::linesearch(params_.ls_, x0, search_dir, obj_func, obj_func.value(x0), grad ) ;

            x0 = x0 + rate * search_dir ; // update solution

            grad_norm = grad.template lpNorm<Eigen::Infinity>();

            if ( (x_old - x0).template lpNorm<Eigen::Infinity>() < params_.x_tol_  ) break;
            x_old = x0;
            iter++;

        } while ( (grad_norm > params_.g_tol_ ) && (iter < params_.max_iter_));
    }

    Parameters params_ ;


};

}

#endif

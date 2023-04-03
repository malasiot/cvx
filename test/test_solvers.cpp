#include <cvx/math/solvers/bfgs.hpp>
#include <cvx/math/solvers/lbfgs.hpp>
#include <cvx/math/solvers/gradient_descent.hpp>

#include <iostream>

using namespace std ;
using namespace cvx ;
using namespace Eigen ;

class Rosenbrock {
  public:

    // this is just the objective (NOT optional)
    float value(const VectorXf &x) {
        const float t1 = (1 - x[0]);
        const float t2 = (x[1] - x[0] * x[0]);
        return t1 * t1 + 100 * t2 * t2;
    }


    void gradient(const VectorXf &x, VectorXf &grad) {
        grad[0]  = -2 * (1 - x[0]) + 200 * (x[1] - x[0] * x[0]) * (-2 * x[0]);
        grad[1]  =                   200 * (x[1] - x[0] * x[0]);
    }
};

int main(int argc, char *argv[]) {

    LBFGSSolver<float, Rosenbrock> solver ;

    Rosenbrock f;
        // choose a starting point
    VectorXf x(2);
    x << 0, 0;

    solver.params_.max_iter_ = 1000 ;
    solver.minimize(f, x, [](const VectorXf &x, const VectorXf &g, float f, uint iter) {
        cout << iter << ' ' << f << endl ;
    });
        // print argmin
    std::cout << "argmin      " << x.transpose() << std::endl;
    std::cout << "f in argmin " << f.value(x) << std::endl;
}


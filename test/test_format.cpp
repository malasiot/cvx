#include <cvx/misc/format.hpp>

#include <cmath>

using namespace cvx ;
using namespace std ;

int main(int argc, char *argv[]) {

    const char *fmt = "{p45}\n";

    format(cout, fmt, M_PI) ;
}

#include <cvx/util/misc/format.hpp>

#include <cmath>

using namespace cvx::util ;
using namespace std ;

int main(int argc, char *argv[]) {

    const char *fmt = "{p45}\n";

    format(cout, fmt, M_PI) ;
}

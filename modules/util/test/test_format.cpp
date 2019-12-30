#include <cvx/util/misc/format.hpp>

using namespace cvx::util ;
using namespace std ;

int main(int argc, char *argv[]) {

    const char *fmt = "%2$s @%1$6.4g@ %3$10.2f\n";
    printf(fmt, 0.34, "hello", 2.4);

    format(cout, fmt, 0.34, "hello", 2.4) ;
}

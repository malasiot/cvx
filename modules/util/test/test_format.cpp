#include <cvx/util/misc/format.hpp>

#include <cmath>

using namespace cvx::util ;
using namespace std ;

int main(int argc, char *argv[]) {

    const char *fmt = "{}\n";

    double inf = std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();
    auto s0 = format("{0:},{0:+},{0:-},{0: }", 1);   // value of s0 is "1,+1,1, 1"
    auto s1 = format("{0:},{0:+},{0:-},{0: }", -1);  // value of s1 is "-1,-1,-1,-1"
    auto s2 = format("{0:},{0:+},{0:-},{0: }", inf); // value of s2 is "inf,+inf,inf, inf"
    auto s3 = format("{0:},{0:+},{0:-},{0: }", nan); // value of s3 is "nan,+nan,nan, nan"

    format(cout, fmt, M_PI) ;
}

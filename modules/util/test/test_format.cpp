#include <cvx/util/misc/format.hpp>

using namespace cvx::util ;
using namespace std ;

int main(int argc, char *argv[]) {

    const char *fmt = "{} and {}\n";


    format(cout, fmt, 0.34, "hello", 2.4) ;
}

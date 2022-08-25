#include <cvx/misc/format.hpp>

#include <cmath>

using namespace cvx ;
using namespace std ;

int main(int argc, char *argv[]) {
    cout << format("{} {}", "hello", "world") << endl ;
    cout << format("{}", M_PI) << endl ;
    cout << format("{:6d}", 120) << endl ;
    cout << format("{:*<7}", 'x') << endl ;
    cout << format("We have {:>} chickens", 3) << endl ;
}

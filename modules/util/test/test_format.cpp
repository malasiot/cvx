#include <cvx/util/misc/format.hpp>

using namespace cvx::util ;
using namespace std ;

class BaseFormatter {
public:
    void format() {};
};
template<class T>
class Formatter: public BaseFormatter {
public:
    Formatter(const T &v): src_(v) {}


    const T &src_ ;

} ;

template<typename ...Args>
void dox(Args...args) {
    BaseFormatter *x[] = { new Formatter<Args>(args)... } ;

    x[0]->format() ;
}

int main(int argc, char *argv[]) {

    const char *fmt = "{} and {}\n";

    dox(2.3, "hello") ;

    format(cout, fmt, 0.34, "hello", 2.4) ;
}

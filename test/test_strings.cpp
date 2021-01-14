#include <cvx/misc/strings.hpp>
#include <cvx/misc/format.hpp>

#include <iostream>
#include <iterator>
#include <algorithm>


using namespace std ;
using namespace cvx ;

int main(int argc, char *argv[]) {


    auto s = split("2.4, 3.5  4.6 ",regex("[, ]+")) ;
    std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cout, "$"));

    string str{"hello {1} and {2}"} ;

    string res = replace(str, regex("\\{([0-9]+)\\}"), [&] ( const smatch &s ) -> string { return s[1] ; }) ;
    cout << res  << '%' << endl ;

    cout << format("{} = {}", string("hello"), 15) << endl ;

}

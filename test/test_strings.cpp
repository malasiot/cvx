#include <cvx/misc/strings.hpp>
#include <cvx/misc/format.hpp>
#include <cvx/misc/variant.hpp>
#include <cvx/misc/timer.hpp>
#include <iostream>
#include <iterator>
#include <algorithm>

#include <thread>
using namespace std ;
using namespace cvx ;

int main(int argc, char *argv[]) {

   Profiler p("code runs in: ") ;
    std::this_thread::sleep_for(15ms);


    Variant cfg = Variant::fromJSON(
                R"(
        { "algorithm": "hg",
           "hg": {
                "data_folder": ""
            }
        }
    )");
    auto s = split("2.4, 3.5  4.6 ",regex("[, ]+")) ;
    std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cout, "$"));

    string str{"hello {1} and {2}"} ;

    string res = replace(str, regex("\\{([0-9]+)\\}"), [&] ( const smatch &s ) -> string { return s[1] ; }) ;
    cout << res  << '%' << endl ;

    cout << format("{} = {}", string("hello"), 15) << endl ;

}

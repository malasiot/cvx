#include <cvx/misc/strings.hpp>
#include <cvx/misc/format.hpp>
#include <cvx/misc/variant.hpp>
#include <cvx/misc/timer.hpp>
#include <cvx/misc/config.hpp>

#include <iostream>
#include <iterator>
#include <algorithm>

#include <thread>
using namespace std ;
using namespace cvx ;

int main(int argc, char *argv[]) {

    string cfgs =
R"(
# Example application configuration file

version = "1.0";

application:
{
  window:
  {
    title = "My Application"
"hhh"
    size = { w = 640; h = 480; };
    pos = { x = 350; y = 250; };
  };

  //list = ( ( "abc", 123, true ), 1.234, ( /* an empty list */ ) );

  books = [ { title  = "Treasure Island";

              author = "Robert Louis Stevenson";
              price  = 29.95;
              qty    = 5; },
            { title  = "Snow Crash";
              author = "Neal Stephenson";
              price  = 9.99;
              qty    = 8; } ];

  misc:
  {
    pi = 3.141592654;
    bigint = 9223372036;
    columns = [ "Last Name", "First Name", "MI" ];
#    bitmask = 0x1FC3;	// hex
#    umask = 0027;	// octal. Range limited to that of "int"
  };

};
)";


    Config cofg ;
    cofg.loadString(cfgs) ;

    string title ;
   bool r= cofg.value("application.window.title", title) ;

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

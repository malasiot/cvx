#include <iostream>
#include <iterator>
#include <algorithm>

#include <cvx/misc/dir_iterator.hpp>
#include <cvx/misc/path.hpp>

#include <cvx/misc/strings.hpp>
#include <cvx/misc/zstream.hpp>
#include <cvx/misc/logger.hpp>

using namespace std ;
using namespace cvx;

void list_dirs_recursive(const Path &base) {
    for( auto &e: DirectoryListing(base) ) {
        Path p(base, e) ;

        if ( e.isDirectory() ) {
            list_dirs_recursive(p) ;
        }
        else {
                cout << p << endl ;
        }
    }
}

int main(int argc, const char *argv[]) {

    cout << Path("/i1/i2/i3/", "t1/t2") << endl;
    cout << Path("/i1/../i2//i3/") << endl;
    cout << Path("i1/../../i2/../i4").normalizedPath() << endl;
     cout << Path("../i1/./../").normalizedPath() << endl;
     cout << Path("i2/i3/").parentPath().parent() << endl ;
     cout << Path("/usr/lib/sftp-server").canonical() << endl ;


    list_dirs_recursive(Path("/home/malasiot/source/maplite")) ;

    cout << Path("/i1/i2/test").fileName() << endl ;
    cout << Path("i1").fileName() << endl ;
    cout << Path("i1/i2/file.txt").fileNameWithoutExtension() << endl ;
    cout << Path("file.txt").fileNameWithoutExtension() << endl ;
    cout << Path("i1/file").fileNameWithoutExtension() << endl ;

    cout << Path("/gg/hh") / "/ff" / "gg" << endl ;
    cout << Path::join("/gg/hh/", "ff/", "gg" ) << endl ;

    for( const auto e: Path("/i1/i2/i3") ) {
        cout << e << endl ;
    }

    Path::mkdirs("rr/") ;

    vector<string> res = Path::glob("/home/malasiot/Downloads/", "e6*.png") ;
   cout << Path::tempFilePath("xx", ".png") << endl ;

}

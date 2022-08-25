#include <cvx/misc/filesystem.hpp>
#include <cvx/misc/path.hpp>
#include <cvx/misc/strings.hpp>

#include <algorithm>
#include <fstream>

using namespace std ;

namespace cvx {

string get_file_contents(const std::string &fname) {
    ifstream strm(fname) ;

    if ( !strm.good() ) return string() ;
    strm.seekg(0, ios::end);
    size_t length = strm.tellg();
    strm.seekg(0,std::ios::beg);

    string res ;
    res.resize(length) ;
    strm.read(&res[0], length) ;

    return res ;
}

string get_data_folder(const string &app_name, int argc, char *argv[])
{
    // test if overidden in command line

    string data_dir ;

    int i ;
    for(  i=1 ; i<argc ; i++ )
    {
        if ( app_name.compare(0, 10, "--data") )
        {
            if ( i+1 < argc )
            {
                Path data_dir_arg(argv[i+1]) ;

                argc -= 2 ;

                for( int j = i ; j < argc ; j++)
                    argv[j] = argv[j+2] ;

                if ( data_dir_arg.exists() ) {
                    return data_dir_arg.canonical() ;
                }
            }
        }
    }

    Path native_app_path = Path::executablePath() ;

    // detect if running from the source tree

    Path root_path = native_app_path.parent() ;

    bool run_from_source = false ;

    Path source_path ;

    for( const string &comp: root_path.elements() )
    {
        if ( startsWith(comp, "build") ) {
            run_from_source = true ;
            break ;
        }
        else source_path /= comp ;
    }

    if ( run_from_source )
    {
        Path data_dir = source_path / "data/" ;

        if ( data_dir.exists() ) {
            return data_dir.canonicalPath().native() ;
        }

        data_dir = source_path / "tests/data/" ;

        if ( data_dir.exists() ) {
            return data_dir.canonicalPath().native() ;
        }
    }

    // test if it is in user data folder

    Path data_dir_default = Path(Path::nativeDataDir()) / app_name / "data/" ;

    if ( data_dir_default.exists() ) {
        return data_dir_default.canonicalPath().native() ;;
    }

#ifndef _WIN32
    // test if it is in home directory ( a hidden file .application_name/data/ )

    Path data_dir_home =  Path(Path::homePath()) / ("." + app_name ) / "data/" ;

    if ( data_dir_home.exists() ) {
        return data_dir_home.canonicalPath().native() ; ;
    }

    // test if it is in home directory in .ros ( a hidden file .ros/.application_name/data/ )

    Path data_dir_ros =  Path(Path::homePath()) / ".ros" / app_name  / "data/" ; ;

    if ( data_dir_ros.exists() ) {
        return data_dir_ros.canonical() ;
    }
#endif
    //test environment variable <APP_NAME>_DATA_DIR

    string var_name = toUpperCopy(app_name) + "_DATA_DIR" ;

    const char *var_dir = getenv(var_name.c_str()) ;

    if ( var_dir && Path::exists(var_dir) ) {
        return Path(var_dir).canonical() ;
    }

    // none found. set to a default location

    data_dir = data_dir_default.canonical() ;
    Path::mkdirs(data_dir) ;

    return data_dir ;

}


}


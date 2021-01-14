#ifndef CVX_UTIL_FILESYSTEM_HPP
#define CVX_UTIL_FILESYSTEM_HPP

#include <string>

namespace cvx {

// get content of file into a string

std::string get_file_contents(const std::string &fname) ;

/*
 *  Returns a platform independent location of application data. The program will search for the data folder in the following locations:
 *
 * 1. The command line for the --data parameter pointing to the directory.
 * 2. If the executable is located inside a source folder (<source_root>/build<suffix>/bin/<executable>) it will check the existence
 *    of <source_root>/data or <source_root>/tests/data/.
 * 3. <user_data>/<app_name>/data/ where <user_data> is ~/.share/data/ on Unix and CSIDL_APPDATA on Windows (e.g. C:\Users\<user>\appData\roaming\). This is
 *    the default location.
 * 4. <home_path>/.<app_name>/data/  (Unix)
 * 5. Environment variable <APP_NAME>_DATA_DIR (uppercase)
 *
 * If none found the folder in the default location will be created.
 */

std::string get_data_folder(const std::string &appName, int argc, char *argv[]) ;

}


#endif

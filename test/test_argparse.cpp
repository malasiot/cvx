#include <iostream>

#include <cvx/misc/arg_parser.hpp>
#include <cvx/misc/strings.hpp>
#include <cvx/misc/optional.hpp>

using namespace std ;
using namespace cvx ;

struct BBox {
    float min_x_, min_y_, max_x_, max_y_ ;
    bool empty_ = true ;

    BBox() = default ;
    BBox(float minx, float miny, float maxx, float maxy):
        min_x_(minx), min_y_(miny), max_x_(maxx), max_y_(maxy), empty_(false) {}

    friend std::istream &operator >> ( std::istream &strm, BBox &box ) {
        strm >>box.min_x_ >> box.min_y_ >> box.max_x_ >> box.max_y_ ;
        box.empty_ = false ;
        return strm ;
    }
};

static void test_simple(int argc, const char *argv[]) {

    ArgumentParser parser ;

    BBox bbox ;
    float f;
    int val ;
    std::vector<std::string> files ;
    std::vector<int> items ;
    int pos ;
    bool print_help = false ;
    ArgumentParser args ;
    args.description("test_argparse [options] output (input)+ \nList information about the FILEs (the current directory by default).\nSort entries alphabetically if none of -cftuvSUX nor --sort is specified.") ;

    args.option("-h|--help", "print this help message") ;
    args.option("-f|--flag", "first flag").value(f).required().implicit("2.0") ;
    args.option("-b|--bbox <minx>:<miny>:<maxx>:<maxy>", "bounding box").value(bbox).
            valueParser([&](istream &strm) {
        string s ;
        std::getline(strm, s, ':') ; bbox.min_x_ = std::stod(s) ;
        std::getline(strm, s, ':') ; bbox.min_y_ = std::stod(s) ;
        std::getline(strm, s, ':') ; bbox.max_x_ = std::stod(s) ;
        std::getline(strm, s, ':') ; bbox.max_y_ = std::stod(s) ;
        return (bool)strm ;
    }) ;
    args.option("-t|--test", "second flag").value(items) ;
    args.option("--custom", "custom flag").value(val).valueParser([&](std::istream &strm)->bool {
        strm >> val ;
        return true ;
    }) ;

  //  args.positional(files) ;

    try {
        args.parse(argc, argv) ;

        if ( args.has("--help") )
            args.printUsage(std::cout) ;

    } catch ( ArgumentParserException &e ) {
     //   if ( !print_help )
            cout << e.what() << endl ;
        args.printUsage(std::cout) ;
    }

    cout << f << endl ;

}

void test_git(int argc, const char *argv[]) {

    ArgumentParser root ;
    ArgumentParser clone_group ;
    ArgumentParser init_group ;

    root.description(R"(usage: git [--version] [--help] [-C <path>] [-c name=value]
[--exec-path[=<path>]] [--html-path] [--man-path] [--info-path]
[-p | --paginate | --no-pager] [--no-replace-objects] [--bare]
[--git-dir=<path>] [--work-tree=<path>] [--namespace=<name>]
<command> [<args>]

These are common Git commands used in various situations:

Start a working area (see also: git help tutorial)
    clone      Clone a repository into a new directory
    init       Create an empty Git repository or reinitialize an existing one

'git help -a' and 'git help -g' list available subcommands and some
concept guides. See 'git help <command>' or 'git help <concept>'
to read about a specific subcommand or concept.")") ;

    bool print_help = false ;
    string cmd ;
#if 0
    root.option("-h|--help", print_help).numArgs(0).description("print this help message").implicitValue("true") ;
    root.positional(cmd).numArgs(1).action( [&] {
        if ( cmd == "init" ) {
            try {
                init_group.parse(argc, argv, root.pos()) ;
                if ( print_help )
                    init_group.printUsage(std::cout) ;
            } catch ( ArgumentParserException &e ) {
                cerr << e.what() << endl ;
                init_group.printUsage(std::cerr) ;
            }
        }
        else if ( cmd == "clone" ) {
            try {
                clone_group.parse(argc, argv, root.pos()) ;
                if ( print_help )
                    clone_group.printUsage(std::cout) ;
            } catch ( ArgumentParserException &e ) {
                cerr << e.what() << endl ;
                clone_group.printUsage(std::cerr) ;
            }
        } else {
            if ( !cmd.empty() ) cerr << "Unknown command: " << cmd << endl ;
            root.printUsage(std::cerr) ;
        }

        return false ;
    } ) ;



    bool verbose = false, quiet = false, bare = false  ;
    string origin_name, repo, dir, tmpl ;

    clone_group.description("usage: git clone [<options>] [--] <repo> [<dir>]") ;
    clone_group.option("-v|--verbose", verbose).numArgs(0).description("be more verbose").implicitValue("true") ;
    clone_group.option("-o|--origin", origin_name).numArgs(1).description("use <name> instead of 'origin' to track upstream").name("<name>") ;
    clone_group.option("-h|--help", print_help).numArgs(0).description("print this help message").implicitValue("true") ;
    clone_group.positional(repo).required() ;
    clone_group.positional(dir) ;



    init_group.description("usage: git init [-q | --quiet] [--bare] [--template=<template-directory>] [--shared[=<permissions>]] [<directory>]") ;
    init_group.option("--template", tmpl).description("directory from which templates will be used").defaultValue("<template-directory>");
    init_group.option("--bare", bare).numArgs(0).description("create a bare repository") ;
    init_group.option("-h|--help", print_help).numArgs(0).description("print this help message").implicitValue("true") ;
    init_group.positional(dir) ;
#endif
    try {
        root.parse(argc, argv) ;

        if ( print_help && cmd.empty() )
                root.printUsage(std::cout) ;

    } catch ( ArgumentParserException &e ) {
        cout << e.what() << endl ;
        e.group().printUsage(std::cout) ;
    }
}

int main(int argc, const char *argv[]) {
    test_simple(argc, argv) ;
//    test_git(argc, argv) ;
}

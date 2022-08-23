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
};

namespace cvx {
template<>
struct ValueParser<BBox> {
    bool parse(const std::string &arg, BBox &bbox) {
        stringstream strm(arg) ;
        string s ;
        std::getline(strm, s, ':') ; bbox.min_x_ = std::stod(s) ;
        std::getline(strm, s, ':') ; bbox.min_y_ = std::stod(s) ;
        std::getline(strm, s, ':') ; bbox.max_x_ = std::stod(s) ;
        std::getline(strm, s, ':') ; bbox.max_y_ = std::stod(s) ;
        return (bool)strm ;
    }
};
}

static void test_simple(int argc, const char *argv[]) {

    ArgumentParser parser ;

    BBox bbox ;
    float f;
    int val ;
    std::vector<std::string> files ;
    std::string file ;
    std::vector<int> items ;
    int pos ;
    bool print_help = false ;
    ArgumentParser args ;
    args.description("test_argparse [options] output (input)+ \nList information about the FILEs (the current directory by default).\nSort entries alphabetically if none of -cftuvSUX nor --sort is specified.") ;

    args.option("-h|--help", print_help, "print this help message") ;
    args.option("-f|--flag [<v>]", f, "first flag").required().implicit("2.0") ;
    args.option("-b|--bbox <minx>:<miny>:<maxx>:<maxy>", bbox, "bounding box") ;

    args.option("-t|--test", items, "second flag") ;

    args.positional("files", files) ;

    try {
        args.parse(argc, argv) ;

        if ( print_help )
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

    root.option("-h|--help", print_help, "print this help message") ;

    root.positional("command", cmd, "Command").action( [&] {
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
    clone_group.option("-v|--verbose", verbose, "be more verbose") ;
    clone_group.option("-o|--origin <name>", origin_name,"use <name> instead of 'origin' to track upstream") ;
    clone_group.option("-h|--help", print_help, "print this help message") ;
    clone_group.positional("repo", repo, "Repository").required() ;
    clone_group.positional("dir", dir, "Directory") ;

    init_group.description("usage: git init [-q | --quiet] [--bare] [--template=<template-directory>] [--shared[=<permissions>]] [<directory>]") ;
    init_group.option("--template", tmpl, "directory from which templates will be used") ;
    init_group.option("--bare", bare, "create a bare repository") ;
    init_group.option("-h|--help", print_help, "print this help message");
    init_group.positional("dir", dir, "Directory") ;

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
//    test_simple(argc, argv) ;
    test_git(argc, argv) ;
}

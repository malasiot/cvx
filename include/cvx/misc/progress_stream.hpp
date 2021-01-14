#ifndef CVX_PROGRESS_STREAM_H
#define CVX_PROGRESS_STREAM_H

#include <string>
#include <iostream>

namespace cvx {

class ProgressStream {
public:
    ProgressStream() {}

    // start a progress indicator with given message and the total number of steps that have to be taken
    virtual void beginTask(const std::string &msg, unsigned int total_steps) {}
    // advance indicator by the given number of steps
    virtual void advance(int steps) {}

    virtual void endTask() {}
};

class ConsoleProgressPrinter:  public ProgressStream {
    public:

        ConsoleProgressPrinter(std::ostream &strm = std::cout ) ;
        ~ConsoleProgressPrinter() ;

        void beginTask(const std::string &msg, unsigned int total_steps) ;
        void advance(unsigned int steps) ;
        void endTask() ;

private:

        unsigned int total_steps_ ;
        unsigned int steps_ ;
        std::string cur_msg_ ;
        std::ostream &strm_ ;
        int last_tick_ ;
};

}


#endif

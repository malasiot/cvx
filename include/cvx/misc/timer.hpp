#ifndef CVX_TIMER_HPP
#define CVX_TIMER_HPP

#include <chrono>
#include <string>
#include <iostream>

namespace cvx {

template<class duration_t = std::chrono::milliseconds,
          class clock_t = std::chrono::high_resolution_clock>
class Timer {
    using timep_t = typename clock_t::time_point;

    bool is_running() const { return stop_time_ == timep_t::min(); }
    timep_t end_time() const { return is_running() ? clock_t::now() : stop_time_; }

    timep_t begin_time_{clock_t::now()}, stop_time_{timep_t::min()};
public:
    void stop() { if (is_running()) stop_time_ = clock_t::now(); }

    auto duration() const {
            return std::chrono::duration_cast<duration_t>(end_time() - begin_time_);
    }

    friend std::ostream& operator<<(std::ostream& os, Timer const& stopwatch)
    {
       // auto e = std::chrono::duration<double, std::milli>(stopwatch.elapsed());
        auto e = stopwatch.duration() ;
        auto oldLoc = std::cout.imbue(std::locale(""));
        std::cout << e.count() << " ms\n";
        std::cout.imbue(oldLoc);
        return os;
    }
};

class Profiler {
public:
    Profiler(const std::string &msg): msg_(msg) {}
    ~Profiler() {
        std::cout << msg_ << t_ << std::endl ;
    }
private:
    Timer<> t_ ;
    std::string msg_ ;
};

}
#endif // TIMER_HPP

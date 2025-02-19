#pragma once

#include <memory>
#include <vector>
//#include "indicators.hpp"

namespace indicators {
    class BlockProgressBar;
    template <typename Indicator> class DynamicProgress;
}

class ProgressBars {
#ifdef PROGRESSBAR_INTERNAL
    std::vector<std::unique_ptr<indicators::BlockProgressBar>> bar_storage;
    indicators::DynamicProgress<indicators::BlockProgressBar> bars;
#endif
public:
    ProgressBars();
    ~ProgressBars();
    size_t new_bar(size_t thread_num, uint32_t max_progress);
    void tick(size_t index);
    void post_work(size_t index, std::string message);
    void set_error(size_t index, std::string message);
    void set_complete(size_t index, uint32_t max_progress);
};


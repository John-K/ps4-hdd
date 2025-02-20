#pragma once

#include <memory>

class ProgressBars {
private:
    class private_stuff;
    std::unique_ptr<private_stuff> priv;
public:
    ProgressBars();
    ~ProgressBars();
    size_t new_bar(size_t thread_num, uint32_t max_progress);
    void tick(size_t index);
    void post_work(size_t index, std::string message);
    void set_error(size_t index, std::string message);
    void set_complete(size_t index, uint32_t max_progress);
};


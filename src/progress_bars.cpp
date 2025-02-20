#include <print>
#include <vector>
#include <memory.h>
#include <stdint.h>
#include "indicators.hpp"
#include "progress_bars.hpp"

class ProgressBars::private_stuff {
public:
    private_stuff() : bar_storage(), bars() {}
    std::vector<std::unique_ptr<indicators::BlockProgressBar>> bar_storage;
    indicators::DynamicProgress<indicators::BlockProgressBar> bars;
};

ProgressBars::ProgressBars() : priv(std::make_unique<private_stuff>()) {
    priv->bars.set_option(indicators::option::HideBarWhenComplete{true});
    indicators::show_console_cursor(false);
}

ProgressBars::~ProgressBars() {
    indicators::show_console_cursor(true);
}

size_t ProgressBars::new_bar(size_t thread_num, uint32_t max_progress) {
    priv->bar_storage.emplace_back(
        std::make_unique<indicators::BlockProgressBar>(
            indicators::option::BarWidth{50},
            indicators::option::ForegroundColor{indicators::Color::yellow},
            indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
            indicators::option::MaxProgress{max_progress},
            indicators::option::Start{"🔐["},
            indicators::option::End{"]📂"},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true},
            indicators::option::PrefixText{std::format("Thread {:3d} ", thread_num)}
    ));

    return priv->bars.push_back(*priv->bar_storage.back());
}

void ProgressBars::post_work(size_t index, std::string message) {
    priv->bars[index].set_option(indicators::option::ShowRemainingTime{false});
    priv->bars[index].set_option(indicators::option::PostfixText{message});
}

void ProgressBars::set_error(size_t index, std::string message) {
    priv->bars[index].set_option(indicators::option::ForegroundColor{indicators::Color::red});
    priv->bars[index].set_option(indicators::option::PostfixText{std::format("ERR: {}", message).c_str()}); 
}

void ProgressBars::tick(size_t index) {
    priv->bars[index].tick();
}

void ProgressBars::set_complete(size_t index, uint32_t max_progress) {
    // NOTE: ordering here is somewhat load-bearing.
    //       if mark_as_completed() is last, then
    //       one progress bar usually gets left behind
    priv->bars[index].mark_as_completed();
    priv->bars[index].set_option(indicators::option::PostfixText{""});
    priv->bars[index].set_option(indicators::option::ForegroundColor{indicators::Color::white});
    priv->bars[index].set_progress(max_progress);
}


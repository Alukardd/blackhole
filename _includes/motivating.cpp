#include <blackhole/blackhole.hpp>

enum class level { debug, info, warning, error };

int main(int, char**) {
    auto log = repository_t::instance().create<verbose_logger_t<level>>();

    BH_LOG(log, level::info, "[%d] %s is great!", 42, "Blackhole");
    return 0;
}
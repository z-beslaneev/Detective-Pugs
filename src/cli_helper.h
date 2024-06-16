#pragma once

#include <optional>
#include <iostream>

#include <boost/program_options.hpp>

namespace cli_helpers {

struct Arguments {
    std::uint64_t tick_period {0};
    std::string config_file_path;
    std::string www_root;
    bool randomize_spawn_dog { false };
    std::string state_file_path;
    std::uint64_t save_state_period {0};
};

std::optional<Arguments> ParseCommandLine(int argc, const char* const argv[]);

}

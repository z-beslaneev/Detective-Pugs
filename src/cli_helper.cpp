#include "cli_helper.h"

namespace cli_helpers
{

using namespace std::literals;

std::optional<Arguments> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"Allowed options"};

    Arguments args;

    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"), "set tick period")
        ("config-file,c", po::value(&args.config_file_path)->value_name("file path"), "set config file path")
        ("www-root,w", po::value(&args.www_root)->value_name("folder path"), "set static files root")
        ("randomize-spawn-points", "spawn dogs at random positions")
        ("state-file,s", po::value(&args.state_file_path)->value_name("save file path"), "set state file path")
        ("save-state-period,st", po::value(&args.save_state_period)->value_name("milliseconds"), "set state save period");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    if (vm.contains("randomize-spawn-points"s)) {
       args.randomize_spawn_dog = true;
    }

    if (!vm.contains("config-file"s)) {
       throw std::runtime_error("Config file have not been specified");
    }

    if (!vm.contains("www-root"s)) {
       throw std::runtime_error("www-root folder have not been specified");
    }

    return args;
}

}

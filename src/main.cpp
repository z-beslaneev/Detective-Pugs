#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>

#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logger_helper.h"
#include "application.h"
#include "cli_helper.h"
#include "ticker.h"
#include "infrastructure/serializing_listener.h"
#include "postgres.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace


int main(int argc, const char* argv[]) {
    try {

        auto args = cli_helpers::ParseCommandLine(argc, argv);

        if (!args) {
            // help page
            return EXIT_SUCCESS;
        }

        logger_helper::InitLogger();

        constexpr const char GAME_DB_URL[] = "GAME_DB_URL";

        std::string db_url;
        if (const auto* url = std::getenv(GAME_DB_URL)) {
            db_url = url;
        }
        else {
            throw std::runtime_error(GAME_DB_URL + " environment variable not found"s);
        }

        model::Game game = json_loader::LoadGame(args->config_file_path);
        postgres::Database db {db_url};
        application::Application app(game, db, args->randomize_spawn_dog);
        std::shared_ptr<infrastructure::SerializingListener> listener;

        if (!args->state_file_path.empty())
        {
            listener.reset(new infrastructure::SerializingListener{app, args->state_file_path});

            if (args->save_state_period > 0)
                listener->SetSavePeriod(std::chrono::milliseconds(args->save_state_period));

            app.SetUpdateListener(listener);    
        }

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);

        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    ioc.stop();
                }
            });

        auto api_strand = net::make_strand(ioc);
        auto api_handler = std::make_shared<http_handler::APIRequestHandler>(app, api_strand);

        if (args->tick_period > 0) {
            auto ticker = std::make_shared<Ticker>(api_strand, std::chrono::milliseconds(args->tick_period),
                [&app](std::chrono::milliseconds delta) { app.UpdateGameState(delta); }
            );
            ticker->Start();        
        }

        http_handler::RequestHandler handler(api_handler, args->www_root);

        http_handler::LoggingRequestHandler logging_handler (handler);

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, logging_handler);
        
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        json::object start_message;
        start_message["port"s] = port;
        start_message["address"s] = address.to_string();

        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, start_message)
                            << "server started"sv;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        if (listener) {
            listener->Save();
        }

        json::value custom_data{{"code"s, 0}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_value, custom_data)
                            << "server exited"sv;
        
    } catch ([[maybe_unused]] const std::exception& ex) {

        json::value custom_data{{"code"s, EXIT_FAILURE}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_value, custom_data)
                    << "server exited"sv << " " << ex.what();

        return EXIT_FAILURE;
    }
}

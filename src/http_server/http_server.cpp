#include "http_server.h"

#include <boost/asio/dispatch.hpp>

namespace http_server {

SessionBase::SessionBase(tcp::socket&& socket)
    : stream_(std::move(socket)) {
}   

 void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
 }

void SessionBase::Read() { 
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                        // По окончании операции будет вызван метод OnRead
                        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, std::size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        json::object read_error;
        read_error["code"s] = ec.value();
        read_error["text"s] = ec.message();
        read_error["where"s] = "read";
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, read_error)
                                << "error"sv;
        return Close();
    }
    if (ec) {
        json::object read_error;
        read_error["code"s] = ec.value();
        read_error["text"s] = ec.message();
        read_error["where"s] = "read";
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, read_error)
                                << "error"sv;
        return;
    }
    HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t bytes_written) {
    if (ec) {
        json::object write_error;
        write_error["code"s] = ec.value();
        write_error["text"s] = ec.message();
        write_error["where"s] = "read";
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, write_error)
                                << "error"sv;
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    // Считываем следующий запрос
    Read();
}

void SessionBase::Close() {
    stream_.socket().shutdown(tcp::socket::shutdown_send);
}


}  // namespace http_server

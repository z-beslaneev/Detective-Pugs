#pragma once

#include "request_handler_helper.h"
#include "model.h"
#include "logger_helper.h"
#include "api_request_handler.h"

#include <filesystem>

#include <boost/algorithm/string.hpp>

namespace http_handler {


class RequestHandler {
public:

    RequestHandler(std::shared_ptr<APIRequestHandler> api_handler, const fs::path& root_path)
        : root_path_{root_path}
        , api_request_handler_{api_handler}
        , app_(api_handler->GetApplication()) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send

        std::string target(req.target().data(), req.target().size());

        // Проверяем, относится ли звапрос к API
        // заппросы карт/карты обрабатываем тут, тк это статичные данные
        static const std::string PREFIX = "/api/v1/";
        if (target.starts_with(PREFIX))
        {
            std::string resource_type = target.substr(PREFIX.length());
            if (resource_type.starts_with("maps"))
            {
                if (req.method() != http::verb::get && req.method() != http::verb::head)
                    return send(MakeNotAlowedResponse("Invalid method"sv, "GET, HEAD"sv, req.version(), req.keep_alive()));

                auto tokens = SplitUriPath(resource_type);
                // /maps or /map/map_id
                if (tokens.size() != 2)
                    return send(GetAllMaps(req.version(), req.keep_alive()));

                return send(GetMapById(tokens[1], req.version(), req.keep_alive()));
            }

            return api_request_handler_->Handle(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        }

        std::string decoded_uri = PercentDecode(target);

        assert(!decoded_uri.empty());

        fs::path requested_path = root_path_;

        if (decoded_uri == "/")
            requested_path /= fs::path("index.html"sv);
        else
            requested_path /= fs::path(decoded_uri.substr(1));

        if (!IsSubPath(requested_path, root_path_))
        {
            StringResponse response;
            response.result(http::status::bad_request);
            response.insert(http::field::content_type, ContentType::TEXT_PLAIN);
            response.body() = "I didn't invite you into my home/"sv;
            return send(response);
        }

        if (!fs::exists(requested_path))
        {
            StringResponse response;
            response.result(http::status::not_found);
            response.insert(http::field::content_type, ContentType::TEXT_PLAIN);
            response.body() = "Sorry Mario, our princess on another server";
            return send(response);
        }

        http::file_body::value_type file;
        boost::system::error_code ec;

        file.open(requested_path.generic_string().c_str(), beast::file_mode::read, ec);

        FileResponse file_response;

        file_response.insert(http::field::content_type, ExtesionToContentType(requested_path.extension().generic_string()));
        file_response.body() = std::move(file);
        file_response.prepare_payload();

        return send(file_response);
    }

private:
    StringResponse GetAllMaps(unsigned int http_version, bool keep_alive);
    StringResponse GetMapById(std::string_view id, unsigned int http_version, bool keep_alive);
    std::string PercentDecode(const std::string& uri) const;
    std::string_view ExtesionToContentType(const std::string& extension) const;
    bool IsSubPath(fs::path path, fs::path base) const;

    fs::path root_path_;
    std::shared_ptr<APIRequestHandler> api_request_handler_;
    application::Application& app_;
};

class DurationMeasure {
public:
    DurationMeasure() = default;

    int GetDurationInMilliseconds() {
        std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_ts - start_ts_);
        return duration.count();
    }

private:
    std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
}; 

template<class SomeRequestHandler>
class LoggingRequestHandler {
public:
    LoggingRequestHandler(SomeRequestHandler& decorated)
        : decorated_(decorated) {
    }

    template<typename Request> 
    static void LogRequest(const Request& req, const std::string& ip) {

        json::object request_data;
        request_data["ip"s] = ip;
        request_data["URI"s] = std::string(req.target());
        request_data["method"s] = std::string(req.method_string());

        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, request_data)
                            << "request received"sv;
    }

    template<typename Response>
    static void LogResponse(const Response& res, const std::string& ip, const int duration) {

        json::object response_data;
        response_data["ip"s] = ip;
        response_data["response_time"s] = duration;
        response_data["code"s] = res.result_int();

        auto ct_it = res.find(http::field::content_type);
        
        response_data["content_type"s] = ct_it != res.end() ? std::string(ct_it->value()) : std::string("null");

        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, response_data)
                            << "response sent"sv;
    }
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& ip) {

        LogRequest(req, ip);
        
        auto timer = std::make_shared<DurationMeasure>();
        decorated_(std::forward<decltype(req)>(req), [send_ = std::forward<decltype(send)>(send), ip, timer](auto&& resp)
        {
            auto duration = timer->GetDurationInMilliseconds();
            LogResponse(resp, ip, duration);
            send_(resp);
        });
    }

private:
     SomeRequestHandler& decorated_;
};

}  // namespace http_handler

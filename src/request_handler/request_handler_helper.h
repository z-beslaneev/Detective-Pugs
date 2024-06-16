#pragma once

#include "http_server.h"

namespace http_handler {

using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;
namespace net = boost::asio;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

using FileResponse = http::response<http::file_body>;

StringResponse MakeStringResponse(const http::status status, const std::string_view body, unsigned int http_version, bool keep_alive);
StringResponse MakeErrorResponse(const http::status status, const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive);

StringResponse MakeBadRequest(const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive);
StringResponse MakeNotAlowedResponse(const std::string_view message, const std::string_view allow, unsigned int http_version, bool keep_alive);
StringResponse MakeNotFoundResponse(const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive);
StringResponse MakeUnauthorizedResponse(const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive);

void PrettyPrint( std::ostream& os, json::value const& jv, std::string* indent = nullptr );
std::string PrettySerialize(json::value const& jv);

std::vector<std::string> SplitUriPath(const std::string_view uri_path);

struct ContentType {
    ContentType() = delete;
    
    constexpr static std::string_view TEXT_HTML =                "text/html"sv;
    constexpr static std::string_view TEXT_CSS =                 "text/css"sv;
    constexpr static std::string_view TEXT_PLAIN =               "text/plain"sv;
    constexpr static std::string_view TEXT_JAVASCRIPT =          "text/javascript"sv;

    constexpr static std::string_view APPLICATION_JSON =         "application/json"sv;
    constexpr static std::string_view APPLICATION_XML =          "application/xml"sv;
    constexpr static std::string_view APPLICATION_OCTET_STREAM = "application/octet-stream"sv;

    constexpr static std::string_view IMAGE_PNG =                "image/png"sv;
    constexpr static std::string_view IMAGE_JPEG =               "image/jpeg"sv;
    constexpr static std::string_view IMAGE_GIF =                "image/gif"sv;
    constexpr static std::string_view IMAGE_BMP =                "image/bmp"sv;
    constexpr static std::string_view IMAGE_ICO =                "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view IMAGE_TIFF =               "image/tiff"sv;
    constexpr static std::string_view IMAGE_SVG =                "image/svg+xml"sv;

    constexpr static std::string_view AUDIO_MP3 =                "audio/mpeg"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

}
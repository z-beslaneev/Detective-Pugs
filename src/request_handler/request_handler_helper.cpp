#include "request_handler_helper.h"

namespace http_handler {

StringResponse MakeStringResponse(const http::status status, const std::string_view body, unsigned int http_version, bool keep_alive) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, ContentType::APPLICATION_JSON);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    response.set(http::field::cache_control, "no-cache");

    return response;
}

StringResponse MakeErrorResponse(const http::status status, const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive)
{
    json::object error_message;
    error_message["code"] = std::string(code);
    error_message["message"] = std::string(message);

    return MakeStringResponse(status, json::serialize(error_message), http_version, keep_alive);
}

StringResponse MakeBadRequest(const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive)
{
    return MakeErrorResponse(http::status::bad_request, code, message, http_version, keep_alive);
}

StringResponse MakeNotAlowedResponse(const std::string_view message, const std::string_view allow, unsigned int http_version, bool keep_alive)
{
    auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod"sv, message, http_version, keep_alive);
    resp.set(http::field::allow, allow);
    return resp;
}

StringResponse MakeNotFoundResponse(const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive)
{
    return MakeErrorResponse(http::status::not_found, code, message, http_version, keep_alive);
}

StringResponse MakeUnauthorizedResponse(const std::string_view code, const std::string_view message, unsigned int http_version, bool keep_alive)
{
    return MakeErrorResponse(http::status::unauthorized, code, message, http_version, keep_alive);
}

void PrettyPrint( std::ostream& os, json::value const& jv, std::string* indent) {
    std::string indent_;
    if(!indent)
        indent = &indent_;

    switch(jv.kind())
    {
    case json::kind::object:
    {
        os << "{\n";
        indent->append(4, ' ');
        auto const& obj = jv.get_object();
        if(!obj.empty())
        {
            auto it = obj.begin();
            for(;;)
            {
                os << *indent << json::serialize(it->key()) << " : ";
                PrettyPrint(os, it->value(), indent);
                if(++it == obj.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "}";
        break;
    }

    case json::kind::array:
    {
        os << "[\n";
        indent->append(4, ' ');
        auto const& arr = jv.get_array();
        if(! arr.empty())
        {
            auto it = arr.begin();
            for(;;)
            {
                os << *indent;
                PrettyPrint( os, *it, indent);
                if(++it == arr.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "]";
        break;
    }

    case json::kind::string:
    {
        os << json::serialize(jv.get_string());
        break;
    }

    case json::kind::uint64:
        os << jv.get_uint64();
        break;

    case json::kind::int64:
        os << jv.get_int64();
        break;

    case json::kind::double_:
        os << std::fixed << std::setprecision(5)
            << jv.get_double();
        break;

    case json::kind::bool_:
        if(jv.get_bool())
            os << "true";
        else
            os << "false";
        break;

    case json::kind::null:
        os << "null";
        break;
    }

    if(indent->empty())
        os << "\n";
}

std::string PrettySerialize(json::value const& jv) {
    std::stringstream ss;
    PrettyPrint(ss, jv);

    return ss.str();
}

std::vector<std::string> SplitUriPath(const std::string_view uri_path)
{
    std::vector<std::string> tokens;
    boost::split(tokens, uri_path, boost::is_any_of("/"));
    return tokens;
}
}
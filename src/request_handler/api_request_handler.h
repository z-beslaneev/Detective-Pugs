#pragma once

#include "request_handler_helper.h"
#include "application.h"

#include <functional>
#include <regex>


namespace http_handler {

inline std::unordered_map<std::string, std::string> parseParameters(const std::string& uri) {
    std::unordered_map<std::string, std::string> params;

    std::regex paramRegex("(\\w+)=(\\w+)");
    std::smatch match;

    std::string::const_iterator searchStart(uri.cbegin());
    while (std::regex_search(searchStart, uri.cend(), match, paramRegex)) {
        params[match[1].str()] = match[2].str();
        searchStart = match.suffix().first;
    }

    return params;
}    


using Strand = net::strand<net::io_context::executor_type>;
using HandlersMap = std::map<std::string_view, std::function<StringResponse(const StringRequest&)>>;

class APIRequestHandler : public std::enable_shared_from_this<APIRequestHandler> {
public:
    APIRequestHandler(application::Application& app, Strand api_strand)
        : app_(app)
        , api_strand_(api_strand) {}

    template <typename Body, typename Allocator, typename Send>
    void Handle(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

        auto handle_api = [self = shared_from_this(), send,
            req = std::forward<decltype(req)>(req)] {
                assert(self->api_strand_.running_in_this_thread());
                return send(self->HandleAPIRequest(req));
            };

        return net::dispatch(api_strand_, handle_api);
    }

    application::Application& GetApplication() { return app_; }

private:
    template <typename Body, typename Allocator>
    StringResponse HandleAPIRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {

        using namespace std::placeholders;
        static auto get_players = std::bind(&APIRequestHandler::GetPlayers, this, _1, _2);
        static auto get_state = std::bind(&APIRequestHandler::GetState, this, _1, _2);
        static auto action = std::bind(&APIRequestHandler::Action, this, _1, _2);

        // TODO REFACTOR
        if (std::string(req.target().begin(), req.target().end()) == "/api/v1/game/state")
        {
            if (req.method() != http::verb::get && req.method() != http::verb::head)
                return MakeNotAlowedResponse("Invalid method"sv, "GET, HEAD"sv, req.version(), req.keep_alive());
        }
        auto endp = std::string(req.target().begin(), req.target().end());
        if (endp.starts_with("/api/v1/game/records")) {
            auto params = parseParameters(endp);

            std::optional<int> start;
            if (params.count("start") > 0) {
                start = std::stoi(params["start"]);
            }

            std::optional<int> maxItems;
            if (params.count("maxItems") > 0) {
                maxItems = std::stoi(params["maxItems"]);
            }

            if (maxItems && *maxItems > 100)
                return MakeBadRequest("invalidArgument"sv, "Max items must len than 100"sv, req.version(), req.keep_alive());

            return GetRecords(req, start, maxItems);
        }
		
        const static HandlersMap handlers {
            {"/api/v1/game/join"sv,   [this](const auto& req) { return this->JoinToGame(req);}},
            {"/api/v1/game/players"sv,[this](const auto& req) { return this->ExecuteAuthorized(get_players, req); }},
            {"/api/v1/game/state"sv,  [this](const auto& req) { return this->ExecuteAuthorized(get_state, req); }},
            {"/api/v1/game/player/action"sv, [this](const auto& req) { return this->ExecuteAuthorized(action, req); }},
			{"/api/v1/game/tick"sv,   [this](const auto& req) { return this->Tick(req); }}
        };

        auto handler_it = handlers.find(std::string(req.target().begin(), req.target().end()));

        if (handler_it == handlers.end())
            return MakeBadRequest("badRequest"sv, "Bad request"sv, req.version(), req.keep_alive());

        return handler_it->second(req);
    }

    template <typename Fn>
    StringResponse ExecuteAuthorized(Fn&& action, const StringRequest& req) {

        std::string authHeader {req[http::field::authorization]};
        static const std::string BEARER = "Bearer ";
        const uint16_t TOKEN_LENGTH = 32;

        if (authHeader.empty() || !authHeader.starts_with(BEARER) || authHeader.substr(BEARER.length()).length() < TOKEN_LENGTH)
            return MakeUnauthorizedResponse("invalidToken"sv, "Authorization header is missing"sv, req.version(), req.keep_alive());

        auto token = authHeader.substr(BEARER.length());

        if (!app_.IsAuthorized(token))
            return MakeUnauthorizedResponse("unknownToken"sv, "Player token has not been found"sv, req.version(), req.keep_alive());

        return action(req, token);
    }

    StringResponse JoinToGame(const StringRequest& req);
    StringResponse GetPlayers(const StringRequest &req, const std::string_view token);
    StringResponse GetState(const StringRequest& req, const std::string_view token);
    StringResponse Action(const StringRequest& req, const std::string_view token);
    StringResponse Tick(const StringRequest& req);
    StringResponse GetRecords(const StringRequest& req, std::optional<int> start, std::optional<int> maxItems);

    application::Application& app_;
    Strand api_strand_;
};

}

#include "api_request_handler.h"
#include "model_properties.h"

#include <boost/json.hpp>

#include <regex>
#include <iostream>

namespace http_handler {

namespace json = boost::json;


StringResponse APIRequestHandler::JoinToGame(const StringRequest& req)
{
    if (req.method() != http::verb::post)
        return MakeNotAlowedResponse("Only POST method is expected"sv, "POST"sv, req.version(), req.keep_alive());

    std::string user_name;
    std::string map_id;

    try {
        auto body_json = boost::json::parse(req.body());
        user_name = body_json.at(Properties::JOIN_USER_NAME).as_string();
        map_id = body_json.at(Properties::JOIN_MAP_ID).as_string();

    } catch ([[maybe_unused]] const std::exception& e) {
        return MakeBadRequest("invalidArgument"sv, "Join game request parse error"sv, req.version(), req.keep_alive());
    }

    if (user_name.empty())
        return MakeBadRequest("invalidArgument"sv, "Invalid name"sv, req.version(), req.keep_alive());

    if (!app_.FindMap(map_id))
        return MakeNotFoundResponse("mapNotFound"sv, "Map not found"sv, req.version(), req.keep_alive());

    auto [authToken, playerId] = app_.JoinToGame(user_name, map_id);

    json::object response;
    response[Properties::AUTH_TOKEN] = authToken;
    response[Properties::PLAYER_ID] = playerId;

    return MakeStringResponse(http::status::ok, json::serialize(response), req.version(), req.keep_alive());
}

StringResponse APIRequestHandler::GetPlayers(const StringRequest &req, const std::string_view token)
{
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return MakeNotAlowedResponse("Invalid method"sv, "GET, HEAD"sv, req.version(), req.keep_alive());

    json::object response;

    for (auto& player : app_.GetAllPlayers()) {
        json::object player_json;

        player_json[Properties::USER_NAME] = player.GetDog()->GetName();
        std::string id_key = std::to_string(*player.GetDog()->GetDogId());

        response[id_key] = player_json;
    }

    return MakeStringResponse(http::status::ok, json::serialize(response), req.version(), req.keep_alive());
}

StringResponse APIRequestHandler::GetState(const StringRequest& req, const std::string_view token)
{
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return MakeNotAlowedResponse("Invalid method"sv, "GET, HEAD"sv, req.version(), req.keep_alive());

    json::object response;
    json::object players;

    auto game_state = app_.GetState(token);

    for (auto& palyer_state : game_state.players_state_) {

        json::object player_json;

        player_json[Properties::PLAYER_POSITION] = json::array{ palyer_state.position_x_, palyer_state.position_y_ };
        player_json[Properties::PLAYER_SPEED] = json::array{ palyer_state.horizontal_speed_, palyer_state.vertical_speed_ };

        player_json[Properties::PLAYER_DIRECTION] = palyer_state.dog_direction_;

        auto json_bag = json::array{};

        for (const auto& objects_in_bag : palyer_state.bag_) {
            json::object object_in_bag;

            object_in_bag[Properties::PLAYER_BAG_OBJ_ID] = objects_in_bag.id;
            object_in_bag[Properties::PLAYER_BAG_OBJ_TYPE] = objects_in_bag.type;

            json_bag.push_back(object_in_bag);
        }

        player_json[Properties::PLAYER_BAG] = json_bag;

        player_json[Properties::PLAYER_SCORE] = palyer_state.score_;

        players[palyer_state.dog_id_] = player_json;
    }

    response[Properties::PLAYERS_RESPONSE] = players;

    if (!game_state.loots_state_.empty()) {

        json::object lost_objects;

        for (size_t i = 0; i < game_state.loots_state_.size(); ++i) {
            json::object lost_object;

            lost_object[Properties::PLAYER_LOST_OBJECT_TYPE] = game_state.loots_state_[i].type;
            lost_object[Properties::PLAYER_LOST_OBJECT_POS] = json::array{ game_state.loots_state_[i].position.x, game_state.loots_state_[i].position.y };

            lost_objects[std::to_string(i)] = lost_object;
        }

        response[Properties::PLAYER_LOST_OBJECTS] = lost_objects;
    }

    return MakeStringResponse(http::status::ok, PrettySerialize(response), req.version(), req.keep_alive());
}

StringResponse APIRequestHandler::Action(const StringRequest& req, const std::string_view token)
{
    if (req.method() != http::verb::post)
        return MakeNotAlowedResponse("Invalid method"sv, "POST"sv, req.version(), req.keep_alive());

    std::string direction;

    auto body_json = boost::json::parse(req.body());
    direction = body_json.at(Properties::MOVE_ACTION).as_string();

    if (direction.empty()) {
        app_.Move(token, 'S');
        return MakeStringResponse(http::status::ok, json::serialize(json::object{}), req.version(), req.keep_alive());
    }

    static const std::regex MOVE_PATTERN("[LRUD]");
    if (!std::regex_match(direction, MOVE_PATTERN)) {
        return MakeBadRequest("invalidArgument"sv, "Failed to parse action"sv, req.version(), req.keep_alive());
    }

    app_.Move(token, direction[0]);

    return MakeStringResponse(http::status::ok, json::serialize(json::object{}), req.version(), req.keep_alive());
}

StringResponse APIRequestHandler::Tick(const StringRequest& req) {
    if (req.method() != http::verb::post)
        return MakeNotAlowedResponse("Invalid method"sv, "POST"sv, req.version(), req.keep_alive());

    std::int64_t time_delta = 0;

    try {
        auto body_json = boost::json::parse(req.body());
        time_delta = body_json.at(Properties::TIME_DELTA).as_int64();

        if (time_delta <= 0)
            throw std::exception {};

    } catch ([[maybe_unused]] const std::exception& e) {
        return MakeBadRequest("invalidArgument"sv, "Failed to parse tick request JSON"sv, req.version(), req.keep_alive());
    }

    app_.UpdateGameState(std::chrono::milliseconds {time_delta});

    return MakeStringResponse(http::status::ok, json::serialize(json::object{}), req.version(), req.keep_alive());
}

StringResponse APIRequestHandler::GetRecords(const StringRequest &req, std::optional<int> start, std::optional<int> maxItems) {
    
    auto records = app_.GetRecordsInfo(start, maxItems);
    auto response = json::array{};

    for (auto& [name, score, play_time] : records) {
        json::object record_json;
        record_json["name"] = name;
        record_json["score"] = score;
        record_json["playTime"] = std::round(play_time / 1000.0);

        response.push_back(record_json);
    }

    return MakeStringResponse(http::status::ok, PrettySerialize(response), req.version(), req.keep_alive());
}

}

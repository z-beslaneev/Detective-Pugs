#pragma once

#include "model.h"
#include "player.h"
#include "application_listener.h"
#include "postgres.h"

#include <chrono>

namespace application {

using milliseconds = std::chrono::milliseconds;

// auth_token, player_id
using AuthResponse = std::pair<std::string, std::uint64_t>;
using UpdateListener = std::shared_ptr<IApplicationlListener>;

struct PlayerState {
    std::string dog_id_;
    std::string dog_direction_;
    float horizontal_speed_;
    float vertical_speed_;
    float position_x_;
    float position_y_;
    uint64_t score_;
    model::LootStates bag_;
};

using PlayersState = std::vector<PlayerState>;

struct GameState {
    PlayersState players_state_;
    model::LootStates loots_state_;
};

using RecordsInfo = std::vector<std::tuple<std::string, int, double>>;


class Application {

public:

    Application(model::Game& game, postgres::Database& db, bool random_spawn);

    const model::Game::Maps& GetMaps();
    const model::Map* FindMap(std::string_view id);
    AuthResponse JoinToGame(std::string_view user_name, std::string_view map_id);
    std::vector<Player>& GetAllPlayers();
    GameState GetState(std::string_view token);
    RecordsInfo GetRecordsInfo(std::optional<int> start, std::optional<int> maxItems);
    void Move(const std::string_view token, char direction);
    void UpdateGameState(const std::chrono::milliseconds time_delta);
    bool IsAuthorized(std::string_view token);

    void SetUpdateListener(UpdateListener listener) { update_listener_ = listener; }

    model::Game& GetGame() { return game_; }
    Players& GetPlayers() { return players_; }
    void SetPlayers(Players players) { players_ = players; }

    void SetRandomSpawn(bool random_spawn) { random_spawn_ = random_spawn; }
    bool GetRandomSpawn() { return random_spawn_; }

private:

    void ProcessRetirementPlayers(const std::vector<model::Dog::Id>& ids_to_remove);    

private:
    model::Game& game_;
    Players players_;
    bool random_spawn_ = false;
    UpdateListener update_listener_;
    postgres::Database& db_;
};

}
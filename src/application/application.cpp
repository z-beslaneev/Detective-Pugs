#include "application.h"

namespace application {

Application::Application(model::Game &game, postgres::Database& db, bool random_spawn)
    : game_(game)
    , random_spawn_(random_spawn)
    , db_(db) {
}

const model::Game::Maps &Application::GetMaps() {
    return game_.GetMaps();
}

const model::Map *Application::FindMap(std::string_view id) {
    return game_.FindMap(model::Map::Id(std::string(id)));
}

AuthResponse Application::JoinToGame(std::string_view user_name, std::string_view map_id) {

    auto map_id_t = model::Map::Id(std::string(map_id));
    model::GameSession* session = game_.GetSession(map_id_t);

    auto dog = std::make_shared<model::Dog>(user_name);

    auto def_speed_global = game_.GetDefaultDogSpeed();
    auto def_speed_map = game_.FindMap(map_id_t)->GetDogSpeed();

    if (def_speed_map) {
        dog->SetDefaultSpeed(*def_speed_map);
    }
    else if (def_speed_global) {
        dog->SetDefaultSpeed(*def_speed_global);
    }

    auto def_bag_capacity = game_.GetDefaultBagCapacity();
    auto def_map_capacity = game_.FindMap(map_id_t)->GetBagCapacity();

    if (def_bag_capacity) {
        dog->SetBagCapacity(*def_bag_capacity);
    }
    else if (def_map_capacity) {
        dog->SetBagCapacity(*def_map_capacity);
    }

    std::uint64_t dog_id = session->AddDog(dog, random_spawn_);

    auto [player, token] = players_.AddPlayer(dog, session);

    return {*token, dog_id};
}

bool Application::IsAuthorized(std::string_view token) {
    return players_.IsTokenValid(Token(std::string(token)));
}

void Application::ProcessRetirementPlayers(const std::vector<model::Dog::Id>& ids_to_remove) {

    std::vector<postgres::PlayerInfo> infos;

    for (const auto& dog_id : ids_to_remove) {

        auto player = players_.FindByDogId(dog_id);
        
        if (!player)
            continue;

        auto dog = player->GetDog();

        auto name = dog->GetName();
        auto score = dog->GetScore();
        auto uptime = dog->GetUptime();

        infos.push_back({ name, score, uptime });

        players_.RemovePlayerByDogId(dog_id);
    }

    if (!infos.empty()) {
        db_.AddRecords(infos);
    }
}

void Application::Move(const std::string_view token, char direction)
{
    auto player = players_.FindByToken(Token{ std::string(token) });
    auto dog = player->GetDog();

    dog->Move(model::Direction(direction));
}

void Application::UpdateGameState(const std::chrono::milliseconds time_delta) {

    std::vector<model::Dog::Id> all_ids_to_remove;

    for (auto& session : game_.GetSessions()) {
        auto sessions_ids_to_remove = session.second.UpdateGameState(time_delta.count());

        std::transform(sessions_ids_to_remove.begin(), sessions_ids_to_remove.end(), 
            std::back_inserter(all_ids_to_remove), [](auto id) { return id; });
    }

    ProcessRetirementPlayers(all_ids_to_remove);

    if (update_listener_) {
        update_listener_->OnUpdate(time_delta);
    }
}

std::vector<Player>& Application::GetAllPlayers() {
    return players_.GetPlayers(); 
}

GameState Application::GetState(std::string_view token)
{
    GameState states;

    for (auto& player : GetAllPlayers()) {
        PlayerState state;

        auto dog = player.GetDog();
        state.dog_id_ = std::to_string(*dog->GetDogId());

        std::string dir = std::string{ dog->GetDirection() };
        state.dog_direction_ = dir == "S" ? "" : dir;

        auto pos = dog->GetPosition();
        state.position_x_ = pos.x;
        state.position_y_ = pos.y;

        auto speed = dog->GetSpeed();
        state.horizontal_speed_ = speed.horizontal;
        state.vertical_speed_ = speed.vertical;

        state.bag_ = dog->GetBag().GetObjects();
        state.score_ = dog->GetScore();

        states.players_state_.push_back(state);
    }

    auto player = players_.FindByToken(Token(std::string(token)));
    
    if (player) {
        auto loot = player->GetSession()->GetLootStates();
        states.loots_state_ = std::move(loot);
    }

    return states;
}
RecordsInfo Application::GetRecordsInfo(std::optional<int> start, std::optional<int> maxItems)
{
    auto records = db_.GetRecords(start, maxItems);
    RecordsInfo records_info;

    for (auto& record : records) {
        records_info.push_back({ record.name, record.score, record.play_time });
    }

    return records_info;
}

}
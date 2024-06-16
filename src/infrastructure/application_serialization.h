#pragma once

#include "model/model_serialization.h"
#include "application/application.h"

#include <boost/serialization/map.hpp>

#include <map>

namespace serialization {

class PlayerRepr {
public:
    PlayerRepr() = default;
    explicit PlayerRepr(const application::Player& player)
    : player_id_{ *player.GetDog()->GetDogId() }
    , game_session_id_{ *player.GetSession().GetMapId() } {  
    }

    application::Player Restore(application::Application& app) {
        model::GameSession* session = app.GetGame().GetSession(game_session_id_);
        auto dog = session->GetDog(player_id_);
        return application::Player{ session, dog };
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& player_id_;
        ar& *game_session_id_;
    }

private:
    std::uint64_t player_id_;
    model::Map::Id game_session_id_{""};
};

class PlayerTokensRepr {
public:
    PlayerTokensRepr() = default;

    explicit PlayerTokensRepr(const application::Players::PlayerIdToIndex& token_to_index) {
        for (const auto& [token, index] : token_to_index) {
            player_tokens_.emplace(*token, index);
        }
    }

    application::Players::PlayerIdToIndex Restore() {
        application::Players::PlayerIdToIndex token_to_index;
        for (const auto& [token, index] : player_tokens_) {
            token_to_index.emplace(application::Token(token), index);
        }

        return std::move(token_to_index);
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& player_tokens_;
    }

private:
    std::map<std::string, uint64_t> player_tokens_;    
};

class PlayersRepr {
public:
    PlayersRepr() = default;
    explicit PlayersRepr(const application::Players& players)
        : player_tokens_repr_(players.GetPlayerIdToIndex()) {

        for (const auto& player : players.GetPlayers()) {
            players_repr_.emplace_back(PlayerRepr{player});
        }
    }

    void Restore(application::Application& app, application::Players& appPlayers) {

        std::vector<application::Player> players;
        for (auto& player_repr : players_repr_) {
            players.emplace_back(player_repr.Restore(app));
        }

        appPlayers.SetPlayers(std::move(players));
        appPlayers.SetPlayerIdToIndex(player_tokens_repr_.Restore());
    }
    
    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_repr_;
        ar& player_tokens_repr_;
    }

private:
    std::vector<PlayerRepr> players_repr_;
    PlayerTokensRepr player_tokens_repr_;    
};
   
    
class ApplicationRepr {
public:
    ApplicationRepr() {}
    explicit ApplicationRepr(application::Application& app)
        : game_repr_(app.GetGame())
        , players_repr_(app.GetPlayers())
        , random_spawn_(app.GetRandomSpawn()) {
    }


    void Restore(application::Application& app) {
        game_repr_.Restore(app.GetGame());
        players_repr_.Restore(app, app.GetPlayers());
        app.SetRandomSpawn(random_spawn_);
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_repr_;
        ar& players_repr_;
        ar& random_spawn_;
    }

private:
    GameRepr game_repr_;
    PlayersRepr players_repr_;
    bool random_spawn_ = false;
};

}
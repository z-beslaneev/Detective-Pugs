#pragma once

#include "model.h"

#include <random>
#include <iomanip>
#include <sstream>

namespace application {

struct TokenTag {};

using Token = util::Tagged<std::string, TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class Player {
public:
    Player(model::GameSession* session, std::shared_ptr<model::Dog> dog);

    model::GameSession* GetSession();
    std::shared_ptr<model::Dog> GetDog();

    model::GameSession GetSession() const;
    std::shared_ptr<model::Dog> GetDog() const;

private:
    model::GameSession* session_;
    std::shared_ptr<model::Dog> dog_;
};

class Players {
public:
    Players() = default;

    Players& operator=(const Players& other) {
        if (this != &other) {
            players_ = other.players_;
            player_id_to_index_ = other.player_id_to_index_;
        }
        return *this;
    }    

    using PlayerIdToIndex = std::unordered_map<Token, size_t, TokenHasher>;

    std::pair<Player*, Token> AddPlayer(std::shared_ptr<model::Dog> dog, model::GameSession* session) {

        Token token = GenerateToken();

        players_.emplace_back(Player(session, dog));

        player_id_to_index_[token] = players_.size() - 1;

        return {&players_.back(), token};
    }

    bool IsTokenValid(Token token) const {
        return player_id_to_index_.find(token) != player_id_to_index_.end();
    }


    Player* FindByToken(Token token) {
        if (auto it = player_id_to_index_.find(token); it != player_id_to_index_.end()) {
            return &players_.at(it->second);
        }
        return nullptr;
    }

    std::optional<Player> FindByToken(Token token) const {
        if (auto it = player_id_to_index_.find(token); it != player_id_to_index_.end()) {
            return players_.at(it->second);
        }

        return std::nullopt;
    }

    Player* FindByDogIdAndMapId(model::Dog::Id dog_id, model::Map::Id map_id) {
        for (Player& player : players_) {
            if (player.GetDog()->GetDogId() == dog_id && player.GetSession()->GetMapId() == map_id) {
                return &player;
            }
        }
        return nullptr;
    }

    std::optional<Player> FindByDogId(const model::Dog::Id& dog_id) const {
        for (const Player& player : players_) {
            if (player.GetDog()->GetDogId() == dog_id) {
                return player;
            }
        }
        return std::nullopt;
    }

    void RemovePlayerByDogId(const model::Dog::Id& dog_id) {

        auto it = std::find_if(players_.begin(), players_.end(), [&dog_id](const Player& player) {
            return player.GetDog()->GetDogId() == dog_id;
        });

        if (it == players_.end()) {
            return;
        }

        auto indexToRemove = std::distance(players_.begin(), it);

        players_.erase(it);

        auto tokenIt = std::find_if(player_id_to_index_.begin(), player_id_to_index_.end(),
                       [indexToRemove](const auto& pair) { return pair.second == indexToRemove; });                

        if (tokenIt != player_id_to_index_.end()) {
            player_id_to_index_.erase(tokenIt);
        }
        
        for (auto& pair : player_id_to_index_) {
            if (pair.second > indexToRemove) {
                pair.second--;
            }
        }              
    }

    std::vector<Player>& GetPlayers() {
        return players_;
    }

    std::vector<Player> GetPlayers() const {
        return players_;
    }

    void SetPlayers(const std::vector<Player>& players) {
        players_ = std::move(players);
    }

    PlayerIdToIndex GetPlayerIdToIndex() const {
        return player_id_to_index_;
    }

    void SetPlayerIdToIndex(const PlayerIdToIndex& player_id_to_index) {
        player_id_to_index_ = player_id_to_index;
    }

private:

    Token GenerateToken() {
        std::stringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << generator1_();
        ss << std::setw(16) << std::setfill('0') << generator2_();
        
        return Token(ss.str());
    }

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    std::vector<Player> players_;
    PlayerIdToIndex player_id_to_index_;
};

}
#pragma once

#include <boost/serialization/vector.hpp>

#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::Speed& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.horizontal;
    ar& vec.vertical;
}    

template <typename Archive>
void serialize(Archive& ar, LootState& obj, [[maybe_unused]] const unsigned version) {
    ar& (obj.id);
    ar& (obj.type);
    ar& (obj.position);
    ar& (obj.value_);
    ar& (obj.width);
    ar& (obj.is_picked_up);
}

template <typename Archive>
void serialize(Archive& ar, model::LootGeneratorConfig& obj, [[maybe_unused]] const unsigned version) {
    ar& (obj.period_);
    ar& (obj.probability_);
    
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetDogId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , bag_capacity_(dog.GetBag().GetCapacity())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirectionEnm())
        , score_(dog.GetScore())
        , bag_content_(dog.GetBag().GetObjects()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{name_};
        dog.SetId(*id_);
        dog.SetPosition(pos_);
        dog.SetBagCapacity(bag_capacity_);
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AccumulateScore(score_);
        for (const auto& item : bag_content_) {
            if (!dog.PutToBag(item)) {
                throw std::runtime_error("Failed to put bag content");
            }
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    size_t bag_capacity_ = 0;
    model::Speed speed_;
    model::Direction direction_ = model::Direction::NORTH;
    uint64_t score_ = 0;
    model::LootStates bag_content_;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& session)
        : map_id_(session.GetMapId())
        , last_dog_id(session.GetDogIdCounter())
        , loot_states_(session.GetLootStates())
        , loot_gen_config_(session.GetLootGeneratorConfig()) {

            for (const auto& dog : session.GetDogs()) {
                dogs_repr_.emplace_back(DogRepr(*dog));
            }
        }

    model::GameSession Restore(
            const model::Map* map, 
            const model::LootGeneratorConfig loot_generator_config) {

        model::GameSession game_session(*map, loot_generator_config, 15.0);
        std::vector<std::shared_ptr<model::Dog>> dogs;

        for (const auto& dog_repr : dogs_repr_) {
            dogs.emplace_back(new model::Dog(dog_repr.Restore()));
        }

        game_session.EmplaceDogs(dogs);
        game_session.SetDogIdCounter(last_dog_id);
        game_session.SetLootStates(loot_states_);


        return game_session;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *map_id_;
        ar& last_dog_id;
        ar& loot_states_;
        ar& dogs_repr_;
        ar& loot_gen_config_;
    }

    model::Map::Id GetMapId() {
        return map_id_;
    }

private:
    model::Map::Id map_id_ {""};
    std::uint64_t last_dog_id {0};
    model::LootStates loot_states_;
    std::vector<DogRepr> dogs_repr_;
    model::LootGeneratorConfig loot_gen_config_;
};

class GameSessionsRepr {
public:
    GameSessionsRepr() = default;

    explicit GameSessionsRepr(const model::Game::GameSessions& sessions) {

        for (const auto& session : sessions) {
            game_sessions_repr_.emplace_back(GameSessionRepr(session.second));
        }
    }

    model::Game::GameSessions Restore(const model::Game& game) {

        model::Game::GameSessions game_sessions;

        for (auto& repr : game_sessions_repr_) {
            auto* map = game.FindMap(repr.GetMapId());
            game_sessions.emplace(map->GetId(), repr.Restore(map, game.GetLootGeneratorConfig()));
        }

        return game_sessions;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_sessions_repr_;
    }

private:
    std::vector<GameSessionRepr> game_sessions_repr_;

};

class GameRepr {
public:
    GameRepr() = default;

    explicit GameRepr(const model::Game& game) 
    : game_sessions_repr_(game.GetSessions()) {
    }

    void Restore(model::Game& game) {

        game.SetGameSessions(game_sessions_repr_.Restore(game));    
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_sessions_repr_;
    }

private:
    GameSessionsRepr game_sessions_repr_;    
};

}  // namespace serialization

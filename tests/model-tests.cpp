#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include "../src/model/model.h"

using namespace std::literals;

SCENARIO("Model tests") {

    GIVEN("GameSession with simple map") {

        model::Map simple_map(model::Map::Id("test_map"), "TestMap");
        simple_map.AddRoad(model::Road{model::Road::HORIZONTAL, {0, 0}, 10});


        simple_map.AddLootType(model::LootType{"key", "assets/key.obj", "obj", 90, "#338844", 0.03});
        simple_map.AddLootType(model::LootType{"wallet", "assets/wallet.obj", "obj", 90, "#338899", 0.01});

        constexpr std::chrono::milliseconds TIME_INTERVAL = 50ms;
        model::GameSession session {simple_map , model::LootGeneratorConfig{5, 1}, 15.0};

        WHEN("dont try to generate loot without looters") {
            THEN("no loot is generated") {
                session.UpdateGameState(TIME_INTERVAL.count());
                REQUIRE(session.GetLootStates().size() == 0);
            }
        }

        WHEN("add looter") {
            session.AddDog(std::make_shared<model::Dog>("test_dog"), false);
            THEN("loot is generated") {
                session.UpdateGameState(TIME_INTERVAL.count());
                REQUIRE(session.GetLootStates().size() == 1);
            }
        }

        WHEN("add few looters") {
            THEN("loot type is included in the type range") {

                auto types_max_index = simple_map.GetLootTypes().size() - 1;

                for (int i = 0; i < 10; ++i) {
                    session.AddDog(std::make_shared<model::Dog>(std::string("test_dog") + std::to_string(i)), false);
                    session.UpdateGameState(TIME_INTERVAL.count());
                    
                    for (auto& loot : session.GetLootStates()) {
                        REQUIRE(loot.type <= types_max_index);
                    }
                }
            }
        }
    }
}    
#include "json_loader.h"
#include "model_properties.h"

#include <fstream>
#include <stdexcept>

#include <boost/json.hpp>
#include <boost/system.hpp>

namespace json_loader {

namespace sys = boost::system;
namespace json = boost::json;

using namespace std::literals;

json::value LoadJsonFromFile( std::istream& is) {

    json::stream_parser stream_parser;
    std::string line;
    sys::error_code ec;

    while (std::getline( is, line ))
    {
        stream_parser.write(line, ec);

        if (ec)
            return nullptr;
    }

    stream_parser.finish(ec);

    if (ec)
        return nullptr;

    return stream_parser.release();
}

void LoadRoads(model::Map& model_map, const json::value& map_item) {

    for (const auto& roadItem : map_item.at(Properties::ROADS_ARRAY).as_array())
    {
        auto x0 = static_cast<int>(roadItem.at(Properties::ROAD_POINT_X).as_int64());
        auto y0 = static_cast<int>(roadItem.at(Properties::ROAD_POINT_Y).as_int64());
        
        if (auto x1p = roadItem.as_object().if_contains(Properties::ROAD_HORIZONTAL))
        {
            auto x1 = static_cast<int>(x1p->as_int64());
            model_map.AddRoad(model::Road{model::Road::HORIZONTAL, {x0, y0}, x1});
        }
        else
        {
            auto y1 = static_cast<int>(roadItem.at(Properties::ROAD_VERTICAL).as_int64());
            model_map.AddRoad(model::Road{model::Road::VERTICAL, {x0, y0}, y1});
        }
    }
}

void LoadBuildings(model::Map& model_map, const json::value& map_item) {

    for (const auto& build_item : map_item.at(Properties::BUILDINGS_ARRAY).as_array())
    {
        auto x = static_cast<int>(build_item.at(Properties::BUILDING_POINT_X).as_int64());
        auto y = static_cast<int>(build_item.at(Properties::BUILDING_POINT_Y).as_int64());
        auto w = static_cast<int>(build_item.at(Properties::BUILDING_WEIGHT).as_int64());
        auto h = static_cast<int>(build_item.at(Properties::BUILDING_HEIGHT).as_int64());

        model_map.AddBuilding(model::Building({{x, y}, {w, h}}));
    }
}

void LoadOffices(model::Map& model_map, const json::value& map_item) {

    for (const auto& office_item : map_item.at(Properties::OFFICES_ARRAY).as_array())
    {
        std::string id = office_item.at(Properties::OFFICE_ID).as_string().c_str();
        auto x = static_cast<int>(office_item.at(Properties::OFFICE_POINT_X).as_int64());
        auto y = static_cast<int>(office_item.at(Properties::OFFICE_POINT_Y).as_int64());
        auto offX = static_cast<int>(office_item.at(Properties::OFFICE_OFFSET_X).as_int64());
        auto offY = static_cast<int>(office_item.at(Properties::OFFICE_OFFSET_Y).as_int64());

        model_map.AddOffice(model::Office(model::Office::Id(id),{x, y}, {offX, offY}));
    }
}

void LoadLootTypes(model::Map& model_map, const json::value& map_item) {

    for (const auto& loot_item : map_item.at(Properties::LOOT_TYPES_ARRAY).as_array())
    {
        auto name = loot_item.at(Properties::LOOT_NAME).as_string().c_str();
        auto file = loot_item.at(Properties::LOOT_FILE).as_string().c_str();
        auto type = loot_item.at(Properties::LOOT_TYPE).as_string().c_str();

        std::optional<int> rotation;
        if (loot_item.as_object().contains(Properties::LOOT_ROTATION))
            rotation = static_cast<int>(loot_item.at(Properties::LOOT_ROTATION).as_int64());

        std::optional<std::string> color; 
        if (loot_item.as_object().contains(Properties::LOOT_COLOR)) 
            color = loot_item.at(Properties::LOOT_COLOR).as_string().c_str();

        auto scale = static_cast<float>(loot_item.at(Properties::LOOT_SCALE).as_double());
        auto value = loot_item.at(Properties::LOOT_VALUE).as_int64();
        

        model_map.AddLootType({name, file, type, rotation, color, scale, value});
    }
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    std::ifstream file(json_path);
    if (!file)
        throw std::invalid_argument{ json_path.string()};

    json::value json = LoadJsonFromFile(file);

    if (json.is_null())
        throw std::runtime_error("Can't parse config file");

    // Загрузить модель игры из файла
    model::Game game;

    if (json.as_object().contains(Properties::DEFAULT_DOG_SPEED)) {

        game.SetDefaultDogSpeed(static_cast<float>(json.at(Properties::DEFAULT_DOG_SPEED).as_double()));
    }

    if (json.as_object().contains(Properties::LOOT_GENERATOR_CONFIG)) {

        auto config = json.at(Properties::LOOT_GENERATOR_CONFIG).as_object();
        auto period = static_cast<float>(config.at(Properties::LOOT_GENERATOR_PERIOD).as_double());
        auto probability = static_cast<float>(config.at(Properties::LOOT_GENERATOR_PROBABILITY).as_double());

        game.SetLootGeneratorConfig({period, probability});
    }

    if (json.as_object().contains(Properties::DEFAULT_BAG_CAPACITY)) {

        game.SetDefaultBagCapacity(static_cast<int>(json.at(Properties::DEFAULT_BAG_CAPACITY).as_int64()));
    }

    if (json.as_object().contains(Properties::DOG_RETIREMENT_TIME)) {

        constexpr double ONE_SECOND = 1000.0;
        game.SetDogRetirementTime(static_cast<float>(json.at(Properties::DOG_RETIREMENT_TIME).as_double() * ONE_SECOND));
    }

    for (const auto& map_item : json.at(Properties::MAPS_ARRAY).as_array())
    {
        std::string id = map_item.at(Properties::MAP_ID).as_string().c_str();
        std::string name = map_item.at(Properties::MAP_NAME).as_string().c_str();

        model::Map model_map(model::Map::Id(id), name);

        if (map_item.as_object().contains(Properties::DOG_SPEED))
        {
            model_map.AddDogSpeed(static_cast<float>(map_item.at(Properties::DOG_SPEED).as_double()));
        }

        if (json.as_object().contains(Properties::BAG_CAPACITY))
        {
            model_map.SetBagCapacity(static_cast<int>(json.at(Properties::BAG_CAPACITY).as_int64()));
        }

        LoadRoads(model_map, map_item);
        LoadBuildings(model_map, map_item);
        LoadOffices(model_map, map_item);
        LoadLootTypes(model_map, map_item);

        game.AddMap(model_map);
    }

    return game;
}

}  // namespace json_loader

#pragma once

struct Properties{
    Properties() = delete;

    constexpr static char MAPS_ARRAY[] = "maps";

    constexpr static char MAP_ID[] = "id";
    constexpr static char MAP_NAME[] = "name";

    constexpr static char ROADS_ARRAY[] = "roads";
    constexpr static char ROAD_POINT_X[] = "x0";
    constexpr static char ROAD_POINT_Y[] = "y0";
    constexpr static char ROAD_HORIZONTAL[] = "x1";
    constexpr static char ROAD_VERTICAL[] = "y1";

    constexpr static char BUILDINGS_ARRAY[] = "buildings";
    constexpr static char BUILDING_POINT_X[] = "x";
    constexpr static char BUILDING_POINT_Y[] = "y";
    constexpr static char BUILDING_WEIGHT[] = "w";
    constexpr static char BUILDING_HEIGHT[] = "h";

    constexpr static char OFFICES_ARRAY[] = "offices";
    constexpr static char OFFICE_ID[] = "id";
    constexpr static char OFFICE_POINT_X[] = "x";
    constexpr static char OFFICE_POINT_Y[] = "y";
    constexpr static char OFFICE_OFFSET_X[] = "offsetX";
    constexpr static char OFFICE_OFFSET_Y[] = "offsetY";

    constexpr static char DEFAULT_DOG_SPEED[] = "defaultDogSpeed";
    constexpr static char DOG_SPEED[] = "dogSpeed";

    constexpr static char JOIN_USER_NAME[] = "userName";
    constexpr static char JOIN_MAP_ID[] = "mapId";

    constexpr static char AUTH_TOKEN[] = "authToken";
    constexpr static char PLAYER_ID[] = "playerId";
    constexpr static char USER_NAME[] = "name";

    constexpr static char PLAYERS_RESPONSE[] = "players";
    constexpr static char PLAYER_POSITION[] = "pos";
    constexpr static char PLAYER_SPEED[] = "speed";
    constexpr static char PLAYER_DIRECTION[] = "dir";
    constexpr static char PLAYER_BAG[] = "bag";
    constexpr static char PLAYER_BAG_OBJ_ID[] = "id";
    constexpr static char PLAYER_BAG_OBJ_TYPE[] = "type";
    constexpr static char PLAYER_SCORE[] = "score";
    constexpr static char PLAYER_LOST_OBJECTS[] = "lostObjects";
    constexpr static char PLAYER_LOST_OBJECT_TYPE[] = "type";
    constexpr static char PLAYER_LOST_OBJECT_POS[] = "pos";

    constexpr static char MOVE_ACTION[] = "move";

    constexpr static char TIME_DELTA[] = "timeDelta";

    constexpr static char LOOT_GENERATOR_CONFIG[] = "lootGeneratorConfig";
    constexpr static char LOOT_GENERATOR_PERIOD[] = "period";
    constexpr static char LOOT_GENERATOR_PROBABILITY[] = "probability";

    constexpr static char LOOT_TYPES_ARRAY[] = "lootTypes";
    constexpr static char LOOT_NAME[] = "name";
    constexpr static char LOOT_FILE[] = "file";
    constexpr static char LOOT_TYPE[] = "type";
    constexpr static char LOOT_ROTATION[] = "rotation";
    constexpr static char LOOT_COLOR[] = "color";
    constexpr static char LOOT_SCALE[] = "scale";
    constexpr static char LOOT_VALUE[] = "value";

    constexpr static char DEFAULT_BAG_CAPACITY[] = "defaultBagCapacity";
    constexpr static char BAG_CAPACITY[] = "defaultBagCapacity";

    constexpr static char DOG_RETIREMENT_TIME[] = "dogRetirementTime";

};
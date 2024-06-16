#include "request_handler.h"
#include "model_properties.h"

namespace http_handler {

StringResponse RequestHandler::GetAllMaps(unsigned int http_version, bool keep_alive)
{
    json::array response;

    for (const auto& map : app_.GetMaps())
    {
        json::object map_item;
        map_item[Properties::MAP_ID] = *map.GetId();
        map_item[Properties::MAP_NAME] = map.GetName();

        response.push_back(map_item);
    }
    
    return MakeStringResponse(http::status::ok, PrettySerialize(response), http_version, keep_alive);
}

void FillRoads(const model::Map& model_map, json::object& map_json) {
    json::array roads_json_arr;
    for (auto& road : model_map.GetRoads())
    {
        json::object road_json;

        road_json[Properties::ROAD_POINT_X] = road.GetStart().x;
        road_json[Properties::ROAD_POINT_Y] = road.GetStart().y;

        std::string direction_key = road.IsHorizontal() ? Properties::ROAD_HORIZONTAL : Properties::ROAD_VERTICAL;

        road_json[direction_key] = road.IsHorizontal() ? road.GetEnd().x : road.GetEnd().y;

        roads_json_arr.push_back(road_json);
    }

    map_json[Properties::ROADS_ARRAY] = roads_json_arr;
}

void FillBuildings(const model::Map& model_map, json::object& map_json) {
    json::array buildings_json_arr;
    for (auto& building : model_map.GetBuildings())
    {
        json::object building_json;

        building_json[Properties::BUILDING_POINT_X] = building.GetBounds().position.x;
        building_json[Properties::BUILDING_POINT_Y] = building.GetBounds().position.y;
        building_json[Properties::BUILDING_WEIGHT] = building.GetBounds().size.width;
        building_json[Properties::BUILDING_HEIGHT] = building.GetBounds().size.height;

        buildings_json_arr.push_back(building_json);
    }
    map_json[Properties::BUILDINGS_ARRAY] = buildings_json_arr;
}

void FillOffices(const model::Map& model_map, json::object& map_json) {
    json::array offices_json_arr;
    for (auto& office : model_map.GetOffices())
    {
        json::object office_json;

        office_json[Properties::OFFICE_ID] = *office.GetId();
        office_json[Properties::OFFICE_POINT_X] = office.GetPosition().x;
        office_json[Properties::OFFICE_POINT_Y] = office.GetPosition().y;
        office_json[Properties::OFFICE_OFFSET_X] = office.GetOffset().dx;
        office_json[Properties::OFFICE_OFFSET_Y] = office.GetOffset().dy;

        offices_json_arr.push_back(office_json);
    }
    map_json[Properties::OFFICES_ARRAY] = offices_json_arr;
}

void FillLootTypes(const model::Map& model_map, json::object& map_json) {
    json::array lootTypes_json_arr;
    for (auto& lootType : model_map.GetLootTypes())
    {
        json::object lootType_json;
        lootType_json[Properties::LOOT_NAME] = lootType.name_;
        lootType_json[Properties::LOOT_FILE] = lootType.file_;
        lootType_json[Properties::LOOT_TYPE] = lootType.type_;
        if (lootType.rotation_)
            lootType_json[Properties::LOOT_ROTATION] = *lootType.rotation_;

        if (lootType.color_)
            lootType_json[Properties::LOOT_COLOR] = *lootType.color_;

        lootType_json[Properties::LOOT_SCALE] = lootType.scale_;
        lootType_json[Properties::LOOT_VALUE] = lootType.value_;

        lootTypes_json_arr.push_back(lootType_json);
    }

    map_json[Properties::LOOT_TYPES_ARRAY] = lootTypes_json_arr;
}

StringResponse RequestHandler::GetMapById(std::string_view id, unsigned int http_version, bool keep_alive) {
    
    const auto* model_map_p = app_.FindMap(id);

    if (!model_map_p)
        return MakeNotFoundResponse("mapNotFound"sv, "Map not found"sv, http_version, keep_alive);

    json::object map_json;

    map_json[Properties::MAP_ID] = *model_map_p->GetId();
    map_json[Properties::MAP_NAME] = model_map_p->GetName();

    FillRoads(*model_map_p, map_json);
    FillBuildings(*model_map_p, map_json);
    FillOffices(*model_map_p, map_json);
    FillLootTypes(*model_map_p, map_json);

    return MakeStringResponse(http::status::ok, PrettySerialize(map_json), http_version, keep_alive);
}    

std::string RequestHandler::PercentDecode(const std::string& uri) const {
    
    std::stringstream ss;

    for (auto cur_it = uri.begin(); cur_it != uri.end(); ++cur_it)
    {
        if (*cur_it != '%')
        {
            ss << *cur_it;
            continue;
        }

		// Колчество символов после '%' должно быть не меньше 2
		static constexpr uint8_t HEX_LEN = 2;
		if  (std::distance(cur_it, uri.end()) < HEX_LEN)
			return "";
		
		try {
			std::string hex(1, *(++cur_it));
			hex += *(++cur_it);

			int decoded_char = std::stoi(hex, nullptr, 16);
			
			// Проверяем на вхождение в ASCII диапозон
			if (decoded_char < 0 && decoded_char > 255)
				return {};
			
			ss << static_cast<char>(decoded_char);
			
		} catch ([[maybe_unused]] const std::invalid_argument& e) {
			return {};
		}
    }

    return ss.str();
}

std::string_view RequestHandler::ExtesionToContentType(const std::string& extension) const {

    static const std::map<std::string_view, std::string_view> extensionMap {
        { ".html", ContentType::TEXT_HTML },
        { ".htm", ContentType::TEXT_HTML },
        { ".css", ContentType::TEXT_CSS },
        { ".txt", ContentType::TEXT_PLAIN },
        { ".js", ContentType::TEXT_JAVASCRIPT },
        { ".json", ContentType::APPLICATION_JSON },
        { ".xml", ContentType::APPLICATION_XML },
        { ".png", ContentType::IMAGE_PNG },
        { ".jpeg", ContentType::IMAGE_JPEG },
        { ".jpg", ContentType::IMAGE_JPEG },
        { ".jpe", ContentType::IMAGE_JPEG },
        { ".bmp", ContentType::IMAGE_BMP },
        { ".gif", ContentType::IMAGE_GIF },
        { ".ico", ContentType::IMAGE_ICO },
        { ".tiff", ContentType::IMAGE_TIFF },
        { ".tif", ContentType::IMAGE_TIFF },
        { ".svg", ContentType::IMAGE_SVG },
        { ".svgz", ContentType::IMAGE_SVG },
        { ".mp3", ContentType::AUDIO_MP3 }
    };

    auto it = extensionMap.find(extension);
    if (it != extensionMap.end()) {
        return it->second;
    }
    
    return ContentType::APPLICATION_OCTET_STREAM;
}

bool RequestHandler::IsSubPath(fs::path path, fs::path base) const {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}


}  // namespace http_handler

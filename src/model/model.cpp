#include "model.h"

#include <stdexcept>
#include <cassert>


namespace model {
using namespace std::literals;

static float GATHER_WIDTH = 0.6f;
static float OFFICE_WIDTH = 0.5f;

model::Road::Road(HorizontalTag, Point start, Coord end_x, float width) noexcept
    : start_{start}
    , end_{end_x, start.y}
    , width_{width} {
        
    }

model::Road::Road(VerticalTag, Point start, Coord end_y, float width) noexcept
    : start_{start}
    , end_{start.x, end_y}
    , width_{width} {
        
    }

bool Road::IsOnTheRoad(Point point) const noexcept {
    return point.x >= std::min(start_.x, end_.x) - width_ &&
        point.x <= std::max(start_.x, end_.x) + width_ &&
        point.y >= std::min(start_.y, end_.y) - width_ &&
        point.y <= std::max(start_.y, end_.y) + width_;    
}

Point Road::BoundToTheRoad(Point point) const noexcept {
    double min_x = std::min(start_.x, end_.x) - width_;
    double max_x = std::max(start_.x, end_.x) + width_;

    double new_x = std::clamp(point.x, min_x, max_x); 

    double min_y = std::min(start_.y, end_.y) - width_;
    double max_y = std::max(start_.y, end_.y) + width_;

    double new_y = std::clamp(point.y, min_y, max_y);         

    return Point(new_x, new_y);
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::operator==(const Road& other) const {
    return start_.x == other.start_.x && end_.x == other.end_.x &&
            start_.y == other.start_.y && end_.y == other.end_.y;

}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

Dog::Dog(std::string_view name) noexcept
    : name_(name) {        
    }

void Dog::SetId(std::uint64_t dog_id) {
    *id_ = dog_id;
}

void Dog::SetPosition(const Point& point) {
    point_ = point;
}

void Dog::Move(Direction dir) {

    if (dir != Direction::STOP) {
        direction_ = dir;
        ResetRestTime();
    }

    switch (dir) {
        case Direction::NORTH :
            speed_ = {0.f, -default_speed_};
            break;
        case Direction::SOUTH :
            speed_ = {0.f, default_speed_};
            break;
        case Direction::EAST :
            speed_ = {default_speed_, 0.f};
            break;
        case Direction::WEST :
            speed_ = {-default_speed_, 0.f};
            break;
        case Direction::STOP :
            speed_ = {0.f, 0.f};
            break;
        default :
            assert(false);
    }    
}

void Dog::SetSpeed(const Speed& speed) {
    speed_ = speed;
}

void Dog::SetDefaultSpeed(float speed) {
    default_speed_ = speed;
}

void Dog::SetBagCapacity(unsigned int capacity)
{
    bag_capacity_ = capacity;
    bag_ = LostObjectsBag(bag_capacity_);
}

Point Dog::GetPosition() const {
    return point_;
}

Speed Dog::GetSpeed() const {
    return speed_;
}

void Dog::SetDirection(Direction dir) {
    direction_ = dir;
}

bool Dog::PutToBag(const LootState& object) {
    return bag_.Add(object);
}

char Dog::GetDirection() const
{
    return static_cast<char>(direction_);
}

Direction Dog::GetDirectionEnm() const {
    return direction_;
}

Dog::Id Dog::GetDogId() const {
    return id_;
}

std::string Dog::GetName() const {
    return name_;
}

LostObjectsBag &Dog::GetBag() {
    return bag_;
}

LostObjectsBag Dog::GetBag() const {
    return bag_;
}

Point Dog::CalculateNextPosition(std::uint64_t time_delta) const {
    constexpr float ONE_SECOND = 1000.f;
    auto x = point_.x + speed_.horizontal * (time_delta / ONE_SECOND);
    auto y = point_.y + speed_.vertical * (time_delta / ONE_SECOND);

    return { x, y };
}

void Dog::AccumulateScore(uint64_t score)
{
    score_ += score;
}

uint64_t Dog::GetScore() const
{
    return score_;
}

RoadLoader::RoadLoader(const std::vector<Road>& roads) {
    for (const auto& road : roads) {
        if (road.IsHorizontal()) {
            horizontal_roads_.emplace_back(road);
        }
        else {
            vertical_roads_.emplace_back(road);
        }
    }

    HandleTheRoads();
}

std::vector<Road> RoadLoader::GetDicts() const {
    std::vector<Road> src = vertical_roads_;
    src.insert(src.end(), horizontal_roads_.begin(), horizontal_roads_.end());
    src.insert(src.end(), new_roads_.begin(), new_roads_.end());

    return src;
}

RoadLoader::RoadPairs RoadLoader::CheckRoads(const std::vector<Road>& roads) {
    RoadPairs for_merging;

    for (size_t i = 0; i < roads.size(); ++i) {
        Road road1 = roads[i];
        for (size_t j = i + 1; j < roads.size(); ++j) {
            Road road2 = roads[j];

            if (road1.IsVertical()) {
                if (road1.GetStart().x == road2.GetStart().x) {
                    if (road1.GetStart().y == road2.GetEnd().y || road1.GetEnd().y == road2.GetStart().y) {
                        for_merging.push_back({ road1, road2 });
                    }
                }
            } else {
                if (road1.GetStart().y == road2.GetStart().y) {
                    if (road1.GetStart().x == road2.GetEnd().x || road1.GetEnd().x == road2.GetStart().x) {
                        for_merging.push_back({ road1, road2 });
                    }
                }
            }
        }
    }

    return for_merging;
}

Road RoadLoader::MergeRoads(const Road& road1, const Road& road2) {
    Road result = road1;

    result.start_.x = std::min(road1.GetStart().x, road2.GetStart().x);
    result.end_.x = std::max(road1.GetEnd().x, road2.GetEnd().x);
    result.start_.y = std::min(road1.GetStart().y, road2.GetStart().y);
    result.end_.y = std::max(road1.GetEnd().y, road2.GetEnd().y);

    return result;
}

void RoadLoader::HandleTheRoads() {
    RoadPairs verticalPairs = CheckRoads(vertical_roads_);
    RoadPairs horizontalPairs = CheckRoads(horizontal_roads_);
    
    for ( auto& pair : verticalPairs) {
        new_roads_.push_back(MergeRoads(pair[0], pair[1]));
        vertical_roads_.erase(std::find(vertical_roads_.begin(), vertical_roads_.end(), pair[0]));
        vertical_roads_.erase(std::find(vertical_roads_.begin(), vertical_roads_.end(), pair[1]));
    }
    for ( auto& pair : horizontalPairs) {
        new_roads_.push_back(MergeRoads(pair[0], pair[1]));
        horizontal_roads_.erase(std::find(horizontal_roads_.begin(), horizontal_roads_.end(), pair[0]));
        horizontal_roads_.erase(std::find(horizontal_roads_.begin(), horizontal_roads_.end(), pair[1]));
    }
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

GameSession::GameSession(const Map& map, const LootGeneratorConfig& config, double dog_retirement_time) 
    : map_(map) 
    , loot_generator_(std::chrono::milliseconds{static_cast<uint64_t>(config.period_)}, config.probability_) 
    , dog_retirement_time_(dog_retirement_time)
    , roads_ (RoadLoader(map.GetRoads()).GetDicts()) {
    }

std::uint64_t GameSession::AddDog(std::shared_ptr<model::Dog> dog, bool random_spawn) {
    
    auto id = dog_id_counter_;
    
    dog->SetId(dog_id_counter_++);
    if (random_spawn) 
        dog->SetPosition(GenerateRandomPosition());
    else {
        const auto& road = map_.GetRoads().front();
        dog->SetPosition({road.GetStart().x, road.GetStart().y});
    }

    dogs_.emplace_back(dog);
    return id;
}

Map::Id GameSession::GetMapId() const {
    return map_.GetId();
}

std::vector<std::shared_ptr<model::Dog>> GameSession::GetDogs() const
{
    return dogs_;
}

std::optional<Point> GameSession::TryMoveOnMap(const Point& from, const Point& to) const {

    std::vector<Road> start_roads;

    std::copy_if(roads_.begin(), roads_.end(), std::back_inserter(start_roads), 
        [&](const Road& road) {
            return road.IsOnTheRoad(from);
        });

    if (start_roads.empty()) {
        return std::nullopt;
    }

    auto most_far = start_roads[0].BoundToTheRoad(to);

    if (start_roads.size() == 1) {
        return most_far;
    }

    auto max_distance = Point::Distance(from, most_far);

    for (std::size_t index = 1; index < start_roads.size(); ++index) {
        auto pretender = start_roads[index].BoundToTheRoad(to);
        auto distance = Point::Distance(from, pretender);

        if (distance > max_distance) {
            most_far = pretender;
            max_distance = distance;
        }
    }

    return most_far;
}

LootStates GameSession::GetLootStates() const
{
    return loot_states_;
}

void GameSession::GenerateLootOnMap(unsigned loot_to_gen)
{
    auto loot_types_size = map_.GetLootTypes().size();
    auto& loots = map_.GetLootTypes();
    
    for (size_t loot = 0; loot < loot_to_gen; ++loot) {
        loot_states_.emplace_back(loot, GetRandomSizeT(loot_types_size), GenerateRandomPosition(), loots[loot].value_);
    }
}

std::vector<Dog::Id> GameSession::UpdateGameState(const std::int64_t time_delta) {

    ItemDogProvider::Gatherers gatherers;

    std::vector<Dog::Id> ids_to_remove;

    for (auto& dog : dogs_) {      

        auto previuos_position = dog->GetPosition();
        auto next_position = dog->CalculateNextPosition(time_delta);

        // bounding
        auto new_position = TryMoveOnMap(dog->GetPosition(), next_position);

        if (new_position) {
            dog->SetPosition(*new_position);
        }

        if (*new_position != next_position) {
            dog->Move(Direction::STOP);
        }

        if (dog->GetSpeed() == model::Speed{ 0.f, 0.f }) {
            dog->IncrementRestTime(time_delta);

            if (dog->GetRestTime() >= dog_retirement_time_) {
                ids_to_remove.push_back(dog->GetDogId());
                continue;
            }
        }
        else {
            dog->ResetRestTime();
        }

        dog->IncrementUpTime(time_delta);

        gatherers.emplace_back(
            previuos_position,
            dog->GetPosition(),
            GATHER_WIDTH);
    }

    std::erase_if(dogs_, [this](const auto& dog) {
        return dog->GetRestTime() >= dog_retirement_time_;
    });

    auto loot_to_gen = loot_generator_.Generate(std::chrono::milliseconds{time_delta}, loot_states_.size(), dogs_.size());
    GenerateLootOnMap(loot_to_gen);

    const auto& offices = map_.GetOffices();
    ItemDogProvider::Items items;

    items.reserve(loot_states_.size() + offices.size());

    std::transform(loot_states_.begin(), loot_states_.end(), std::back_inserter(items),
                   [](const LootState& state) {
                       return collision_detector::Item{state.position, state.width};
                   });

    size_t offices_start_idx = items.size();
    for (const auto& office : offices) {
        items.emplace_back(office.GetPosition(), OFFICE_WIDTH);
    }

    auto events = collision_detector::FindGatherEvents(
        ItemDogProvider(std::move(items), std::move(gatherers))
    );

    for (const auto& event : events) {

        auto& dog = dogs_[event.gatherer_id];
        auto& bag = dog->GetBag();

        if (event.item_id < offices_start_idx) {

            auto& obj = loot_states_[event.item_id];
            if (bag.IsFull() || obj.is_picked_up) {
                continue;
            }

            obj.is_picked_up = true;
            bag.Add(obj);

        } else {
            if (bag.IsEmpty()) {
               continue;
            }

            dog->AccumulateScore(bag.Drop());
        }
    }

    std::erase_if(loot_states_, [](const auto& obj) {
        return obj.is_picked_up;
    });

    return ids_to_remove;
}

float GameSession::GenerateRandomFloat(float min, float max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distribution(min, max);
    
    return distribution(gen);
}

Point GameSession::GenerateRandomPosition() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> distribution(0, map_.GetRoads().size() - 1);

    auto random_index = distribution(gen);

    auto roads = map_.GetRoads();
    auto road = roads[random_index];

    Point start = road.GetStart();
    Point end = road.GetEnd();

    if (road.IsHorizontal())
    {
        if (start.x > end.x)
            std::swap(start, end);

        return { GenerateRandomFloat(start.x, end.x), static_cast<float>(start.y) };
    }
    else
    {
        if (start.y > end.y)
            std::swap(start, end);

        return { static_cast<float>(start.x), GenerateRandomFloat(start.y, end.y)};
    }
}

size_t GameSession::GetRandomSizeT(size_t n)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> distrib(0, n - 1);
    return distrib(gen);
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

}  // namespace model

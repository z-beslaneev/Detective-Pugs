#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <random>

#include "tagged.h"
#include "loot_generator.h"
#include "collision_detector.h"
#include "geom.h"

namespace model {

using Dimension = float;
using Coord = Dimension;

using Point = geom::Point2D;

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x, float width = 0.4) noexcept;
    Road(VerticalTag, Point start, Coord end_y, float width = 0.4) noexcept;

    bool IsOnTheRoad(Point point) const noexcept;
    Point BoundToTheRoad(Point point) const noexcept;

    bool operator==(const Road& other) const;     
    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;

    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;

    Point start_;
    Point end_;
    float width_;
};

class Building {
public:
    explicit Building(const Rectangle& bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

struct LootType {
    std::string name_;
    std::string file_;
    std::string type_;
    std::optional<int> rotation_;
    std::optional<std::string> color_;
    float scale_;
    uint64_t value_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using LootTypes = std::vector<LootType>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    const LootTypes& GetLootTypes() const noexcept {
        return loot_types_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void AddLootType(const LootType loot) {
        loot_types_.emplace_back(loot);
    }

    void AddDogSpeed(float speed) {
        dog_speed_ = speed;
    }

    std::optional<float> GetDogSpeed() const {
        return dog_speed_;
    }

    void SetBagCapacity(int capacity) {
        bag_capacity_ = capacity;
    }

    std::optional<int> GetBagCapacity() const {
        return bag_capacity_;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    LootTypes loot_types_;

    std::optional<float> dog_speed_;
    std::optional<int> bag_capacity_;
};


struct Speed {
    float horizontal, vertical;

    auto operator<=>(const Speed&) const = default;

    bool operator==(const Speed& other) const {
        constexpr float epsilon = 0.1f;

        return std::abs(horizontal - other.horizontal) < epsilon &&
               std::abs(vertical - other.vertical) < epsilon;
    }    
};

enum class Direction : char {
    NORTH = 'U',
    SOUTH = 'D',
    WEST  = 'L',
    EAST  = 'R',
    STOP = 'S'
};

struct LootState {
    int id;
    size_t type;
    Point position;
    uint64_t value_ = 0;
    float width = 0.f;
    bool is_picked_up = false;

    auto operator<=>(const LootState&) const = default;
};

using LootStates = std::vector<LootState>;

class LostObjectsBag {
  public:

    explicit LostObjectsBag(size_t capacity) : capacity_(capacity) {
        lost_objects_.reserve(capacity_);
    }

    bool IsFull() const {
        return lost_objects_.size() == capacity_;
    }

    bool IsEmpty() const {
        return lost_objects_.empty();
    }

    bool Add(LootState lost_object) {
        if (IsFull()) {
            return false;
        }

        lost_objects_.push_back(std::move(lost_object));
        return true;
    }

    size_t Drop() {

        size_t score = std::accumulate(lost_objects_.begin(), lost_objects_.end(), 0,
                           [](int sum, const auto& obj) {
                               return sum + obj.value_;
                           });

        lost_objects_.clear();
        return score;
    }
    
    LootStates GetObjects() const {
        return lost_objects_;
    }

    size_t GetCapacity() {
        return capacity_;
    }

  private:
    LootStates lost_objects_;
    size_t capacity_;
};

class Dog {
public:
    using Id = util::Tagged<std::uint64_t, Dog>;

    explicit Dog(std::string_view name) noexcept;

    void SetId(std::uint64_t dog_id);
    void SetPosition(const Point& point);
    void Move(Direction dir);
    void SetSpeed(const Speed& speed);
    void SetDefaultSpeed(float speed);
    void SetBagCapacity(unsigned int capacity);
    
    Point GetPosition() const;
    Speed GetSpeed() const;

    void SetDirection(Direction dir);

    bool PutToBag(const LootState &object);

    char GetDirection() const;
    Direction GetDirectionEnm() const;
    Id GetDogId() const;
    std::string GetName() const;
    LostObjectsBag GetBag() const;
    LostObjectsBag& GetBag();

    Point CalculateNextPosition(std::uint64_t time_delta) const;

    void AccumulateScore(uint64_t score);
    uint64_t GetScore() const;

    void ResetRestTime() {
        rest_time_ = 0.0;
    }

    void IncrementRestTime(double time_delta) {
        rest_time_ += time_delta;
    }

    double GetRestTime() const {
        return rest_time_;
    }

    double GetUptime() const {
        return uptime_;
    }

    void IncrementUpTime(double time_delta) {
        uptime_ += time_delta;
    }

private:
    std::string name_;
    Id id_ {0};
    Point point_{ 0.f, 0.f };
    Speed speed_{ 0.f, 0.f };
    Direction direction_{ Direction::NORTH };
    float default_speed_{1};
    unsigned int bag_capacity_ {3};
    LostObjectsBag bag_{bag_capacity_};
    uint64_t score_ {0};
    double rest_time_ = {0.0};
    double uptime_ = {0.0};
};

class RoadLoader {
public:
using RoadPairs = std::vector<std::vector<Road>>;
    explicit RoadLoader(const std::vector<Road>& roads);

    std::vector<Road> GetDicts() const;

private:

    static RoadPairs CheckRoads(const std::vector<Road>& roads);
    static Road MergeRoads(const Road& road1, const Road& road2);
    void HandleTheRoads(); 

    std::vector<Road> vertical_roads_;
    std::vector<Road> horizontal_roads_;
    std::vector<Road> new_roads_;    
};

struct LootGeneratorConfig {
    float period_;
    float probability_;
};

class ItemDogProvider : public collision_detector::ItemGathererProvider {
  public:
    using Items = std::vector<collision_detector::Item>;
    using Gatherers = std::vector<collision_detector::Gatherer>;

    ItemDogProvider(Items items, Gatherers gatherers) :
        items_(std::move(items)),
        gatherers_(std::move(gatherers)) {}

    size_t ItemsCount() const override {
        return items_.size();
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        return items_[idx];
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

  private:
    Items items_;
    Gatherers gatherers_;
};

class GameSession {
public:
    explicit GameSession(const Map& map, const LootGeneratorConfig& config, double dog_retirement_time);
    GameSession() = delete;

    std::uint64_t AddDog(std::shared_ptr<model::Dog> dog, bool random_spawn);
    Map::Id GetMapId() const;
    std::vector<std::shared_ptr<model::Dog>> GetDogs() const;
    std::vector<Dog::Id> UpdateGameState(const std::int64_t time_delta);
    std::optional<Point> TryMoveOnMap(const Point& from, const Point& to) const;
    LootStates GetLootStates() const;

    std::shared_ptr<model::Dog> GetDog(std::uint64_t id) {
        for (auto& dog : dogs_) {
            if (*dog->GetDogId() == id) {
                return dog;
            }
        }
        return nullptr;
    }

    std::uint64_t GetDogIdCounter() const { return dog_id_counter_; }
    void SetDogIdCounter(std::uint64_t dog_id_counter) { dog_id_counter_ = dog_id_counter; }

    void EmplaceDogs(std::vector<std::shared_ptr<model::Dog>> dogs) {
        dogs_ = std::move(dogs);
    }

    LootGeneratorConfig GetLootGeneratorConfig() const {
        return {loot_generator_.GetConfig().first, loot_generator_.GetConfig().second};
    }

    void SetLootStates(const LootStates& states) {
        loot_states_ = states;
    }

private:
    void GenerateLootOnMap(unsigned loot_to_gen);
    static float GenerateRandomFloat(float min, float max);
    static size_t GetRandomSizeT(size_t n);
    Point GenerateRandomPosition();

    const Map map_;
    std::vector<std::shared_ptr<model::Dog>> dogs_;
    std::uint64_t dog_id_counter_{0};
    std::vector<Road> roads_;
    loot_gen::LootGenerator loot_generator_;
    LootStates loot_states_;
    double dog_retirement_time_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    GameSession* GetSession(const model::Map::Id& id) {
        if (sessions_.find(id) != sessions_.end()) {
            return &sessions_.at(id);
        }

        const Map* map = FindMap(id);
        if (!map) {
            return nullptr;
        }
        
        auto p = sessions_.emplace(id, GameSession{*map, loot_generator_config_, dog_retirement_time_});
        if (!p.second) {
            return nullptr;
        }

        return &p.first->second;
    }

    void SetDefaultDogSpeed(float speed) {
        default_dog_speed_ = speed;
    }

    void SetLootGeneratorConfig(const LootGeneratorConfig& config) {
        loot_generator_config_ = config;
    }

    LootGeneratorConfig GetLootGeneratorConfig() const {
        return loot_generator_config_;
    }

    std::optional<float> GetDefaultDogSpeed() const {
        return default_dog_speed_;
    }

    void SetDefaultBagCapacity(int capacity) {
        default_bag_capacity_ = capacity;
    }

    std::optional<int> GetDefaultBagCapacity() const {
        return default_bag_capacity_;
    }

    void SetDogRetirementTime(double time) {
        dog_retirement_time_ = time;
    }

    double GetDogRetirementTime() const {
        return dog_retirement_time_;
    }
    
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using GameSessions = std::unordered_map<Map::Id, GameSession, MapIdHasher>;

    GameSessions& GetSessions() {
        return sessions_;
    }

    GameSessions GetSessions() const {
        return sessions_;
    }

    void SetGameSessions(GameSessions sessions) {
        sessions_ = std::move(sessions);
    }
    
private:
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    GameSessions sessions_;
    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    std::optional<float> default_dog_speed_;
    LootGeneratorConfig loot_generator_config_;
    std::optional<int> default_bag_capacity_;
    // defalut retirement time is 60 seconds
    double dog_retirement_time_{60 * 1000.0};
};

}  // namespace model

#pragma once

#include <pqxx/pqxx>
#include <string>


namespace postgres {

struct PlayerInfo {
	std::string name;
	int score;
	double play_time;
};

class Database {
public:
	explicit Database(const std::string& conn);

	void AddRecord(const std::string& name, int score, double play_time);

	std::vector<PlayerInfo> GetRecords(std::optional<int> start, std::optional<int> maxItems);

	void AddRecords(const std::vector<PlayerInfo>& infos);

private:
	pqxx::connection conn_;		
};

}
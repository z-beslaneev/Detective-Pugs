#include "postgres.h"


namespace postgres {

using pqxx::operator"" _zv;

Database::Database(const std::string& conn) 
	: conn_{ conn } {
	pqxx::work w(conn_);
	w.exec(
		"CREATE TABLE IF NOT EXISTS retired_players (id SERIAL PRIMARY KEY, name varchar(100), score integer, time real);"_zv);

	w.commit();
}

void Database::AddRecord(const std::string& name, int score, double play_time) {
	pqxx::work w(conn_);
	w.exec("INSERT INTO retired_players (name, score, time) VALUES (" +
		w.quote(static_cast<std::string>(name)) + ", " + std::to_string(score) + ", " + std::to_string(play_time) + ")");
	w.commit();
}

std::vector<PlayerInfo> Database::GetRecords(std::optional<int> start, std::optional<int> maxItems) {
	pqxx::read_transaction r(conn_);

	constexpr size_t default_max = 100;
	std::size_t max = maxItems ? *maxItems : default_max;
	
	std::string query_text;
	if (start) {
		query_text = "SELECT name, score, time FROM retired_players ORDER BY score DESC, time ASC, name ASC OFFSET " + std::to_string(*start) + " LIMIT " + std::to_string(max) + ";";
	} else {
		query_text = "SELECT name, score, time FROM retired_players ORDER BY score DESC, time ASC, name ASC LIMIT "  + std::to_string(max) + ";";
	}

	std::vector<PlayerInfo> result;
	for (auto [name, score, time] : r.query<std::string, int, double>(query_text)) {
		result.push_back({ name, score, time });
	}

	return result;
}

void Database::AddRecords(const std::vector<PlayerInfo>& infos) {
	pqxx::work w(conn_);

	for (auto& info : infos) {
		w.exec("INSERT INTO retired_players (name, score, time) VALUES (" +
			w.quote(static_cast<std::string>(info.name)) + ", " + std::to_string(info.score) + ", " + std::to_string(info.play_time) + ")");
	}

	w.commit();
}

}
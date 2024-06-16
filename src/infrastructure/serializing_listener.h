#pragma once

#include <filesystem>

#include "infrastructure/application_serialization.h"
#include "application/application_listener.h"
#include "application/application.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace infrastructure {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;    

using namespace std::literals;
namespace fs = std::filesystem;

class SerializingListener : public IApplicationlListener {

public:
    SerializingListener(application::Application& app, const fs::path& save_file_path);
    void SetSavePeriod(const std::chrono::milliseconds& period);
    void OnUpdate(const std::chrono::milliseconds& delta) override;
    void Save();
private:
    void Restore();    

private:
    std::chrono::milliseconds total_ = 0ms;
    application::Application& app_;
    fs::path save_file_path_;
    std::optional<std::chrono::milliseconds> save_period_;
};
    
} // infrastructure
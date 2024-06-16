#include "serializing_listener.h"

#include <iostream>
#include <fstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>


infrastructure::SerializingListener::SerializingListener(application::Application& app, const fs::path& save_file_path)
    : app_(app)
    , save_file_path_(save_file_path) {
        
    if (fs::exists(save_file_path)) {
        std::ifstream archive{save_file_path_};
        InputArchive input_archive{ archive };
        serialization::ApplicationRepr repr;
        input_archive >> repr;
        repr.Restore(app_);
    }
}

void infrastructure::SerializingListener::SetSavePeriod(const std::chrono::milliseconds &period) {
    save_period_ = period;
}

void infrastructure::SerializingListener::OnUpdate(const std::chrono::milliseconds &delta) {

    if (!save_period_)
        return;

    total_ += delta;

    if (total_ >= save_period_) {
        Save();
        total_ = 0ms;
    }
}

void infrastructure::SerializingListener::Save() {

    std::ofstream archive{ save_file_path_ };
    OutputArchive output_archive{ archive };
    serialization::ApplicationRepr repr{ app_ };
    output_archive << repr;
}

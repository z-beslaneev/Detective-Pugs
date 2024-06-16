#include "logger_helper.h"

namespace {

    void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    
    auto ts = rec[timestamp];
    //strm << to_iso_extended_string(*ts) << ": ";
    json::object message;
    message["timestmap"] = to_iso_extended_string(*ts);

    auto value = rec[additional_value];
    if (value)
        message["data"] = *value;

    auto data = rec[additional_data];
    if (data)
        message["data"] = *data;

    message["message"] = *rec[logging::expressions::smessage];

    strm << json::serialize(message);
} 

}

void logger_helper::InitLogger() {

    logging::add_common_attributes();

    logging::add_console_log( 
        std::cout,
        keywords::format = &MyFormatter,
        keywords::auto_flush = true
        );
}
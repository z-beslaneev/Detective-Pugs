#pragma once
#define  _WIN32_WINNT   0x0601 //TODO delete 
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_value, "AdditionalValue", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::object)

namespace logger_helper
{
    void InitLogger();
}
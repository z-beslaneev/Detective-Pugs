cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake) 

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(model STATIC 
	src/model/collision_detector.h
	src/model/collision_detector.cpp
	src/model/geom.h
	src/model/model.h
	src/model/model.cpp
	src/model/model_properties.h
	src/model/model_serialization.h
	src/model/loot_generator.h
	src/model/loot_generator.cpp
	src/model/tagged.h)

add_executable(game_server
	src/application/application.h
	src/application/application.cpp
	src/application/application_listener.h
	src/application/player.h
	src/application/player.cpp
	src/http_server/http_server.cpp
	src/http_server/http_server.h
	src/database/postgres.h
	src/database/postgres.cpp
	src/request_handler/api_request_handler.h
	src/request_handler/api_request_handler.cpp
	src/request_handler/request_handler_helper.h
	src/request_handler/request_handler_helper.cpp
	src/request_handler/request_handler.cpp
	src/request_handler/request_handler.h
	src/infrastructure/serializing_listener.h
	src/infrastructure/serializing_listener.cpp
	src/infrastructure/application_serialization.h
	src/cli_helper.h
	src/cli_helper.cpp
	src/main.cpp
	src/sdk.h
	src/logger_helper.h
	src/logger_helper.cpp
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
)

target_include_directories(game_server PRIVATE
	src/ 
	src/application/
	src/http_server/
	src/model/
	src/request_handler/
	src/database/
	src/infrastructure/
    )

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost model CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server_tests
	tests/collision-detector-tests.cpp
    tests/model-tests.cpp
    tests/loot_generator_tests.cpp
	tests/state-serialization-tests.cpp
)

target_include_directories(game_server_tests PRIVATE CONAN_PKG::boost src/model/)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads model)

catch_discover_tests(game_server_tests)
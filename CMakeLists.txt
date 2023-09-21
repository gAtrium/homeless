cmake_minimum_required(VERSION 3.9)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(homeless)
find_package(CURL REQUIRED) 

add_executable(${PROJECT_NAME} main.cpp listing.cpp listing.hpp bot_sneedment.hpp scraper.cpp scraper.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE STATIC TgBot ${CURL_LIBRARIES})
include_directories(${CURL_INCLUDE_DIR})
cmake_minimum_required(VERSION 3.16)
project(mini-search-engine VERSION 0.1.0 LANGUAGES CXX)
set(DCMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Wshadow
  -Wno-unused-parameter
)

file(GLOB_RECURSE SEARCH_LIB_SOURCES
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_library(search_lib STATIC ${SEARCH_LIB_SOURCES})

target_include_directories(search_lib PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
)

target_compile_features(search_lib PUBLIC cxx_std_17)

foreach(APP crawl indexer search)
  if(APP STREQUAL "crawl")
    set(APP_SOURCE "${CMAKE_SOURCE_DIR}/app/crawl_main.cpp")
  elseif(APP STREQUAL "indexer")
    set(APP_SOURCE "${CMAKE_SOURCE_DIR}/app/index_main.cpp")
  else()
    set(APP_SOURCE "${CMAKE_SOURCE_DIR}/app/search_main.cpp")
  endif()

  add_executable(${APP} ${APP_SOURCE})
  target_link_libraries(${APP} PRIVATE search_lib)
  target_include_directories(${APP} PRIVATE "${CMAKE_SOURCE_DIR}/include")
endforeach()

enable_testing()
add_subdirectory(tests)

cmake_minimum_required(VERSION 3.24)
project(sqmix-mitm)

set(CMAKE_CXX_STANDARD 17)



add_library(sqmixmitm src/DiscoveryService.cpp src/DiscoveryService.h)
target_include_directories(sqmixmitm PUBLIC src)


include_directories(src)

add_executable(discovery-service EXCLUDE_FROM_ALL src/tools/discovery-service.cpp)
target_link_libraries(discovery-service sqmixmitm)

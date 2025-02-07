cmake_minimum_required(VERSION 3.24)
project(sqmix-mitm)

set(CMAKE_CXX_STANDARD 17)



add_library(sqmixmitm
        src/Discovery.cpp
        src/Discovery.h
        src/DiscoveryResponder.cpp
        src/DiscoveryResponder.h
        src/MidiControl.cpp
        src/MidiControl.h
        src/MitmService.cpp
        src/MitmService.h)
target_include_directories(sqmixmitm PUBLIC src)


include_directories(src)

add_executable(discovery EXCLUDE_FROM_ALL src/tools/discovery.cpp)
target_link_libraries(discovery sqmixmitm)

add_executable(discovery-responder EXCLUDE_FROM_ALL src/tools/discovery-responder.cpp)
target_link_libraries(discovery-responder sqmixmitm)

add_executable(midi-control EXCLUDE_FROM_ALL src/tools/midi-control.cpp)
target_link_libraries(midi-control sqmixmitm)

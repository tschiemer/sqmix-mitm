cmake_minimum_required(VERSION 3.24)
project(sqmix-mitm)

set(CMAKE_CXX_STANDARD 20)



add_library(sqmixmitm
        src/Discovery.cpp
        src/Discovery.h
        src/DiscoveryResponder.cpp
        src/DiscoveryResponder.h
        src/MidiControl.cpp
        src/MidiControl.h
        src/MixMitm.cpp
        src/MixMitm.h
        src/Event.cpp
        src/Event.h
        src/Command.cpp
        src/Command.h
        src/log.cpp
        src/log.h)
target_include_directories(sqmixmitm PUBLIC src)


include_directories(src)

add_executable(sq-discovery EXCLUDE_FROM_ALL src/tools/discovery.cpp)
target_link_libraries(sq-discovery sqmixmitm)

add_executable(sq-discovery-responder EXCLUDE_FROM_ALL src/tools/discovery-responder.cpp)
target_link_libraries(sq-discovery-responder sqmixmitm)

add_executable(sq-midi-control EXCLUDE_FROM_ALL src/tools/midi-control.cpp)
target_link_libraries(sq-midi-control sqmixmitm)

add_executable(sq-mitm EXCLUDE_FROM_ALL src/tools/mitm.cpp)
target_link_libraries(sq-mitm sqmixmitm)

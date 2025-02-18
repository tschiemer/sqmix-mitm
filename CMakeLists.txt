cmake_minimum_required(VERSION 3.24)
project(sqmix-mitm)

set(CMAKE_CXX_STANDARD 20)

set(SQMIXMITM_SUPPORT_SRC
        src/tools/midi.h
        )

add_library(sqmixmitm
        src/Discovery.cpp
        include/sqmixmitm/Discovery.h
        src/DiscoveryResponder.cpp
        include/sqmixmitm/DiscoveryResponder.h
        src/MidiControl.cpp
        include/sqmixmitm/MidiControl.h
        src/MixMitm.cpp
        include/sqmixmitm/MixMitm.h
        src/Event.cpp
        include/sqmixmitm/Event.h
        src/Command.cpp
        include/sqmixmitm/Command.h
        src/log.cpp
        include/sqmixmitm/log.h src/Version.cpp include/sqmixmitm/Version.h)
target_include_directories(sqmixmitm PUBLIC include)
target_include_directories(sqmixmitm PRIVATE include/sqmixmitm)

#include_directories(include/sqmixmitm)

add_executable(sq-discovery EXCLUDE_FROM_ALL src/tools/discovery.cpp ${SQMIXMITM_SUPPORT_SRC})
target_link_libraries(sq-discovery sqmixmitm)

add_executable(sq-discovery-responder EXCLUDE_FROM_ALL src/tools/discovery-responder.cpp ${SQMIXMITM_SUPPORT_SRC})
target_link_libraries(sq-discovery-responder sqmixmitm)

add_executable(sq-midi-control EXCLUDE_FROM_ALL src/tools/midi-control.cpp ${SQMIXMITM_SUPPORT_SRC})
target_link_libraries(sq-midi-control sqmixmitm)

add_executable(sq-mitm EXCLUDE_FROM_ALL src/tools/mitm.cpp ${SQMIXMITM_SUPPORT_SRC})
target_link_libraries(sq-mitm sqmixmitm)

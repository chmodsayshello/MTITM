cmake_minimum_required(VERSION 3.10)

project(MTITM C)

include_directories(
    include/
    mini-gmp/
    srp/
    srp/sha/
)

set(SOURCES
    src/main.c
    src/proxy.c
    src/mtserver.c
    src/genericpacket.c
    src/mtclient.c
    src/util.c

    mini-gmp/mini-gmp.c
    srp/srp.c
    srp/sha/sha1.c
    srp/sha/sha256.c
)

add_executable(MTITM ${SOURCES})

set_target_properties(MTITM PROPERTIES
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)
set(CMAKE_FIND_STATIC_PREFER_STATIC ON)
set(CMAKE_FIND_STATIC ON)
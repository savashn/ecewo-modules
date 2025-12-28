cmake_minimum_required(VERSION 3.14)
project(modules_test VERSION 1.0.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)
FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/savashn/ecewo.git
    GIT_TAG v3.0.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(ecewo)

add_executable(modules_test
    src/cluster/ecewo-cluster.c
    src/cookie/ecewo-cookie.c
    src/cors/ecewo-cors.c
    src/fs/ecewo-fs.c
    src/helmet/ecewo-helmet.c
    src/mock/ecewo-mock.c
    src/session/ecewo-session.c
    src/static/ecewo-static.c
    tests/test-runner.c
    tests/test-cluster.c
    tests/test-cookie.c
    tests/test-cors.c
    tests/test-helmet.c
    tests/test-session.c
    tests/test-fs.c
    tests/test-static.c
)

target_link_libraries(modules_test PRIVATE ecewo)

target_include_directories(modules_test PRIVATE
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/src/cluster
    ${CMAKE_SOURCE_DIR}/src/cookie
    ${CMAKE_SOURCE_DIR}/src/cors
    ${CMAKE_SOURCE_DIR}/src/fs
    ${CMAKE_SOURCE_DIR}/src/helmet
    ${CMAKE_SOURCE_DIR}/src/mock
    ${CMAKE_SOURCE_DIR}/src/session
    ${CMAKE_SOURCE_DIR}/src/static
)

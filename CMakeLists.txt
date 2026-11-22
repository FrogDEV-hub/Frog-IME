cmake_minimum_required(VERSION 3.16)
project(fcitx5-frog-ime VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
include(GNUInstallDirs)

find_package(Fcitx5Core REQUIRED)
find_package(Fcitx5Config REQUIRED)
find_package(Fcitx5Utils REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RIME REQUIRED IMPORTED_TARGET rime)

add_library(frog_ime_core STATIC
    src/layout.cpp
    src/rime_backend.cpp
    src/buffer.cpp
)
set_target_properties(frog_ime_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(frog_ime_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_options(frog_ime_core PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(frog_ime_core PUBLIC
    PkgConfig::RIME
)

add_library(frog-ime MODULE
    src/fcitx5_engine.cpp
    src/frog_ime_addon.cpp
)
set_target_properties(frog-ime PROPERTIES PREFIX "")
target_compile_options(frog-ime PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(frog-ime
    frog_ime_core
    Fcitx5::Core
    Fcitx5::Config
    Fcitx5::Utils
    PkgConfig::RIME
)

install(TARGETS frog-ime
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5"
)
install(FILES data/frog-ime.conf
    DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/addon"
)
install(FILES data/frog-ime-im.conf
    DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod"
    RENAME frog-ime.conf
)

enable_testing()
find_package(Catch2 3 REQUIRED)

add_executable(frog_ime_smoke_test
    tests/test_smoke.cpp
)
target_compile_options(frog_ime_smoke_test PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(frog_ime_smoke_test PRIVATE
    Catch2::Catch2WithMain
)
add_test(NAME smoke COMMAND frog_ime_smoke_test)

add_executable(frog_ime_layout_test
    tests/test_layout.cpp
)
target_compile_options(frog_ime_layout_test PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(frog_ime_layout_test PRIVATE
    frog_ime_core
    Catch2::Catch2WithMain
)
add_test(NAME layout COMMAND frog_ime_layout_test)

add_executable(frog_ime_rime_test
    tests/test_rime_backend.cpp
)
target_compile_options(frog_ime_rime_test PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(frog_ime_rime_test PRIVATE
    frog_ime_core
    Catch2::Catch2WithMain
)
add_test(NAME rime_backend COMMAND frog_ime_rime_test)

add_executable(frog_ime_buffer_test
    tests/test_buffer.cpp
)
target_compile_options(frog_ime_buffer_test PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(frog_ime_buffer_test PRIVATE
    frog_ime_core
    Catch2::Catch2WithMain
)
add_test(NAME buffer COMMAND frog_ime_buffer_test)

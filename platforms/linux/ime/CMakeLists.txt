cmake_minimum_required(VERSION 3.16)

project(RIMESLinuxIme VERSION 0.1.0 LANGUAGES C CXX)

option(RIMES_BUILD_FCITX5 "Build the Fcitx5 RIMES addon" ON)
option(RIMES_BUILD_TESTS "Build unit tests and headless E2E hosts" ON)
option(RIMES_BUILD_QT_HOST "Build the Qt E2E host when Qt5 is available" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

include(GNUInstallDirs)
include(CTest)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(Rime REQUIRED IMPORTED_TARGET rime>=1.7.0)

function(rimes_harden target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-missing-field-initializers
    )
endfunction()

add_library(RimesEngine STATIC
    src/engine/rime_engine.cpp
    src/engine/rime_key.cpp
    src/engine/rime_paths.cpp
    src/engine/rime_snapshot.cpp
)
target_include_directories(RimesEngine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(RimesEngine PUBLIC PkgConfig::Rime Threads::Threads)
set_target_properties(RimesEngine PROPERTIES POSITION_INDEPENDENT_CODE ON)
rimes_harden(RimesEngine)

if(RIMES_BUILD_FCITX5)
    find_package(Gettext REQUIRED)
    find_package(Fcitx5Core REQUIRED)
    find_package(Fcitx5Module REQUIRED COMPONENTS TestFrontend)
    include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

    add_library(rimes MODULE
        src/fcitx/rimes_candidate.cpp
        src/fcitx/rimes_ime.cpp
        src/fcitx/rimes_state.cpp
    )
    target_link_libraries(rimes PRIVATE Fcitx5::Core RimesEngine)
    target_compile_definitions(rimes PRIVATE
        RIMES_INSTALL_SHARED_DIR="${CMAKE_INSTALL_FULL_DATADIR}/rimes/data"
    )
    set_target_properties(rimes PROPERTIES
        PREFIX ""
        OUTPUT_NAME "rimes"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    rimes_harden(rimes)

    configure_file(
        data/rimes-addon.conf.in
        "${CMAKE_BINARY_DIR}/rimes-addon.conf"
        @ONLY
    )
    configure_file(
        data/rimes.conf.in
        "${CMAKE_BINARY_DIR}/rimes.conf"
        COPYONLY
    )
    configure_file(
        data/org.fcitx.Fcitx5.Addon.Rimes.metainfo.xml.in
        "${CMAKE_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Rimes.metainfo.xml"
        @ONLY
    )

    install(TARGETS rimes DESTINATION "${FCITX_INSTALL_ADDONDIR}")
    install(FILES "${CMAKE_BINARY_DIR}/rimes-addon.conf"
            RENAME rimes.conf
            DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
    install(FILES "${CMAKE_BINARY_DIR}/rimes.conf"
            DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
    install(FILES "${CMAKE_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Rimes.metainfo.xml"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
endif()

if(RIMES_BUILD_TESTS)
    add_executable(rimes-key-tests tests/rime_key_tests.cpp)
    target_link_libraries(rimes-key-tests PRIVATE RimesEngine)
    rimes_harden(rimes-key-tests)

    add_executable(rimes-snapshot-tests tests/rime_snapshot_tests.cpp)
    target_link_libraries(rimes-snapshot-tests PRIVATE RimesEngine)
    rimes_harden(rimes-snapshot-tests)

    add_executable(rimes-paths-tests tests/rime_paths_tests.cpp)
    target_link_libraries(rimes-paths-tests PRIVATE RimesEngine)
    rimes_harden(rimes-paths-tests)

    add_executable(rimes-engine-smoke tests/rime_engine_smoke.cpp)
    target_link_libraries(rimes-engine-smoke PRIVATE RimesEngine)
    rimes_harden(rimes-engine-smoke)

    add_test(NAME RimesKey COMMAND rimes-key-tests)
    add_test(NAME RimesSnapshot COMMAND rimes-snapshot-tests)
    add_test(NAME RimesPaths COMMAND rimes-paths-tests)

    pkg_check_modules(GTK3 gtk+-3.0)
    if(GTK3_FOUND)
        add_executable(rimes-gtk-host tests/e2e/gtk_host.c)
        target_include_directories(rimes-gtk-host PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(rimes-gtk-host PRIVATE ${GTK3_LIBRARIES})
        target_compile_options(rimes-gtk-host PRIVATE ${GTK3_CFLAGS_OTHER})
    endif()

    if(RIMES_BUILD_QT_HOST)
        find_package(Qt5 QUIET COMPONENTS Widgets)
        if(Qt5Widgets_FOUND)
            add_executable(rimes-qt-host tests/e2e/qt_host.cpp)
            target_link_libraries(rimes-qt-host PRIVATE Qt5::Widgets)
            set_target_properties(rimes-qt-host PROPERTIES
                AUTOMOC OFF
            )
        endif()
    endif()

    if(RIMES_BUILD_FCITX5)
        add_executable(rimes-fcitx-e2e tests/fcitx_e2e.cpp)
        target_link_libraries(rimes-fcitx-e2e PRIVATE
            Fcitx5::Core
            Fcitx5::Module::TestFrontend
            RimesEngine
        )
        rimes_harden(rimes-fcitx-e2e)

        # Layout expected by fcitx::setupTestingEnvironment.
        # setupTestingEnvironment treats each data dir as an fcitx5 data root,
        # so addon/ and inputmethod/ sit directly under test-data/.
        file(MAKE_DIRECTORY
            "${CMAKE_BINARY_DIR}/test-data/addon"
            "${CMAKE_BINARY_DIR}/test-data/inputmethod"
        )
        add_custom_command(TARGET rimes POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_BINARY_DIR}/rimes-addon.conf"
                "${CMAKE_BINARY_DIR}/test-data/addon/rimes.conf"
            COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_BINARY_DIR}/rimes.conf"
                "${CMAKE_BINARY_DIR}/test-data/inputmethod/rimes.conf"
        )
    endif()
endif()

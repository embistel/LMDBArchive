cmake_minimum_required(VERSION 3.21)
project(LMDBArchive VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Dependencies ---
set(CMAKE_PREFIX_PATH "F:/Qt/6.10.2/mingw_64" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
find_package(unofficial-lmdb CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

# --- LMDB Archive core library ---
add_library(lmdb_archive_core STATIC
    src/core/PathUtil.cpp
    src/core/FileSystemUtil.cpp
    src/core/HashUtil.cpp
    src/core/MetaCodec.cpp
    src/core/ArchiveLock.cpp
    src/lmdb/LmdbEnv.cpp
    src/lmdb/LmdbArchive.cpp
    src/model/ArchiveTreeNode.cpp
    src/model/ArchiveTreeBuilder.cpp
    src/worker/CreateArchiveWorker.cpp
    src/worker/ExtractWorker.cpp
    src/worker/VerifyWorker.cpp
    src/worker/PreviewDecodeWorker.cpp
)

target_include_directories(lmdb_archive_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(lmdb_archive_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Concurrent
    unofficial::lmdb::lmdb
    fmt::fmt
    spdlog::spdlog
    xxHash::xxhash
)

# --- CLI executable ---
add_executable(lmdb-archive-cli
    src/cli/CliMain.cpp
    src/cli/CliCommands.cpp
)

target_link_libraries(lmdb-archive-cli PRIVATE
    lmdb_archive_core
)

set_target_properties(lmdb-archive-cli PROPERTIES OUTPUT_NAME "lmdb-archive")

# --- GUI executable ---
add_executable(lmdb-archive-gui
    src/app/main.cpp
    src/app/AppController.cpp
    src/app/ShellIntegration.cpp
    src/ui/MainWindow.cpp
    src/ui/PreviewPane.cpp
    src/ui/ExtractDialog.cpp
    src/ui/ProgressDialog.cpp
    src/ui/PropertiesDialog.cpp
    src/ui/StatsDialog.cpp
    src/model/FolderTreeModel.cpp
    src/model/EntryTableModel.cpp
)

target_link_libraries(lmdb-archive-gui PRIVATE
    lmdb_archive_core
    Qt6::Widgets
)

set_target_properties(lmdb-archive-gui PROPERTIES
    OUTPUT_NAME "LMDBArchive"
    WIN32_EXECUTABLE TRUE
)

# --- Tests ---
enable_testing()

add_executable(lmdb_archive_tests
    tests/test_pathutil.cpp
    tests/test_archive_roundtrip.cpp
    tests/test_treebuilder.cpp
    tests/test_verify.cpp
    tests/test_single_file_mode.cpp
)

target_link_libraries(lmdb_archive_tests PRIVATE
    lmdb_archive_core
    Catch2::Catch2WithMain
)

include(CTest)
add_test(NAME test_pathutil COMMAND lmdb_archive_tests "[path]")
add_test(NAME test_archive COMMAND lmdb_archive_tests "[archive]")
add_test(NAME test_tree COMMAND lmdb_archive_tests "[tree]")
add_test(NAME test_verify COMMAND lmdb_archive_tests "[verify]")
add_test(NAME test_single_file COMMAND lmdb_archive_tests "[single-file]")

# CMakeList.txt: AOSPBuildSpyDB 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

if(WIN32)
    set(TSQLITE_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/windows/lib)
    set(TSQLITE_INC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/windows/include)
else()
    set(TSQLITE_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/linux/lib)
    set(TSQLITE_INC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/linux/include)
endif()

link_directories (${TSQLITE_LIB_PATH})
include_directories (${TSQLITE_INC_PATH})

# 将源代码添加到此项目的可执行文件。
add_executable (AOSPBuildSpyDB
	AOSPBuildSpyDB.cpp
    TSQLite.cpp
    TBuildSrcIncludeFetcher.cpp)

target_link_libraries (AOSPBuildSpyDB
    sqlite3
)

# 安装。
if(WIN32)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite/windows/lib/sqlite3.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
endif()

# TODO: 如有需要，请添加测试并安装目标。

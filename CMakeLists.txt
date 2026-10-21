cmake_minimum_required(VERSION 3.10)
project(litellm-tde-applet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TDE/R14 paths - Q4OS installs TQt3 headers in /usr/include/tqt3, libs in /usr/lib/x86_64-linux-gnu
set(TDE_PREFIX "/opt/trinity" CACHE PATH "TDE installation prefix")
set(TQT_INCLUDE_DIRS "/usr/include/tqt3" CACHE PATH "TQt3 include dirs")
set(TQT_COMPAT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/compat_headers" CACHE PATH "TQt3 compat headers (tq* symlinks)")
set(TQT_LIB_DIRS "/usr/lib/x86_64-linux-gnu" CACHE PATH "TQt3 lib dirs")
set(TDE_INCLUDE_DIRS "${TDE_PREFIX}/include" CACHE PATH "TDE include dirs")
set(TDE_LIB_DIRS "${TDE_PREFIX}/lib" CACHE PATH "TDE lib dirs")

# Find TQt (TDE's Qt3 fork) - headers in /usr/include/tqt3, libs in /usr/lib/x86_64-linux-gnu
find_path(TQT_QT_H "ntqglobal.h" PATHS ${TQT_INCLUDE_DIRS} NO_DEFAULT_PATH)
find_library(TQT_LIBRARY tqt-mt PATHS ${TQT_LIB_DIRS} NO_DEFAULT_PATH)

# Find TDE libraries - headers may be in tdebase-trinity-dev package
find_path(TDE_KAPP_H "kapp.h" PATHS ${TDE_INCLUDE_DIRS} /usr/include NO_DEFAULT_PATH)
find_path(TDE_KPANELAPPLET_H "kpanelapplet.h" PATHS ${TDE_INCLUDE_DIRS} /usr/include NO_DEFAULT_PATH)
find_library(TDE_TDECORE_LIB tdecore PATHS ${TDE_LIB_DIRS} NO_DEFAULT_PATH)
find_library(TDE_TDEUI_LIB tdeui PATHS ${TDE_LIB_DIRS} NO_DEFAULT_PATH)

if(NOT TQT_QT_H OR NOT TQT_LIBRARY)
    message(FATAL_ERROR "TQt development headers/libs not found. Install: libtqt3-mt-dev")
endif()

if(NOT TDE_KAPP_H OR NOT TDE_KPANELAPPLET_H OR NOT TDE_TDECORE_LIB OR NOT TDE_TDEUI_LIB)
    message(WARNING "TDE development headers/libs not fully found. Install: tdelibs14-trinity-dev tdebase-trinity-dev")
    message(WARNING "TDE_KAPP_H: ${TDE_KAPP_H}")
    message(WARNING "TDE_KPANELAPPLET_H: ${TDE_KPANELAPPLET_H}")
    message(WARNING "TDE_TDECORE_LIB: ${TDE_TDECORE_LIB}")
    message(WARNING "TDE_TDEUI_LIB: ${TDE_TDEUI_LIB}")
endif()

message(STATUS "TDE prefix: ${TDE_PREFIX}")
message(STATUS "TQt includes: ${TQT_INCLUDE_DIRS}")
message(STATUS "TDE includes: ${TDE_INCLUDE_DIRS}")
message(STATUS "TDE libs: ${TDE_LIB_DIRS}")

include_directories(${TQT_COMPAT_INCLUDE_DIRS} ${TQT_INCLUDE_DIRS} ${TDE_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Find Qt3/TQt3 moc
find_program(TQT_MOC tqmoc NAMES moc PATHS /opt/trinity/bin /usr/lib/tqt3/bin /usr/bin)

# Applet library (loaded by kicker) - with MOC handling
set(APPLET_HEADERS src/litellm_applet.h src/litellm_process.h)
set(APPLET_SOURCES src/litellm_applet.cpp src/litellm_process.cpp)

# TDE applets don't use "lib" prefix - set output name explicitly
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_SHARED_MODULE_SUFFIX ".so")

if(TQT_MOC)
    # Generate MOC files manually for both classes with TQ_OBJECT
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_litellm_applet.cpp
        COMMAND ${TQT_MOC} ${CMAKE_CURRENT_SOURCE_DIR}/src/litellm_applet.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_litellm_applet.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/litellm_applet.h
        COMMENT "Running TQt3 moc on litellm_applet.h"
    )
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_litellm_process.cpp
        COMMAND ${TQT_MOC} ${CMAKE_CURRENT_SOURCE_DIR}/src/litellm_process.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_litellm_process.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/litellm_process.h
        COMMENT "Running TQt3 moc on litellm_process.h"
    )
    add_library(litellm_applet MODULE
        ${APPLET_SOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/moc_litellm_applet.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/moc_litellm_process.cpp
    )
    # Force output name without "lib" prefix
    set_target_properties(litellm_applet PROPERTIES
        OUTPUT_NAME "litellm_applet"
        PREFIX ""
        SUFFIX ".so"
    )
else()
    message(FATAL_ERROR "TQt3 moc (tqmoc) not found. Install tqt3-dev-tools")
endif()

target_link_libraries(litellm_applet
    ${TQT_LIBRARY}
    ${TDE_TDECORE_LIB}
    ${TDE_TDEUI_LIB}
)

# Export symbols for tdeinit mechanism
target_compile_definitions(litellm_applet PRIVATE
    MAKE_LITELLM_APPLET_LIB
)

# Install applet .so to TDE applet dir
install(TARGETS litellm_applet
    LIBRARY DESTINATION ${TDE_PREFIX}/lib/trinity
)

# Install .desktop file
install(FILES litellm-applet.desktop
    DESTINATION ${TDE_PREFIX}/share/apps/kicker/applets
)

# Install icons
install(FILES
    icons/litellm_idle.png
    icons/litellm_running.png
    icons/litellm_error.png
    icons/litellm_stopped.png
    DESTINATION ${TDE_PREFIX}/share/icons/hicolor/22x22/apps
)
cmake_minimum_required(VERSION 3.1)

project(wovncviewer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)
find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

set(SOURCE_FILES
    main.cpp
    qkxvncviewer.cpp
    qkxconnectiondialog.cpp
)

set(HEADER_FILES
    version.h
    qkxvncviewer.h
    qkxconnectiondialog.h
)

set(OTHER_FILES
    vncviewer.qrc
    qkxvncviewer.ui
    qkxconnectiondialog.ui
)

if(WIN32)
    set(OTHER_FILES ${OTHER_FILES}        
    )
elseif(APPLE)    
    message("APPLE Here")
else()

endif()

include_directories(
    ${KXVNC_ROOT_DIR}
    ${KXUTIL_ROOT_DIR}
    ${ZLIB_ROOT_DIR}/include
)

link_directories(${ZLIB_ROOT_DIR}/lib
)

if(WIN32)
    message("window system.")
    link_libraries(ws2_32 crypt32 kxutil kxvnc)
    set(BUILD_VERSION_MAJOR 0)
    set(BUILD_VERSION_MINOR 0)
    set(BUILD_VERSION_PATCH 0)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/version.h BUILD_VERSION)
    message("buildVersion:${BUILD_VERSION}")
    string(REGEX MATCHALL "WOVNCVIEWER_VERSION.*\\(\"(.*)\"\\)" BUILD_VERSION ${BUILD_VERSION})
    set(BUILD_VERSION ${CMAKE_MATCH_1})
    message("versionContent:${BUILD_VERSION}")
    string(REPLACE "." ";" VERSION_LIST ${BUILD_VERSION})
    message("versionList:${VERSION_LIST}")
    list(LENGTH VERSION_LIST len)
    list(GET VERSION_LIST 0 BUILD_VERSION_MAJOR)
    list(GET VERSION_LIST 1 BUILD_VERSION_MINOR)
    if( ${len} EQUAL 3)
        list(GET VERSION_LIST 2 BUILD_VERSION_PATCH)
    endif()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/versioninfo.rc.in versioninfo.rc @ONLY)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES} ${HEADER_FILES} ${OTHER_FILES})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/versioninfo.rc") 
elseif(APPLE)
    message("linux system.")
    link_libraries(kxutil kxvnc)
    add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${OTHER_FILES})

    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND cp -f "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wovncviewer" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wovncserver.app/Contents/MacOS/"
    )
else()
    message("linux system.")
    link_libraries(kxutil kxvnc)
    add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${OTHER_FILES})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets Qt5::Network)


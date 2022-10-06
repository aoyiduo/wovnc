cmake_minimum_required(VERSION 3.1)

project(wovncserver LANGUAGES CXX)

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
    qkxsetting.cpp
    qkxutils.cpp
    qkxsystemconfig.cpp
    qkxguiapplication.cpp
    qkxmoredialog.cpp
)

set(HEADER_FILES
    version.h
    qkxglobal.h
    qkxsetting.h
    qkxutils.h
    qkxsystemconfig.h
    qkxguiapplication.h
    qkxmoredialog.h
)

set(OTHER_FILES
    vncserver.qrc
    qkxsystemconfig.ui
    qkxmoredialog.ui
)

if(WIN32)
    set(SOURCE_FILES ${SOURCE_FILES}
        qkxutils_win.cpp
    )

    set(HEADER_FILES ${HEADER_FILES}
    )

elseif(APPLE)    
    message("APPLE Here")
    find_library(corefoundation_lib CoreFoundation REQUIRED)
    find_library(cocoa_lib Cocoa REQUIRED)
    find_library(coremedia_lib CoreMedia REQUIRED)
    find_library(avfoundation_lib AVFoundation REQUIRED)
    find_library(coregraphics_lib CoreGraphics REQUIRED)
    find_library(corevideo_lib CoreVideo REQUIRED)
    find_library(iokit_lib IOKit REQUIRED)
    find_library(appkit_lib APPKit REQUIRED)
    find_library(iosurface_lib IOSurface REQUIRED)

    set(SOURCE_FILES ${SOURCE_FILES}
        qkxutils_mac.cpp
    )

    set(HEADER_FILES ${HEADER_FILES}
        version.h
    )

    link_libraries(
        ${corefoundation_lib}
        ${cocoa_lib}
        ${coremedia_lib}
        ${avfoundation_lib}
        ${coregraphics_lib}
        ${corevideo_lib}
        ${iokit_lib}
        ${appkit_lib}
        ${iosurface_lib}
    )
else()
    find_package(X11 REQUIRED)
    if(!X11_XTest_FOUND)
        message(FATAL_ERROR "X11 extensions are required, but not found!")
    endif()
    if(!X11_Xfixes_LIB)
        message(FATAL_ERROR "X11 fixes extension is required, but not found!")
    endif()
    if(!X11_XShm_LIB)
        message(FATAL_ERROR "X11 XShm extension is required, but not found!")
    endif()

    message("other")
    set(SOURCE_FILES ${SOURCE_FILES}
        qkxutils_unix.cpp
    )

    set(HEADER_FILES ${HEADER_FILES}
    )
endif()

include_directories(${SERVICE_ROOT_DIR}
    ${ZLIB_ROOT_DIR}/include
    ${AES_ROOT_DIR}
    ${KXCAP_ROOT_DIR}
    ${LIBQTSERVICE_ROOT_DIR}/include
    ${LIBYUV_ROOT_DIR}/include
)

link_directories(${ZLIB_ROOT_DIR}/lib
    ${LIBQTSERVICE_ROOT_DIR}/lib
    ${LIBYUV_ROOT_DIR}/lib
)

if(WIN32)
    message("window system.")
    set(ZLIB_LIBRARY zlibstatic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        link_libraries(qtserviced)
    else()
        link_libraries(qtservice)
    endif()
    link_libraries(zlibstatic ws2_32 crypt32 kxcap kxutil)
    set(BUILD_VERSION_MAJOR 0)
    set(BUILD_VERSION_MINOR 0)
    set(BUILD_VERSION_PATCH 0)
    file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/version.h BUILD_VERSION)
    message("buildVersion:${BUILD_VERSION}")
    string(REGEX MATCHALL "WOVNCSERVER_VERSION.*\\(\"(.*)\"\\)" BUILD_VERSION ${BUILD_VERSION})
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
    find_package(Threads REQUIRED)
    find_library(corefoundation_lib CoreFoundation REQUIRED)
    find_library(cocoa_lib Cocoa REQUIRED)
    find_library(coremedia_lib CoreMedia REQUIRED)
    find_library(avfoundation_lib AVFoundation REQUIRED)
    find_library(coregraphics_lib CoreGraphics REQUIRED)
    find_library(corevideo_lib CoreVideo REQUIRED)

    set(ZLIB_LIBRARY  libz.a)
    link_libraries(
        libz.a
        qtservice
        kxcap
        ${CMAKE_THREAD_LIBS_INIT}
        ${corefoundation_lib}
        ${cocoa_lib}
        ${coremedia_lib}
        ${avfoundation_lib}
        ${coregraphics_lib}
        ${corevideo_lib}
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sectcreate CGPreLoginApp __CGPreLoginApp /dev/null")
    #add_compile_options(-sectcreate CGPreLoginApp __CGPreLoginApp /dev/null)
    # And this part tells CMake where to find and install the file itself
    set(PROGRAM_VERSION "1.0" CACHE STRINGS "you can modify this value by command : cmake -DPROGRAM_VERSION=\"xx\"")
    set(myApp_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resource/wovncserver.icns)
    set_source_files_properties(${myApp_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${myApp_ICON} ${SOURCE_FILES} ${HEADER_FILES} ${OTHER_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUNDLE True
        MACOSX_BUNDLE_INFO_STRING "WoVNCServer Remote Assistant"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.aoyiduo.wovncserver"
        MACOSX_BUNDLE_ICON_FILE "wovncserver.icns"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROGRAM_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROGRAM_VERSION}
        MACOSX_BUNDLE_BUNDLE_NAME "WoVNCServer"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resource/template.plist.in
    )
    add_custom_command(TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMAND rm -f "$<TARGET_FILE_DIR:${PROJECT_NAME}>/wovncviewer"
    )
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wovncviewer"
        COMMAND cp -f "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wovncviewer" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/wovncviewer"
    )
else()
    set(ZLIB_LIBRARY  libz.a)
    link_libraries(
        libz.a
        qtservice        
        kxcap
        ${X11_LIBRARIES}
        ${X11_Xfixes_LIB}
        ${X11_XTest_LIB}
        ${X11_XShm_LIB}
    )
    #link_libraries(libz.a qtservice X11 Xext Xtst Xfixes)
    message("linux system.")
    add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${OTHER_FILES})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets Qt5::Network)


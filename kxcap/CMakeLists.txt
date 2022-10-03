cmake_minimum_required(VERSION 3.1)

project(kxcap LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

add_definitions(-DKXCAP_LIBRARY)

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

set(SOURCE_FILES
    qkxcap.cpp
    qkxclipboard.cpp
    qkxcapoption.cpp
    qkxdirtyframe.cpp
    qkxserviceapplication.cpp
    qkxdaemonmaster.cpp
    qkxdaemonslave.cpp
    qkxserviceprocess.cpp
    qkxscreenlistener.cpp    
    qkxcapserverprivate.cpp
    qkxcapserver.cpp
    qkxvncserver.cpp
    qkxvnccompress.cpp
    qkxvnccompressabort.cpp
    qkxtcpvncserver.cpp
    qkxlocalvncserver.cpp
    qkxvncserverpeer.cpp
    qkxscreencapture.cpp
    qkxprivacyscreen.cpp    
    qkxabstractcapture.cpp
    qkxaudiostream.cpp
    qkxaudiocapture.cpp
    qkxsendinput.cpp
    qkxftpworker.cpp
    qkxresample.cpp
    qkxutils.cpp
    qkxh264encoder.cpp
    qkxjpegencoder.cpp
    qkxopusencoder.cpp
    d3des.cpp
    resample/filterkit.cpp
    resample/resample.cpp
    resample/resamplesubs.cpp
)

set(HEADER_FILES
    qkxcap_private.h
    qkxcap_share.h
    qkxcap.h
    qkxclipboard.h
    qkxcapoption.h
    qkxdirtyframe.h
    qkxserviceapplication.h
    qkxdaemonmaster.h
    qkxdaemonslave.h
    qkxserviceprocess.h
    qkxscreenlistener.h
    qkxcapserverprivate.h
    qkxcapserver.h
    qkxvncserver.h
    qkxvnccompress.h
    qkxvnccompressabort.h
    qkxtcpvncserver.h
    qkxlocalvncserver.h
    qkxvncserverpeer.h
    qkxscreencapture.h
    qkxprivacyscreen.h
    qkxabstractcapture.h
    qkxaudiostream.h
    qkxaudiocapture.h
    qkxsendinput.h
    qkxftpworker.h
    qkxresample.h
    keysymdef.h
    qkxutils.h
    qkxh264encoder.h
    qkxjpegencoder.h
    qkxopusencoder.h
    d3des.h
    resample/filterkit.h
    resample/configtemplate.h
    resample/libresample.h
    resample/resample_defs.h
)

if(WIN32)
    set(SOURCE_FILES ${SOURCE_FILES}
        qkxcapserverwindowprivate.cpp
        qkxutils_win.cpp
        qkxwindows.cpp
        qkxdxgicapture.cpp
        qkxdxgiscreencapture.cpp
        qkxgdicapture.cpp
        qkxgdiscreencapture.cpp
        qkxscreenlistener_win.cpp
        qkxwinsendinput.cpp
        qkxwinaudiocapture.cpp
        qkxserviceprocess_win.cpp
        qkxwin8privacyscreen.cpp
        qkxwinclipboard.cpp
    )

    set(HEADER_FILES ${HEADER_FILES}
        qkxcapserverwindowprivate.h
        qkxwindows.h
        qkxdxgicapture.h
        qkxdxgiscreencapture.h
        qkxgdicapture.h
        qkxgdiscreencapture.h
        qkxwinsendinput.h
        qkxwinaudiocapture.h
        qkxwin8privacyscreen.h
        qkxwinclipboard.h
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
        objcapi.cpp
        qkxutils_mac.cpp
        qkxmaccapture.cpp
        qkxmacscreencapture.cpp
        qkxmacsendinput.cpp
        qkxmacclipboard.mm
        qkxscreenlistener_unix.cpp
        qkxcapserverunixprivate.cpp
        qkxserviceprocess_unix.cpp
    )

    set(HEADER_FILES ${HEADER_FILES}
        keycode.h
        macnap.h
        objcapi.h
        qkxmaccapture.h
        qkxmacsendinput.h
        qkxmacclipboard.h
        qkxmacscreencapture.h
        qkxcapserverunixprivate.h
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
        qkxscreenlistener_unix.cpp
        qkxx11sendinput.cpp
        qkxx11capture.cpp
        qkxx11screencapture.cpp
        qkxcapserverunixprivate.cpp
        qkxserviceprocess_unix.cpp
        qkxx11clipboard.cpp
    )

    set(HEADER_FILES ${HEADER_FILES}
        keycode.h
        qkxx11sendinput.h
        qkxx11capture.h
        qkxx11screencapture.h
        qkxcapserverunixprivate.h
        qkxx11clipboard.h
    )
endif()

include_directories(${SERVICE_ROOT_DIR}
    ${LIBOPENH264_ROOT_DIR}/include    
    ${LIBYUV_ROOT_DIR}/include
    ${KXUTIL_ROOT_DIR}
    ${KXFTP_ROOT_DIR}
    ${LIBPORTAUDIO_ROOT_DIR}/include
    ${LIBOPUS_ROOT_DIR}/include
    ${LIBJPEG_ROOT_DIR}/include
    ${LIBQTSERVICE_ROOT_DIR}/include
    ${LIBYUV_ROOT_DIR}/include
)
link_directories(${LIBOPENH264_ROOT_DIR}/lib
    ${LIBPORTAUDIO_ROOT_DIR}/lib
    ${LIBOPUS_ROOT_DIR}/lib
    ${LIBJPEG_ROOT_DIR}/lib
    ${LIBQTSERVICE_ROOT_DIR}/lib
    ${LIBYUV_ROOT_DIR}/lib
)

if(WIN32)
    message("window system.")    
    link_libraries(ws2_32 crypt32 qtservice kxutil kxftp yuv)

    add_library(portaudio_main STATIC IMPORTED)
    set_target_properties(portaudio_main PROPERTIES IMPORTED_LOCATION
        ${LIBPORTAUDIO_ROOT_DIR}/lib/portaudio.lib)
    link_libraries(portaudio_main)

    add_library(opus_main STATIC IMPORTED)
    set_target_properties(opus_main PROPERTIES IMPORTED_LOCATION
        ${LIBOPUS_ROOT_DIR}/lib/opus.lib)
    link_libraries(opus_main)

    add_library(openh264_main STATIC IMPORTED)
    set_target_properties(openh264_main PROPERTIES IMPORTED_LOCATION
        ${LIBOPENH264_ROOT_DIR}/lib/openh264.lib)
    link_libraries(openh264_main)

    add_library(jpeg_main STATIC IMPORTED)
    set_target_properties(jpeg_main PROPERTIES IMPORTED_LOCATION
        ${LIBJPEG_ROOT_DIR}/lib/turbojpeg-static.lib)
    link_libraries(jpeg_main)

elseif(APPLE)
    find_package(Threads REQUIRED)
    find_library(corefoundation_lib CoreFoundation REQUIRED)
    find_library(cocoa_lib Cocoa REQUIRED)
    find_library(coremedia_lib CoreMedia REQUIRED)
    find_library(avfoundation_lib AVFoundation REQUIRED)
    find_library(coregraphics_lib CoreGraphics REQUIRED)
    find_library(corevideo_lib CoreVideo REQUIRED)
    find_library(coreaudio_library CoreAudio REQUIRED)
    find_library(audiotoolbox_library AudioToolbox REQUIRED)
    find_library(audiounit_library AudioUnit REQUIRED)
    find_library(coreservices_library CoreServices REQUIRED)

    add_library(portaudio_main STATIC IMPORTED)
    set_target_properties(portaudio_main PROPERTIES IMPORTED_LOCATION
        ${LIBPORTAUDIO_ROOT_DIR}/lib/libportaudio.a)
    link_libraries(portaudio_main)

    add_library(opus_main STATIC IMPORTED)
    set_target_properties(opus_main PROPERTIES IMPORTED_LOCATION
        ${LIBOPUS_ROOT_DIR}/lib/libopus.a)
    link_libraries(opus_main)

    add_library(openh264_main STATIC IMPORTED)
    set_target_properties(openh264_main PROPERTIES IMPORTED_LOCATION
        ${LIBOPENH264_ROOT_DIR}/lib/libopenh264.a)
    link_libraries(openh264_main pthread)

    add_library(jpeg_main STATIC IMPORTED)
    set_target_properties(jpeg_main PROPERTIES IMPORTED_LOCATION
        ${LIBJPEG_ROOT_DIR}/lib/libturbojpeg.a)
    link_libraries(jpeg_main)

    link_libraries(
        yuv
        qtservice
        kxutil
        kxftp
        ${CMAKE_THREAD_LIBS_INIT}
        ${corefoundation_lib}
        ${cocoa_lib}
        ${coremedia_lib}
        ${avfoundation_lib}
        ${coregraphics_lib}
        ${corevideo_lib}
        ${coreaudio_library}
        ${audiotoolbox_library}
        ${audiounit_library}
        ${coreservices_library}
    )    
else()
    link_libraries(
        qtservice
        kxutil
        kxftp
        yuv
        ${X11_LIBRARIES}
        ${X11_Xfixes_LIB}
        ${X11_XTest_LIB}
        ${X11_XShm_LIB}
    )
    add_library(portaudio_main STATIC IMPORTED)
    set_target_properties(portaudio_main PROPERTIES IMPORTED_LOCATION
        ${LIBPORTAUDIO_ROOT_DIR}/lib/libportaudio.a)
    link_libraries(portaudio_main)

    add_library(opus_main STATIC IMPORTED)
    set_target_properties(opus_main PROPERTIES IMPORTED_LOCATION
        ${LIBOPUS_ROOT_DIR}/lib/libopus.a)
    link_libraries(opus_main)

    add_library(openh264_main STATIC IMPORTED)
    set_target_properties(openh264_main PROPERTIES IMPORTED_LOCATION
        ${LIBOPENH264_ROOT_DIR}/lib/libopenh264.a)
    link_libraries(openh264_main pthread)

    add_library(jpeg_main STATIC IMPORTED)
    set_target_properties(jpeg_main PROPERTIES IMPORTED_LOCATION
        ${LIBJPEG_ROOT_DIR}/lib/libturbojpeg.a)
    link_libraries(jpeg_main)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES} ${OTHER_FILES})
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets Qt5::Network)


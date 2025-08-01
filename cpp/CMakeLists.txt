cmake_minimum_required(VERSION 3.10)

project(video_processor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-fpermissive)
if (MINGW)
    add_compile_options(-Wno-narrowing)
endif()


find_package(OpenGL REQUIRED)

set(GLEW_USE_STATIC_LIBS TRUE)
find_package(GLEW REQUIRED)

add_executable(video_processor main.cpp)

find_package(PkgConfig REQUIRED)

pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(AVFILTER REQUIRED IMPORTED_TARGET libavfilter)

if (UNIX AND NOT APPLE)
    find_package(glfw3 REQUIRED)

    pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
    pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
    pkg_check_modules(XAU REQUIRED IMPORTED_TARGET xau)
    pkg_check_modules(XDMCP REQUIRED IMPORTED_TARGET xdmcp)

    target_link_libraries(video_processor
        PRIVATE
            PkgConfig::AVCODEC
            PkgConfig::AVFORMAT
            PkgConfig::AVFILTER
            PkgConfig::AVUTIL
            PkgConfig::SWSCALE
            PkgConfig::X11
            PkgConfig::XCB
            PkgConfig::XAU
            PkgConfig::XDMCP
            glfw
            GLEW::glew_s
            ${OPENGL_gl_LIBRARY}
            X11
    )
    target_include_directories(video_processor
        PRIVATE
            ${glfw_INCLUDE_DIRS}
            ${GLEW_INCLUDE_DIRS}
    )
elseif (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
    set(FFMPEG_ROOT "E:/Development/Sources/dependencies/ffmpeg")

    target_link_libraries(video_processor
        PRIVATE
            ${FFMPEG_ROOT}/libavformat/libavformat.a
            ${FFMPEG_ROOT}/libavcodec/libavcodec.a
            ${FFMPEG_ROOT}/libavfilter/libavfilter.a
            ${FFMPEG_ROOT}/libswscale/libswscale.a
            ${FFMPEG_ROOT}/libavutil/libavutil.a
            ${FFMPEG_ROOT}/libswresample/libswresample.a
            va
            va_win32
            opengl32
            gdi32
            user32
            ws2_32
            z
            bz2
            lzma
            iconv
            crypt32
            ncrypt
            secur32
            uuid
            ole32
            strmiids
            "C:/libs/GLFW/lib/libglfw3.a"
            GLEW::glew_s
            ${OPENGL_gl_LIBRARY}
    )

    target_include_directories(video_processor
        PRIVATE
            ${FFMPEG_ROOT}
            "C:/libs/GLFW/include"
            ${GLEW_INCLUDE_DIRS}
    )
endif()


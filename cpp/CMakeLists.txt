cmake_minimum_required(VERSION 3.10)
project(video_processor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-fpermissive)
if (MINGW)
    add_compile_options(-Wno-narrowing)
endif()

find_package(PkgConfig REQUIRED)

# FFmpeg (shared across platforms)
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(AVFILTER REQUIRED IMPORTED_TARGET libavfilter)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

set(GLEW_USE_STATIC_LIBS TRUE)
find_package(GLEW REQUIRED)

add_executable(video_processor main.cpp)

if (UNIX AND NOT APPLE)
    # Linux-specific packages
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

elseif (WIN32)
    # Windows-specific system libraries
    target_link_libraries(video_processor
        PRIVATE
            PkgConfig::AVCODEC
            PkgConfig::AVFORMAT
            PkgConfig::AVFILTER
            PkgConfig::AVUTIL
            PkgConfig::SWSCALE
            glfw
            GLEW::glew_s
            ${OPENGL_gl_LIBRARY}
            opengl32
            gdi32
            user32
    )
endif()

target_include_directories(video_processor
    PRIVATE
        ${glfw_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
)

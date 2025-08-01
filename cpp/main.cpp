#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <numeric>
#include <limits>
#include <array>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavformat/avformat.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/opt.h>
    #include <libavutil/avstring.h>
    #include <libavutil/frame.h>
    #include <libavutil/avutil.h>
    #include <libavutil/error.h>
    #include <libavutil/fifo.h>
    #include <libavformat/avio.h>
    #include <libswresample/swresample.h>
}

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <cstdio>
#endif

#include "hash.h"
#include "offset.h"
#include "shader.h"
#include "video.h"
#include "audio.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform sampler2D offsetMap;

void main() {
    vec4 shuffle_sample = texture(offsetMap, TexCoord);
    vec2 decoded_offset = shuffle_sample.xy;
    
    vec2 base_new_uv = TexCoord + decoded_offset;

    vec4 c = texture(ourTexture, base_new_uv);

    FragColor = vec4(1-c.rgb, c.a);
}
)";


int main(int argc, char* argv[]){
    std::string seed = "my_secret_seed_123"; // Or the token as i'm calling it lol
    std::string inpath = "input_video.mp4";
    std::string outpath = "output_video.mp4";

    if (argc > 1)
        inpath = std::string(argv[1]);

    if (argc > 2)
        outpath = std::string(argv[2]);

    if (argc > 3)
        seed = std::string(argv[3]);

    UnsafeYT::Video Processor(
        vertexShaderSource,
        fragmentShaderSource,
        inpath,
        outpath + "noaudio.mp4",
        seed
    );

    Processor.Start();

    UnsafeYT::mixAudioAndAddSine(
        outpath + "noaudio.mp4",
        inpath,
        outpath
    );


    if (std::filesystem::exists(outpath + "noaudio.mp4")) {
        std::filesystem::remove(outpath + "noaudio.mp4");
    }

    return 0;
}

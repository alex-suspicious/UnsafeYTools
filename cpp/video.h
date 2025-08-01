namespace UnsafeYT {
    class Video {
    public:
        GLFWwindow* window;
        GLuint shaderProgram;
        GLuint inputTexture;
        GLuint offsetMapTexture;
        GLuint fbo;
        GLuint fboTexture;
        GLuint VBO;
        GLuint VAO;
        
        AVFormatContext* in_fmt_ctx = nullptr;
        AVCodecContext* in_codec_ctx = nullptr;
        AVFormatContext* out_fmt_ctx = nullptr;
        AVCodecContext* out_codec_ctx = nullptr;
        AVStream* out_stream = nullptr;
        AVFrame* in_frame = nullptr;
        AVPacket* in_packet = nullptr;
        SwsContext* sws_ctx = nullptr;
        SwsContext* out_sws_ctx = nullptr;
        
        int video_stream_index = -1;

        int frame_width;
        int frame_height;
        double fps;
        long framesOveral;
        long frameCount;

        const char* vertexShaderSource;
        const char* fragmentShaderSource;
        std::string inpath;
        std::string outpath;
        std::string seed;

        Video(
            const char* vertexShaderSource,
            const char* fragmentShaderSource,
            const std::string& inpath,
            const std::string& outpath,
            const std::string& seed
        ) : window(nullptr), shaderProgram(0), in_fmt_ctx(nullptr), out_fmt_ctx(nullptr), in_frame(nullptr), in_packet(nullptr), sws_ctx(nullptr), out_sws_ctx(nullptr), frame_width(0), frame_height(0), fps(0.0), framesOveral(0), frameCount(0) {
            this->vertexShaderSource = vertexShaderSource;
            this->fragmentShaderSource = fragmentShaderSource;
            this->inpath = inpath;
            this->outpath = outpath;
            this->seed = seed;
        }

        ~Video() {
            if (in_frame) av_frame_free(&in_frame);
            if (in_packet) av_packet_free(&in_packet);
            if (sws_ctx) sws_freeContext(sws_ctx);
            if (out_sws_ctx) sws_freeContext(out_sws_ctx);
            
            if (in_codec_ctx) avcodec_free_context(&in_codec_ctx);
            if (out_codec_ctx) avcodec_free_context(&out_codec_ctx);

            if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&out_fmt_ctx->pb);
            }
            if (out_fmt_ctx) avformat_free_context(out_fmt_ctx);
            
            if (in_fmt_ctx) avformat_close_input(&in_fmt_ctx);

            glDeleteFramebuffers(1, &fbo);
            glDeleteTextures(1, &fboTexture);
            glDeleteTextures(1, &inputTexture);
            glDeleteTextures(1, &offsetMapTexture);
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteProgram(shaderProgram);
            if (window) {
                glfwDestroyWindow(window);
            }
            glfwTerminate();
        }

        int Start() {
            if (!glfwInit()) {
                std::cerr << "Failed to initialize GLFW" << std::endl;
                return -1;
            }

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

            this->window = glfwCreateWindow(1, 1, "OpenGL Context", NULL, NULL);
            if (!window) {
                std::cerr << "Failed to create GLFW window or OpenGL context" << std::endl;
                glfwTerminate();
                return -1;
            }
            glfwMakeContextCurrent(this->window);

            glewExperimental = GL_TRUE;
            if (glewInit() != GLEW_OK) {
                std::cerr << "Failed to initialize GLEW" << std::endl;
                glfwDestroyWindow(this->window);
                glfwTerminate();
                return -1;
            }

            glViewport(0, 0, 1, 1);

            this->shaderProgram = UnsafeYT::createShaderProgram(this->vertexShaderSource, this->fragmentShaderSource);
            if (this->shaderProgram == 0) {
                glfwDestroyWindow(this->window);
                glfwTerminate();
                return -1;
            }

            float vertices[] = {
                -1.0f,  1.0f,  0.0f, 1.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                1.0f, -1.0f,  1.0f, 0.0f,
                -1.0f,  1.0f,  0.0f, 1.0f,
                1.0f, -1.0f,  1.0f, 0.0f,
                1.0f,  1.0f,  1.0f, 1.0f
            };

            glGenVertexArrays(1, &this->VAO);
            glGenBuffers(1, &this->VBO);
            glBindVertexArray(this->VAO);
            glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            if (avformat_open_input(&in_fmt_ctx, this->inpath.c_str(), NULL, NULL) != 0) {
                std::cerr << "Error: Could not open input video file with FFmpeg." << std::endl;
                return -1;
            }

            if (avformat_find_stream_info(in_fmt_ctx, NULL) < 0) {
                std::cerr << "Error: Could not find stream information." << std::endl;
                return -1;
            }

            for (unsigned int i = 0; i < in_fmt_ctx->nb_streams; i++) {
                if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    video_stream_index = i;
                    break;
                }
            }
            if (video_stream_index == -1) {
                std::cerr << "Error: Could not find a video stream in the input file." << std::endl;
                return -1;
            }

            const AVCodec* in_codec = avcodec_find_decoder(in_fmt_ctx->streams[video_stream_index]->codecpar->codec_id);
            if (!in_codec) {
                std::cerr << "Error: Could not find a suitable decoder for the video stream." << std::endl;
                return -1;
            }

            in_codec_ctx = avcodec_alloc_context3(in_codec);
            if (!in_codec_ctx) {
                std::cerr << "Error: Failed to allocate codec context." << std::endl;
                return -1;
            }
            avcodec_parameters_to_context(in_codec_ctx, in_fmt_ctx->streams[video_stream_index]->codecpar);
            if (avcodec_open2(in_codec_ctx, in_codec, NULL) < 0) {
                std::cerr << "Error: Could not open codec." << std::endl;
                return -1;
            }

            this->frame_width = in_codec_ctx->width;
            this->frame_height = in_codec_ctx->height;
            this->fps = av_q2d(in_fmt_ctx->streams[video_stream_index]->avg_frame_rate);
            this->framesOveral = in_fmt_ctx->streams[video_stream_index]->nb_frames;
            if (this->framesOveral == 0 && in_fmt_ctx->duration != AV_NOPTS_VALUE) {
                this->framesOveral = (long) (in_fmt_ctx->duration * av_q2d(in_fmt_ctx->streams[video_stream_index]->time_base) * this->fps);
            }

            this->in_frame = av_frame_alloc();
            this->in_packet = av_packet_alloc();
            if (!this->in_frame || !this->in_packet) {
                std::cerr << "Error: Failed to allocate frame or packet." << std::endl;
                return -1;
            }

            sws_ctx = sws_getContext(
                this->frame_width, this->frame_height, in_codec_ctx->pix_fmt,
                this->frame_width, this->frame_height, AV_PIX_FMT_RGB24,
                SWS_BILINEAR, NULL, NULL, NULL
            );
            if (!sws_ctx) {
                std::cerr << "Error: Cannot create SwsContext for color conversion." << std::endl;
                return -1;
            }

            int map_width = 80;
            int map_height = 80;
            std::pair<std::vector<float>, std::vector<float>> offset_maps;
            try {
                offset_maps = UnsafeYT::generate_offset_maps(map_width, map_height, this->seed);
            }
            catch (const std::runtime_error& e) {
                std::cerr << "Error generating offset maps: " << e.what() << std::endl;
                return -1;
            }

            bool applyShuffleEffect = true;
            std::vector<float>& current_offset_map_data = applyShuffleEffect ? offset_maps.first : offset_maps.second;

            glGenTextures(1, &this->offsetMapTexture);
            glBindTexture(GL_TEXTURE_2D, this->offsetMapTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, map_width, map_height, 0, GL_RG, GL_FLOAT, current_offset_map_data.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glGenTextures(1, &this->fboTexture);
            glBindTexture(GL_TEXTURE_2D, this->fboTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->frame_width, this->frame_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenFramebuffers(1, &this->fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fboTexture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! Status: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
                return -1;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glGenTextures(1, &this->inputTexture);
            glBindTexture(GL_TEXTURE_2D, this->inputTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            const char* output_filename = this->outpath.c_str();
            const AVOutputFormat* out_fmt = av_guess_format(NULL, output_filename, NULL);
            if (!out_fmt) {
                std::cerr << "Error: Could not guess output format." << std::endl;
                return -1;
            }
            if (avformat_alloc_output_context2(&out_fmt_ctx, out_fmt, NULL, output_filename) < 0) {
                std::cerr << "Error: Could not create output context." << std::endl;
                return -1;
            }
            const AVCodec* out_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
            if (!out_codec) {
                std::cerr << "Error: Could not find H.264 encoder." << std::endl;
                return -1;
            }

            out_stream = avformat_new_stream(out_fmt_ctx, out_codec);
            if (!out_stream) {
                std::cerr << "Error: Failed to create output stream." << std::endl;
                return -1;
            }

            out_codec_ctx = avcodec_alloc_context3(out_codec);
            if (!out_codec_ctx) {
                std::cerr << "Error: Failed to allocate output codec context." << std::endl;
                return -1;
            }

            out_codec_ctx->width = this->frame_width;
            out_codec_ctx->height = this->frame_height;
            out_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; 
            out_codec_ctx->time_base = (AVRational){1, (int)this->fps};
            out_codec_ctx->gop_size = 12;
            out_codec_ctx->max_b_frames = 2;

            if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
                out_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }
            if (avcodec_open2(out_codec_ctx, out_codec, NULL) < 0) {
                std::cerr << "Error: Could not open output codec." << std::endl;
                return -1;
            }
            avcodec_parameters_from_context(out_stream->codecpar, out_codec_ctx);

            if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
                if (avio_open(&out_fmt_ctx->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
                    std::cerr << "Error: Could not open output file '" << output_filename << "'." << std::endl;
                    return -1;
                }
            }
            avformat_write_header(out_fmt_ctx, NULL);

            this->out_sws_ctx = sws_getContext(
                this->frame_width, this->frame_height, AV_PIX_FMT_RGB24,
                out_codec_ctx->width, out_codec_ctx->height, out_codec_ctx->pix_fmt,
                SWS_BILINEAR, NULL, NULL, NULL
            );

            if (!this->out_sws_ctx) {
                std::cerr << "Error: Cannot create SwsContext for output conversion." << std::endl;
                return -1;
            }

            std::cout << "Starting video processing with OpenGL..." << std::endl;
            std::cout << "Applying " << (applyShuffleEffect ? "Shuffle" : "Unshuffle") << " effect." << std::endl;
            std::cout << "Offset map dimensions: " << map_width << "x" << map_height << std::endl;

            this->Process();
            std::cout << "Finished processing. Total frames written: " << this->frameCount << std::endl;
            av_write_trailer(out_fmt_ctx);

            return 0;
        }

        void Process() {
            AVPacket* pkt = av_packet_alloc();
            if (!pkt) {
                std::cerr << "Error: Failed to allocate output packet." << std::endl;
                return;
            }

            AVFrame* out_frame = av_frame_alloc();
            if (!out_frame) {
                std::cerr << "Error: Failed to allocate output frame." << std::endl;
                av_packet_free(&pkt);
                return;
            }

            out_frame->format = out_codec_ctx->pix_fmt;
            out_frame->width = out_codec_ctx->width;
            out_frame->height = out_codec_ctx->height;
            if (av_frame_get_buffer(out_frame, 0) < 0) {
                std::cerr << "Error: Failed to allocate output frame buffers." << std::endl;
                av_frame_free(&out_frame);
                av_packet_free(&pkt);
                return;
            }

            AVFrame* rgb_frame = av_frame_alloc();
            if (!rgb_frame) {
                std::cerr << "Error: Failed to allocate RGB frame." << std::endl;
                av_frame_free(&out_frame);
                av_packet_free(&pkt);
                return;
            }

            rgb_frame->format = AV_PIX_FMT_RGB24;
            rgb_frame->width = this->frame_width;
            rgb_frame->height = this->frame_height;
            if (av_frame_get_buffer(rgb_frame, 0) < 0) {
                std::cerr << "Error: Failed to allocate RGB frame buffers." << std::endl;
                av_frame_free(&out_frame);
                av_frame_free(&rgb_frame);
                av_packet_free(&pkt);
                return;
            }

            AVFrame* processed_rgb_frame = av_frame_alloc();
            if (!processed_rgb_frame) {
                std::cerr << "Error: Failed to allocate processed RGB frame." << std::endl;
                av_frame_free(&out_frame);
                av_frame_free(&rgb_frame);
                av_packet_free(&pkt);
                return;
            }
            
            processed_rgb_frame->format = AV_PIX_FMT_RGB24;
            processed_rgb_frame->width = this->frame_width;
            processed_rgb_frame->height = this->frame_height;
            if (av_frame_get_buffer(processed_rgb_frame, 0) < 0) {
                std::cerr << "Error: Failed to allocate processed RGB frame buffers." << std::endl;
                av_frame_free(&out_frame);
                av_frame_free(&rgb_frame);
                av_frame_free(&processed_rgb_frame);
                av_packet_free(&pkt);
                return;
            }

            while (av_read_frame(in_fmt_ctx, in_packet) >= 0) {
                if (in_packet->stream_index == video_stream_index) {
                    if (avcodec_send_packet(in_codec_ctx, in_packet) >= 0) {
                        while (avcodec_receive_frame(in_codec_ctx, in_frame) >= 0) {
                            sws_scale(sws_ctx, in_frame->data, in_frame->linesize, 0, this->frame_height, rgb_frame->data, rgb_frame->linesize);

                            glBindTexture(GL_TEXTURE_2D, this->inputTexture);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->frame_width, this->frame_height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_frame->data[0]);

                            glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
                            glViewport(0, 0, this->frame_width, this->frame_height);
                            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                            glClear(GL_COLOR_BUFFER_BIT);

                            glUseProgram(this->shaderProgram);
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, this->inputTexture);
                            glUniform1i(glGetUniformLocation(this->shaderProgram, "ourTexture"), 0);

                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, this->offsetMapTexture);
                            glUniform1i(glGetUniformLocation(this->shaderProgram, "offsetMap"), 1);

                            glBindVertexArray(this->VAO);
                            glDrawArrays(GL_TRIANGLES, 0, 6);
                            glBindVertexArray(0);

                            glPixelStorei(GL_PACK_ALIGNMENT, 1);
                            glReadPixels(0, 0, this->frame_width, this->frame_height, GL_RGB, GL_UNSIGNED_BYTE, processed_rgb_frame->data[0]);
                            sws_scale(this->out_sws_ctx, processed_rgb_frame->data, processed_rgb_frame->linesize, 0, this->frame_height, out_frame->data, out_frame->linesize);

                            out_frame->pts = this->frameCount;

                            int ret = avcodec_send_frame(out_codec_ctx, out_frame);
                            if (ret >= 0) {
                                while (avcodec_receive_packet(out_codec_ctx, pkt) >= 0) {
                                    av_packet_rescale_ts(pkt, out_codec_ctx->time_base, out_stream->time_base);
                                    pkt->stream_index = out_stream->index;
                                    av_interleaved_write_frame(out_fmt_ctx, pkt);
                                    av_packet_unref(pkt);
                                }
                            }

                            this->frameCount++;
                            if (this->framesOveral > 0 && this->frameCount % 40 == 0) {
                                std::cout << ((float)this->frameCount / (float)this->framesOveral) * 80.0 << std::endl;
                            }

                            glfwPollEvents();
                            if (glfwWindowShouldClose(this->window)) break;
                        }
                    }
                }
                av_packet_unref(in_packet);
            }

            avcodec_send_frame(out_codec_ctx, NULL);
            while (avcodec_receive_packet(out_codec_ctx, pkt) >= 0) {
                av_packet_rescale_ts(pkt, out_codec_ctx->time_base, out_stream->time_base);
                av_interleaved_write_frame(out_fmt_ctx, pkt);
                av_packet_unref(pkt);
            }

            av_frame_free(&out_frame);
            av_frame_free(&rgb_frame);
            av_frame_free(&processed_rgb_frame);
            av_packet_free(&pkt); 
        }
    };
}

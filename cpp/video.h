namespace UnsafeYT {
    class Video {
    public:
        GLFWwindow* window;
        GLuint shaderProgram;
        GLuint inputTexture;
        GLuint offsetMapTexture;
        GLuint fbo;
        GLuint VBO;
        GLuint VAO;

        cv::VideoWriter output_video;
        //cv::cuda::GpuMat frame;
        cv::Mat frame;
        cv::VideoCapture cap;

        int frame_width;
        int frame_height;
        const char* vertexShaderSource;
        const char* fragmentShaderSource;
        long frameCount;
        long framesOveral;

        std::string inpath;
        std::string outpath;
        std::string seed;

        Video(
            const char* vertexShaderSource,
            const char* fragmentShaderSource,
            const std::string& inpath,
            const std::string& outpath,
            const std::string& seed
        ) {
            this->vertexShaderSource = vertexShaderSource;
            this->fragmentShaderSource = fragmentShaderSource;
            this->window = nullptr;
            this->shaderProgram = 0;
            this->frame_width = 0;
            this->frame_height = 0;
            this->frameCount = 0;
            this->inpath = inpath;
            this->outpath = outpath;
            this->seed = seed;
            this->framesOveral = 0;
        }

        int Start() {
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

            this->cap = cv::VideoCapture(this->inpath);
            this->framesOveral = static_cast<long>(this->cap.get(cv::CAP_PROP_FRAME_COUNT));

            if (!this->cap.isOpened()) {
                std::cerr << "Error: Could not open video file." << std::endl;
                glfwDestroyWindow(this->window);
                glfwTerminate();
                return -1;
            }

            this->frame_width = static_cast<int>(this->cap.get(cv::CAP_PROP_FRAME_WIDTH));
            this->frame_height = static_cast<int>(this->cap.get(cv::CAP_PROP_FRAME_HEIGHT));
            double fps = this->cap.get(cv::CAP_PROP_FPS);

            if (this->frame_width == 0 || this->frame_height == 0) {
                std::cerr << "Error: Could not get valid video frame dimensions." << std::endl;
                this->cap.release();
                glfwDestroyWindow(this->window);
                glfwTerminate();
                return -1;
            }

            int map_width = 80;
            int map_height = 80;
            if (map_width == 0) map_width = 1;
            if (map_height == 0) map_height = 1;

            std::pair<std::vector<float>, std::vector<float>> offset_maps;
            try {
                offset_maps = UnsafeYT::generate_offset_maps(map_width, map_height, this->seed);
            }
            catch (const std::runtime_error& e) {
                std::cerr << "Error generating offset maps: " << e.what() << std::endl;
                this->cap.release();
                glfwDestroyWindow(this->window);
                glfwTerminate();
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

            glGenFramebuffers(1, &this->fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);

            GLuint fboTexture;
            glGenTextures(1, &fboTexture);
            glBindTexture(GL_TEXTURE_2D, fboTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->frame_width, this->frame_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! Status: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
                glDeleteFramebuffers(1, &this->fbo);
                glDeleteTextures(1, &fboTexture);
                glDeleteTextures(1, &this->offsetMapTexture);
                glfwDestroyWindow(this->window);
                glfwTerminate();
                return -1;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glGenTextures(1, &this->inputTexture);
            glBindTexture(GL_TEXTURE_2D, this->inputTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
            this->output_video = cv::VideoWriter(this->outpath, fourcc, fps, cv::Size(this->frame_width, this->frame_height), true);

            if (!this->output_video.isOpened()) {
                std::cerr << "Error: Could not open the output video file for writing." << std::endl;
                this->cap.release();
                glDeleteFramebuffers(1, &this->fbo);
                glDeleteTextures(1, &fboTexture);
                glDeleteTextures(1, &this->offsetMapTexture);
                glDeleteTextures(1, &this->inputTexture);
                glDeleteVertexArrays(1, &this->VAO);
                glDeleteBuffers(1, &this->VBO);
                glDeleteProgram(this->shaderProgram);
                glfwDestroyWindow(this->window);
                glfwTerminate();
                return -1;
            }

            std::cout << "Starting video processing with OpenGL..." << std::endl;
            std::cout << "Applying " << (applyShuffleEffect ? "Shuffle" : "Unshuffle") << " effect." << std::endl;
            std::cout << "Offset map dimensions: " << map_width << "x" << map_height << std::endl;

            this->Process(fboTexture);
            
            std::cout << "Finished processing. Total frames written: " << this->frameCount << std::endl;

            this->cap.release();
            this->output_video.release();
            glDeleteFramebuffers(1, &this->fbo);
            glDeleteTextures(1, &fboTexture);
            glDeleteTextures(1, &this->offsetMapTexture);
            glDeleteTextures(1, &this->inputTexture);
            glDeleteVertexArrays(1, &this->VAO);
            glDeleteBuffers(1, &this->VBO);
            glDeleteProgram(this->shaderProgram);
            glfwDestroyWindow(this->window);
            glfwTerminate();

            return 0;
        }

        void Process(GLuint fboTexture) {
            cv::Mat processed_frame_cpu(this->frame_height, this->frame_width, CV_8UC3);

            while (this->cap.read(this->frame)) {
                if (this->frame.empty()) {
                    break;
                }

                cv::cvtColor(this->frame, this->frame, cv::COLOR_BGR2RGB);

                glBindTexture(GL_TEXTURE_2D, this->inputTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.cols, frame.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, this->frame.data);

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

                glReadPixels(0, 0, this->frame_width, this->frame_height, GL_RGB, GL_UNSIGNED_BYTE, processed_frame_cpu.data);

                cv::cvtColor(processed_frame_cpu, processed_frame_cpu, cv::COLOR_RGB2BGR);

                this->output_video.write(processed_frame_cpu);
                this->frameCount++;

                if (this->frameCount % 40 == 0)
                    std::cout << ((float)this->frameCount / (float)this->framesOveral) * 80.0 << std::endl;

                glfwPollEvents();

                if (glfwWindowShouldClose(this->window))
                    break;
            }
        }
    };
}

namespace UnsafeYT{
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \n\r\t");
        if (std::string::npos == first) {
            return str;
        }
        size_t last = str.find_last_not_of(" \n\r\t");
        return str.substr(first, (last - first + 1));
    }

    std::string exec(const std::string& cmd) {
        std::array<char, 128> buffer;
        std::string result;


        FILE* pipe = popen(cmd.c_str(), "r");

        if (!pipe) {
            throw std::runtime_error("Failed to open pipe for command: " + cmd);
        }

        try {
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }
        } catch (...) {
            pclose(pipe);

            throw;
        }

        int status = 0;
        status = pclose(pipe);

        if (status != 0) {
            std::string error_message = "Command exited with non-zero status: " + std::to_string(status);
            error_message += "\nCommand: " + cmd;
            error_message += "\nChild process stderr:\n" + result;
            throw std::runtime_error(error_message);
        }
        return result;
    }


    void mixAudioAndAddSine(
        std::string input_video_path,
        std::string source_audio_video_path,
        std::string output_video_path,
        double frequency = 600.0,
        double amplitude = 0.1 
    ) {
        if (!std::filesystem::exists(input_video_path)) {
            std::cerr << "Error: Input video file '" << input_video_path << "' does not exist." << std::endl;
            return;
        }
        if (!std::filesystem::exists(source_audio_video_path)) {
            std::cerr << "Error: Source audio video file '" << source_audio_video_path << "' does not exist." << std::endl;
            return;
        }

        try {
            std::string cmd_duration = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"" + source_audio_video_path + "\"";
            std::string duration_str = exec(cmd_duration);
            double duration = 0.0;
            try {
                duration = std::stod(trim(duration_str));
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not parse duration from '" << duration_str << "'. Defaulting to 0.0. Error: " << e.what() << std::endl;
                duration = 0.0;
            }
            
            if (duration == 0.0) {
                throw std::runtime_error("Could not determine duration of source audio video. Is the file valid?");
            }

            std::string cmd_samplerate = "ffprobe -v error -select_streams a:0 -show_entries stream=sample_rate -of default=noprint_wrappers=1:nokey=1 \"" + source_audio_video_path + "\"";
            std::string samplerate_str = exec(cmd_samplerate);
            int sample_rate = 44100;
            bool has_source_audio = false;

            try {
                sample_rate = std::stoi(trim(samplerate_str));
                has_source_audio = true;
            } catch (const std::exception& e) {
                std::cerr << "Info: Source audio video appears to have no audio stream or sample rate could not be determined. Error: " << e.what() << std::endl;
                sample_rate = 44100;
                has_source_audio = false;
            }
            
            double freq1 = frequency + 6000;
            double freq2 = frequency + 15000;

            std::ostringstream ffmpeg_cmd_stream;

            ffmpeg_cmd_stream << "ffmpeg -i \"" << input_video_path << "\" ";
            ffmpeg_cmd_stream << "-i \"" << source_audio_video_path << "\" ";
            ffmpeg_cmd_stream << "-f lavfi -i \"sine=frequency=" << freq1 << ":r=" << sample_rate << ":d=" << duration << "\" ";
            ffmpeg_cmd_stream << "-f lavfi -i \"sine=frequency=" << freq2 << ":r=" << sample_rate << ":d=" << duration << "\" ";
            ffmpeg_cmd_stream << "-filter_complex \"";

            if (has_source_audio) {
                ffmpeg_cmd_stream << "[1:a]volume=0.03[source_audio_scaled];"; 
                ffmpeg_cmd_stream << "[2:a]volume=" << amplitude << "[sine1_vol];";
                ffmpeg_cmd_stream << "[3:a]volume=" << amplitude << "[sine2_vol];";
                ffmpeg_cmd_stream << "[sine1_vol][sine2_vol]amix=inputs=2:duration=longest:weights=1 1[sine_sum];";
                ffmpeg_cmd_stream << "[source_audio_scaled][sine_sum]amix=inputs=2:duration=longest:normalize=0[final_audio]";
            } else {
                ffmpeg_cmd_stream << "[1:a]volume=" << amplitude << "[sine1_vol];";
                ffmpeg_cmd_stream << "[2:a]volume=" << amplitude << "[sine2_vol];";
                ffmpeg_cmd_stream << "[sine1_vol][sine2_vol]amix=inputs=2:duration=longest:weights=1 1[final_audio]";
            }

            ffmpeg_cmd_stream << "\" ";
            ffmpeg_cmd_stream << "-map 0:v ";
            ffmpeg_cmd_stream << "-map \"[final_audio]\" ";
            ffmpeg_cmd_stream << "-c:v copy -c:a aac -shortest -y \""
                              << output_video_path << "\"";

            std::string ffmpeg_command = ffmpeg_cmd_stream.str();
            
            std::cout << "Executing FFmpeg command:\n" << ffmpeg_command << std::endl;
            exec(ffmpeg_command);

            std::cout << 100 << std::endl;
        } catch (const std::runtime_error& e) {
            std::cerr << "FFmpeg Error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        }
    }
}
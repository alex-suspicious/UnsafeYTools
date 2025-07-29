namespace UnsafeYT{
    std::pair<std::vector<float>, std::vector<float>> generate_offset_maps(int map_width, int map_height, const std::string& seed) {
        if (map_width <= 0 || map_height <= 0) {
            throw std::runtime_error("Map width and height must be positive integers.");
        }
        if (seed.empty()) {
            throw std::runtime_error("Seed string is required for deterministic generation.");
        }
    
        size_t total_pixels = static_cast<size_t>(map_width) * map_height;
    
        double start_hash = UnsafeYT::deterministic_hash(seed, 31, std::numeric_limits<unsigned int>::max());
        double step_hash = UnsafeYT::deterministic_hash(seed + "_step", 37, std::numeric_limits<unsigned int>::max() - 1);
    
        double start_angle = start_hash * M_PI * 2.0;
        double angle_increment = step_hash * M_PI / std::max(static_cast<double>(map_width), static_cast<double>(map_height));
    
        std::vector<std::pair<double, size_t>> indexed_values(total_pixels);
        for (size_t i = 0; i < total_pixels; ++i) {
            double value = std::sin(start_angle + i * angle_increment);
            indexed_values[i] = {value, i};
        }
    
        std::sort(indexed_values.begin(), indexed_values.end());
    
        std::vector<size_t> pLinearized(total_pixels);
        for (size_t k = 0; k < total_pixels; ++k) {
            size_t original_index = indexed_values[k].second;
            size_t shuffled_index = k;
            pLinearized[original_index] = shuffled_index;
        }
    
        std::vector<size_t> pInvLinearized(total_pixels);
        for (size_t original_idx = 0; original_idx < total_pixels; ++original_idx) {
            size_t shuffled_idx = pLinearized[original_idx];
            pInvLinearized[shuffled_idx] = original_idx;
        }
    
        std::vector<float> unshuffle_map_data(total_pixels * 2);
        for (int oy = 0; oy < map_height; ++oy) {
            for (int ox = 0; ox < map_width; ++ox) {
                size_t original_linear_index = oy * map_width + ox;
                size_t shuffled_linear_index = pLinearized[original_linear_index];
    
                int sx_shuffled = shuffled_linear_index % map_width;
                int sy_shuffled = shuffled_linear_index / map_width;
    
                double offset_x = static_cast<double>(sx_shuffled - ox) / static_cast<double>(map_width);
                double offset_y = static_cast<double>(sy_shuffled - oy) / static_cast<double>(map_height);
    
                size_t index = (oy * map_width + ox) * 2;
                unshuffle_map_data[index] = static_cast<float>(offset_x);
                unshuffle_map_data[index + 1] = static_cast<float>(offset_y);
            }
        }
    
        std::vector<float> shuffle_map_data(total_pixels * 2);
        for (int ty_shuffled = 0; ty_shuffled < map_height; ++ty_shuffled) {
            for (int tx_shuffled = 0; tx_shuffled < map_width; ++tx_shuffled) {
                size_t shuffled_linear_index = ty_shuffled * map_width + tx_shuffled;
                size_t original_linear_index = pInvLinearized[shuffled_linear_index];
    
                int ox_original = original_linear_index % map_width;
                int oy_original = original_linear_index / map_width;
    
                double offset_x = static_cast<double>(ox_original - tx_shuffled) / static_cast<double>(map_width);
                double offset_y = static_cast<double>(oy_original - ty_shuffled) / static_cast<double>(map_height);
    
                size_t index = (ty_shuffled * map_width + tx_shuffled) * 2;
                shuffle_map_data[index] = static_cast<float>(offset_x);
                shuffle_map_data[index + 1] = static_cast<float>(offset_y);
            }
        }
    
        return {shuffle_map_data, unshuffle_map_data};
    }    
}
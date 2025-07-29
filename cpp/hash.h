namespace UnsafeYT{
    double deterministic_hash(const std::string& s, int prime, unsigned int modulus) {
        unsigned int h = 0;
        for (char char_val : s) {
            h = (static_cast<unsigned long long>(h) * prime + static_cast<unsigned int>(char_val)) % modulus;
        }
        return static_cast<double>(h) / modulus;
    }
}
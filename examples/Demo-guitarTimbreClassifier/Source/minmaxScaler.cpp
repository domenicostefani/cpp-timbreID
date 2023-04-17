#include <vector>
#include <stdexcept>


void minmax01_inline_scale(float* data, const float* orig_min, const float* orig_scale, size_t size) {
    for (int i = 0; i < size; i++) {
        data[i] = (data[i] - orig_min[i]) * orig_scale[i];
    }
}

void minmax01_inline_scale(std::vector<float> &data, const std::vector<float> &orig_min, const std::vector<float> &orig_scale) {
    if (data.size() != orig_min.size() || data.size() != orig_scale.size()) {
        throw std::runtime_error("minmax01_inline_scale: data and orig_min and orig_scale must have the same size");
    }
    minmax01_inline_scale(data.data(), orig_min.data(), orig_scale.data(), data.size());
}
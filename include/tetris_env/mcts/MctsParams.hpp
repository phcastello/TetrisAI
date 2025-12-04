#pragma once

#include <cstdint>
#include <optional>

struct MctsParams {
    int iterations = 0;
    int maxDepth = 0;
    double exploration = 0.0;
    int threads = 1;
    std::optional<std::uint32_t> seed{};
    std::optional<int> scoreLimit{};
    std::optional<double> timeLimitSeconds{};
};

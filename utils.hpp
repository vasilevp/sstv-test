#pragma once
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <array>

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define DEFER(x) std::shared_ptr<void> CONCAT(_defer___, __COUNTER__)(nullptr, [&](auto) { x; });

namespace utils
{
    static constexpr auto lut = std::to_array<int8_t>({
#include "sine.txt"
    });

};
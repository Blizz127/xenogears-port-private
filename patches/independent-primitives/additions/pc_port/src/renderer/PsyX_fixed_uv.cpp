#include "PsyX_fixed_uv.h"

#include <algorithm>
#include <cstdint>

namespace {
using Wide = std::int64_t;
constexpr Wide unit = 65536;

// Model the observed 32-bit fixed-coordinate wrap using defined C++ arithmetic.
Wide signedWord(Wide bits) {
    constexpr Wide period = Wide{1} << 32;
    const auto low = static_cast<std::uint32_t>(bits);
    return low <= INT32_MAX ? Wide(low) : Wide(low) - period;
}

struct Side {
    const XenoFixedUvVertex& top;
    const XenoFixedUvVertex& bottom;

    Wide height() const { return bottom.y - top.y; }

    Wide strideX() const { return signedWord((Wide(bottom.x) - top.x) * unit); }

    Wide positionX(int y) const {
        return signedWord(top.x * unit + strideX() * (y - top.y) / height());
    }

    // The attribute increment is quantized once per edge, before advancing to y.
    Wide attribute(int value, int endValue, int y) const {
        const Wide stride = (endValue - value) * unit / height();
        return value * unit + stride * (y - top.y);
    }
};

Wide ceilPixel(Wide x) {
    return x / unit + (x % unit > 0);
}

Wide roundDownProduct(Wide distance, Wide gradient) {
    const Wide product = distance * gradient;
    return product / unit - (product % unit < 0);
}

Wide scanGradient(int top, int middle, int bottom, Wide fraction, Wide width) {
    const Wide delta = (middle - top) * unit - (bottom - top) * fraction;
    const Wide minWidth = unit / 16;
    const Wide divisor = width < 0 ? std::min(width, -minWidth) : std::max(width, minWidth);
    return delta * unit / divisor;
}
}

bool XenoBuildFixedUvRows(const XenoFixedUvVertex vertices[3], int width, int height,
                          XenoFixedUvRow* rows) {
    if (!vertices || !rows || width <= 0 || width > 1024 || height <= 0 || height > 512)
        return false;

    std::fill_n(rows, height, XenoFixedUvRow{});
    const XenoFixedUvVertex* order[] = {vertices, vertices + 1, vertices + 2};
    auto byY = [](const XenoFixedUvVertex* a, const XenoFixedUvVertex* b) {
        return a->y < b->y;
    };
    // Retain the input-order tie behavior visible at the signed-width boundary.
    std::iter_swap(order, std::min_element(order, order + 3, byY));
    if (byY(order[2], order[1]))
        std::swap(order[1], order[2]);
    const auto& top = *order[0];
    const auto& middle = *order[1];
    const auto& bottom = *order[2];
    const Side whole{top, bottom};
    if (!whole.height())
        return false;

    const Wide fraction = (middle.y - top.y) * unit / whole.height();
    // Measure from the middle vertex to the opposite edge before assigning sides.
    const Wide across = whole.strideX() / unit * fraction - (middle.x - top.x) * unit;
    const Wide separation = -signedWord(across);
    if (separation == 0)
        return false;

    const Wide du = scanGradient(top.u, middle.u, bottom.u, fraction, separation);
    const Wide dv = scanGradient(top.v, middle.v, bottom.v, fraction, separation);
    const int start = std::max<int>(0, top.y);
    const int end = std::min<int>(height, bottom.y);
    for (int y = start; y < end; ++y) {
        const Side piece = y < middle.y ? Side{top, middle} : Side{middle, bottom};
        const Side& left = separation < 0 ? piece : whole;
        const Side& right = separation < 0 ? whole : piece;
        const Wide xl = left.positionX(y);
        const Wide first = std::max<Wide>(0, ceilPixel(xl));
        const Wide stop = std::min<Wide>(width, ceilPixel(right.positionX(y)));
        if (first >= stop)
            continue;

        const Wide offset = first * unit - xl;
        const Wide u = left.attribute(left.top.u, left.bottom.u, y);
        const Wide v = left.attribute(left.top.v, left.bottom.v, y);
        rows[y] = {static_cast<int32_t>(signedWord(u + unit / 2 + roundDownProduct(offset, du))),
                   static_cast<int32_t>(signedWord(v + unit / 2 + roundDownProduct(offset, dv))),
                   static_cast<int32_t>(du), static_cast<int32_t>(dv),
                   static_cast<int32_t>(first), 1, static_cast<int32_t>(stop - 1), 0};
    }
    return true;
}

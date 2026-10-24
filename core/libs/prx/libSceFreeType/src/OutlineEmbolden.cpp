#include "prx/libSceFreeType/include/GuestFreeType.hpp"
#include "prx/libc/include/General.hpp"

#include <ft2build.h>
#include FT_OUTLINE_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

using namespace FreeType;

extern "C" int APS5_VABI FT_Outline_Get_Orientation_nid_postfix(GuestOutline*);

namespace {

int Embolden(GuestOutline* outline, std::int64_t xStrength, std::int64_t yStrength, const char* function) {
    if (!outline) return FT_Err_Invalid_Outline;
    if (xStrength / 2 == 0 && yStrength / 2 == 0) return 0;
    const int orientation = FT_Outline_Get_Orientation_nid_postfix(outline);
    if (orientation == FT_ORIENTATION_NONE) return outline->n_contours ? FT_Err_Invalid_Argument : 0;
    if (outline->n_contours == 0) return 0;
    if (xStrength < std::numeric_limits<FT_Pos>::min() || xStrength > std::numeric_limits<FT_Pos>::max() ||
        yStrength < std::numeric_limits<FT_Pos>::min() || yStrength > std::numeric_limits<FT_Pos>::max() ||
        outline->n_points > std::numeric_limits<decltype(FT_Outline::n_points)>::max() ||
        outline->n_contours > std::numeric_limits<decltype(FT_Outline::n_contours)>::max()) {
        NotImplemented_nid_no_patch(function);
    }
    std::vector<FT_Vector> points;
    points.reserve(outline->n_points);
    for (std::size_t index = 0; index < outline->n_points; ++index) {
        const auto point = outline->points[index];
        if (point.x < std::numeric_limits<FT_Pos>::min() || point.x > std::numeric_limits<FT_Pos>::max() ||
            point.y < std::numeric_limits<FT_Pos>::min() || point.y > std::numeric_limits<FT_Pos>::max()) {
            NotImplemented_nid_no_patch(function);
        }
        points.push_back({static_cast<FT_Pos>(point.x), static_cast<FT_Pos>(point.y)});
    }
    FT_Outline native{};
    native.n_points = static_cast<decltype(native.n_points)>(outline->n_points);
    native.n_contours = static_cast<decltype(native.n_contours)>(outline->n_contours);
    native.points = points.data();
    native.tags = reinterpret_cast<decltype(native.tags)>(outline->tags);
    native.contours = reinterpret_cast<decltype(native.contours)>(outline->contours);
    native.flags = outline->flags;
    if (FT_Outline_Get_Orientation(&native) != orientation) NotImplemented_nid_no_patch(function);
    const int error = FT_Outline_EmboldenXY(&native, static_cast<FT_Pos>(xStrength), static_cast<FT_Pos>(yStrength));
    if (!error) {
        for (std::size_t index = 0; index < points.size(); ++index) outline->points[index] = {points[index].x, points[index].y};
    }
    return error;
}

}

extern "C" {

int APS5_VABI FT_Outline_Embolden_nid_postfix(GuestOutline* outline, std::int64_t strength) {
    return Embolden(outline, strength, strength, __func__);
}

int APS5_VABI FT_Outline_EmboldenXY_nid_postfix(GuestOutline* outline, std::int64_t xStrength, std::int64_t yStrength) {
    return Embolden(outline, xStrength, yStrength, __func__);
}

}

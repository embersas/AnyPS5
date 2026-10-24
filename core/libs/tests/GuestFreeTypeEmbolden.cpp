#include "FreeTypeFontFixture.hpp"
#include "prx/libSceFreeType/include/GuestObjects.hpp"

#include FT_OUTLINE_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using namespace FreeType;

extern "C" {
int APS5_VABI FT_Outline_Embolden_nid_postfix(GuestOutline*, std::int64_t);
int APS5_VABI FT_Outline_EmboldenXY_nid_postfix(GuestOutline*, std::int64_t, std::int64_t);
int APS5_VABI FT_Render_Glyph_nid_postfix(GuestGlyphSlotRec*, std::int32_t);
}

using EmboldenFunction = int (APS5_VABI*)(GuestOutline*, std::int64_t);
using EmboldenXYFunction = int (APS5_VABI*)(GuestOutline*, std::int64_t, std::int64_t);
static EmboldenFunction embolden = FT_Outline_Embolden_nid_postfix;
static EmboldenXYFunction emboldenXY = FT_Outline_EmboldenXY_nid_postfix;

static void Check(bool value, int line) {
    if (!value) {
        std::fprintf(stderr, "FreeType embolden check failed at line %d\n", line);
        std::abort();
    }
}
#define Require(value) Check((value), __LINE__)

static void Oracle(std::vector<FT_Vector> points, std::vector<unsigned short> contours,
                   FT_Pos xStrength, FT_Pos yStrength, bool uniform) {
    std::vector<GuestVector> storage(points.size() + 2, {991, -992});
    std::vector<unsigned char> tags(points.size(), FT_CURVE_TAG_ON);
    for (std::size_t index = 0; index < points.size(); ++index) storage[index + 1] = {points[index].x, points[index].y};
    const auto originalTags = tags;
    const auto originalContours = contours;
    GuestOutline guest{static_cast<std::uint16_t>(contours.size()), static_cast<std::uint16_t>(points.size()),
                       storage.data() + 1, tags.data(), contours.data(), FT_OUTLINE_EVEN_ODD_FILL};
    const auto record = guest;
    FT_Outline native{guest.n_contours, guest.n_points, points.data(), tags.data(), contours.data(), guest.flags};
    const int expected = uniform ? FT_Outline_Embolden(&native, xStrength) : FT_Outline_EmboldenXY(&native, xStrength, yStrength);
    const int actual = uniform ? embolden(&guest, xStrength) : emboldenXY(&guest, xStrength, yStrength);
    Require(actual == expected);
    Require(guest.n_contours == record.n_contours && guest.n_points == record.n_points && guest.flags == record.flags);
    Require(guest.points == record.points && guest.tags == record.tags && guest.contours == record.contours);
    Require(tags == originalTags && contours == originalContours);
    for (std::size_t index = 0; index < points.size(); ++index) {
        Require(storage[index + 1].x == points[index].x && storage[index + 1].y == points[index].y);
    }
    Require(storage.front().x == 991 && storage.front().y == -992 && storage.back().x == 991 && storage.back().y == -992);
}

static void Render() {
    const auto bytes = SquareGlyphFont('A', 700);
    GuestLibrary library{};
    FT_Library nativeLibrary = nullptr;
    Require(FT_Init_FreeType(&library.host) == 0 && FT_Init_FreeType(&nativeLibrary) == 0);
    {
        GuestFace face;
        face.library = &library;
        FT_Face native = nullptr;
        Require(FT_New_Memory_Face(library.host, bytes.data(), static_cast<FT_Long>(bytes.size()), 0, &face.host) == 0);
        Require(FT_New_Memory_Face(nativeLibrary, bytes.data(), static_cast<FT_Long>(bytes.size()), 0, &native) == 0);
        Require(FT_Set_Pixel_Sizes(face.host, 18, 18) == 0 && FT_Set_Pixel_Sizes(native, 18, 18) == 0);
        Require(FT_Load_Glyph(face.host, 1, FT_LOAD_DEFAULT) == 0 && FT_Load_Glyph(native, 1, FT_LOAD_DEFAULT) == 0);
        SyncFace(face);
        Require(emboldenXY(&face.slot.outline, 64, 96) == FT_Outline_EmboldenXY(&native->glyph->outline, 64, 96));
        Require(FT_Render_Glyph_nid_postfix(&face.slot, FT_RENDER_MODE_NORMAL) == FT_Render_Glyph(native->glyph, FT_RENDER_MODE_NORMAL));
        const auto& actual = face.slot.bitmap;
        const auto& expected = native->glyph->bitmap;
        Require(actual.width > 0 && actual.rows > 0 && actual.width == expected.width && actual.rows == expected.rows);
        Require(actual.pitch == expected.pitch && actual.pixel_mode == expected.pixel_mode);
        Require(face.slot.bitmap_left == native->glyph->bitmap_left && face.slot.bitmap_top == native->glyph->bitmap_top);
        Require(std::memcmp(actual.buffer, expected.buffer, static_cast<std::size_t>(expected.pitch) * expected.rows) == 0);
        Require(FT_Done_Face(face.host) == 0 && FT_Done_Face(native) == 0);
    }
    Require(FT_Done_FreeType(library.host) == 0 && FT_Done_FreeType(nativeLibrary) == 0);
}

static void WideInputs() {
    const std::int64_t wide = std::int64_t{1} << 40;
    std::array<GuestVector, 4> points = {{{wide, 0}, {wide + 640, 0}, {wide + 640, 640}, {wide, 640}}};
    const auto original = points;
    unsigned short contour = 3;
    GuestOutline outline{1, 4, points.data(), nullptr, &contour, 0};
    Require(embolden(&outline, 0) == 0);
    Require(emboldenXY(&outline, 1, -1) == 0);
    Require(embolden(&outline, 64) == FT_Err_Invalid_Argument);
    for (std::size_t index = 0; index < points.size(); ++index) Require(points[index].x == original[index].x && points[index].y == original[index].y);
    if constexpr (sizeof(FT_Pos) < sizeof(std::int64_t)) {
        points = {{{0, 0}, {640, 0}, {640, 640}, {0, 640}}};
        const auto before = points;
        bool threw = false;
        try {
            emboldenXY(&outline, wide, 64);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw);
        for (std::size_t index = 0; index < points.size(); ++index) Require(points[index].x == before[index].x && points[index].y == before[index].y);
    }
}

static void NativeOrientationLimit() {
    if constexpr (sizeof(FT_Pos) >= sizeof(std::int64_t)) return;
    std::vector<GuestVector> points;
    std::vector<unsigned short> contours;
    for (unsigned index = 0; index < 3000; ++index) {
        points.insert(points.end(), {{0, 0}, {640, 0}, {640, 640}, {0, 640}});
        contours.push_back(static_cast<unsigned short>(points.size() - 1));
    }
    const auto before = points;
    GuestOutline outline{static_cast<std::uint16_t>(contours.size()), static_cast<std::uint16_t>(points.size()),
                         points.data(), nullptr, contours.data(), 0};
    bool threw = false;
    try {
        embolden(&outline, 64);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);
    for (std::size_t index = 0; index < points.size(); ++index) Require(points[index].x == before[index].x && points[index].y == before[index].y);
}

static int Run() {
    Require(embolden(nullptr, 0) == FT_Outline_Embolden(nullptr, 0));
    Require(emboldenXY(nullptr, 64, -64) == FT_Outline_EmboldenXY(nullptr, 64, -64));
    const std::array<FT_Pos, 9> strengths = {-128, -65, -1, 0, 1, 2, 63, 64, 129};
    const std::vector<std::vector<FT_Vector>> shapes = {
        {{0, 0}, {640, 0}, {640, 640}, {0, 640}},
        {{0, 0}, {0, 640}, {640, 640}, {640, 0}},
        {{0, 0}, {640, 0}, {320, 640}},
        {{0, 0}, {0, 0}, {640, 0}, {640, 640}, {0, 640}},
        {{0, 0}, {320, 0}, {640, 0}},
        {{0, 0}},
    };
    for (const auto& points : shapes) {
        for (auto x : strengths) {
            Oracle(points, {static_cast<unsigned short>(points.size() - 1)}, x, x, true);
            for (auto y : strengths) Oracle(points, {static_cast<unsigned short>(points.size() - 1)}, x, y, false);
        }
    }
    for (auto x : strengths) {
        Oracle({}, {}, x, x, true);
        Oracle({{0, 0}, {640, 0}, {640, 640}, {0, 640}, {128, 128}, {128, 512}, {512, 512}, {512, 128}}, {3, 7}, x, 64, false);
    }
    WideInputs();
    NativeOrientationLimit();
    Render();
    std::puts("FreeType embolden: 558 native outline comparisons, null and wide inputs, orientation overflow refusal and rendered pixels passed");
    return 0;
}

int main(int argc, char** argv) {
    try {
        if (argc == 3) {
            const bool patched = std::strcmp(argv[2], "patched") == 0;
            const char* uniform = patched ? "l6Cf9rb9fZQ" : "sceFreeTypeOutlineEmbolden_nid_no_patch";
            const char* separate = patched ? "BgUMBGvhATU" : "sceFreeTypeOutlineEmboldenXY_nid_no_patch";
#ifdef _WIN32
            const auto module = LoadLibraryA(argv[1]);
            Require(module != nullptr);
            const auto uniformAddress = GetProcAddress(module, uniform);
            const auto separateAddress = GetProcAddress(module, separate);
#else
            const auto module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
            Require(module != nullptr);
            const auto uniformAddress = dlsym(module, uniform);
            const auto separateAddress = dlsym(module, separate);
#endif
            Require(uniformAddress != nullptr && separateAddress != nullptr);
            static_assert(sizeof(embolden) == sizeof(uniformAddress) && sizeof(emboldenXY) == sizeof(separateAddress));
            std::memcpy(&embolden, &uniformAddress, sizeof(embolden));
            std::memcpy(&emboldenXY, &separateAddress, sizeof(emboldenXY));
        } else {
            Require(argc == 1);
        }
        return Run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FreeType embolden: %s\n", error.what());
        return 1;
    }
}

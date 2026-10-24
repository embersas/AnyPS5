#include "prx/libSceFreeType/include/GuestFreeType.hpp"
#include "prx/libc/include/General.hpp"

using FreeType::GuestOutline;

extern "C" {

int APS5_VABI FreeTypeOutlineEmbolden_nid_no_patch(GuestOutline*, std::int64_t, std::int64_t, const char*);

int APS5_VABI sceFreeTypeOutlineEmbolden_nid_no_patch(GuestOutline* outline, std::int64_t strength) {
    return FreeTypeOutlineEmbolden_nid_no_patch(outline, strength, strength, __func__);
}
APS5_EXPORT("l6Cf9rb9fZQ", sceFreeTypeOutlineEmbolden_nid_no_patch);

int APS5_VABI sceFreeTypeOutlineEmboldenXY_nid_no_patch(GuestOutline* outline, std::int64_t xStrength, std::int64_t yStrength) {
    return FreeTypeOutlineEmbolden_nid_no_patch(outline, xStrength, yStrength, __func__);
}
APS5_EXPORT("BgUMBGvhATU", sceFreeTypeOutlineEmboldenXY_nid_no_patch);

}

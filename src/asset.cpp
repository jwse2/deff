#include "asset.hpp"

Asset::Asset(void)
{
    // By default the asset has nothing to initialize.
}

Asset::~Asset(void)
{
    Release();
}

void Asset::Release(void) noexcept
{
    // By default the asset has nothing to release.
}

const char *lpAssetType[0x21] = {
    "xmodelpieces",
    "physpreset",
    "xanim",
    "xmodel",
    "material",
    "techset",
    "image",
    "sound",
    "sndcurve",
    "loaded_sound",
    "col_map_sp",
    "col_map_mp",
    "com_map",
    "game_map_sp",
    "game_map_mp",
    "map_ents",
    "gfx_map",
    "lightdef",
    "ui_map",
    "font",
    "menufile",
    "menu",
    "localize",
    "weapon",
    "snddriverglobals",
    "fx",
    "impactfx",
    "aitype",
    "mptype",
    "character",
    "xmodelalias",
    "rawfile",
    "stringtable"
};

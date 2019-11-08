#ifndef ASSET_HPP
#define ASSET_HPP

#include "utility.hpp"
#include "stream.hpp"
#include "fastfile.hpp"

#define ASSET_TYPE_XMODELPIECES         0           /* NONE */
#define ASSET_TYPE_PHYSPRESET           1
#define ASSET_TYPE_XANIM                2
#define ASSET_TYPE_XMODEL               3
#define ASSET_TYPE_MATERIAL             4
#define ASSET_TYPE_TECHSET              5
#define ASSET_TYPE_IMAGE                6
#define ASSET_TYPE_SOUND                7
#define ASSET_TYPE_SNDCURVE             8
#define ASSET_TYPE_LOADED_SOUND         9           /* NONE */
#define ASSET_TYPE_COL_MAP_SP           0x0A
#define ASSET_TYPE_COL_MAP_MP           0x0B
#define ASSET_TYPE_COM_MAP              0x0C        /* SP & MP */
#define ASSET_TYPE_GAME_MAP_SP          0x0D
#define ASSET_TYPE_GAME_MAP_MP          0x0E
#define ASSET_TYPE_MAP_ENTS             0x0F        /* NONE */
#define ASSET_TYPE_GFX_MAP              0x10        /* SP & MP */
#define ASSET_TYPE_LIGHTDEF             0x11
#define ASSET_TYPE_UI_MAP               0x12        /* NONE */
#define ASSET_TYPE_FONT                 0x13
#define ASSET_TYPE_MENUFILE             0x14
#define ASSET_TYPE_MENU                 0x15        /* NONE */
#define ASSET_TYPE_LOCALIZE             0x16
#define ASSET_TYPE_WEAPON               0x17
#define ASSET_TYPE_SNDDRIVERGLOBALS     0x18
#define ASSET_TYPE_FX                   0x19
#define ASSET_TYPE_IMPACTFX             0x1A
#define ASSET_TYPE_AITYPE               0x1B        /* NONE */
#define ASSET_TYPE_MPTYPE               0x1C        /* NONE */
#define ASSET_TYPE_CHARACTER            0x1D        /* NONE */
#define ASSET_TYPE_XMODELALIAS          0x1E        /* NONE */
#define ASSET_TYPE_RAWFILE              0x1F
#define ASSET_TYPE_STRINGTABLE          0x20

class Asset 
{
public:
    Asset(void);
    ~Asset(void);
    virtual void Release(void) noexcept;

    virtual void Load(class FastFile *ff, address_t *handle) = 0;
    virtual void Store(class FastFile *ff, address_t *handle) = 0;
    //virtual void Export(Buffer_t *buffer) = 0;
};

/** Allows looking up the names of asset types. */
extern const char *lpAssetType[0x21];

#endif /* ASSET_HPP */

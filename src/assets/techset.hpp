#ifndef TECHSET_HPP
#define TECHSET_HPP

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"

#define MAX_TECHNIQUES          0x22
#define MAX_STATES_PER_MAP      12

#define TECHSET_DECOMPRESSION   2

struct Technique
{
    char *name;
#if 0
    address_t stateMap;
    address_t shaderVertex;
    address_t shaderPixel;
    address_t bindings;
#endif

    void Release(void)
    {
        if (name != nullptr)
        {
            name = nullptr;
        }
    }
};


class Techset : public Asset
{
public:
    Techset(void);
    ~Techset(void);
    void Release(void) noexcept;

    void Load(class FastFile *ff, address_t *handle);
    void Store(class FastFile *ff, address_t *handle);

private:
    void LoadTechnique(class FastFile *ff, address_t *handle);
    void LoadStateMap(class FastFile *ff, address_t *handle);
    void LoadShader(class FastFile *ff, address_t *handle);
    void LoadBinds(class FastFile *ff, address_t *handle, int bindInfo, int flags);

ASSET_PROPERTIES:
    char *name;
    address_t techniques[MAX_TECHNIQUES];
};


/** Allows looking up the name of the technique is linked to. */
extern const char *lpTechSetName[MAX_TECHNIQUES];


#endif /* TECHSET_HPP */

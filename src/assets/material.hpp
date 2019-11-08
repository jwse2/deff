#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"

class Material : public Asset
{
public:
    Material(void);
    ~Material(void);
    void Release(void) noexcept;

    void Load(class FastFile *ff, address_t *handle);
    void Store(class FastFile *ff, address_t *handle);

ASSET_PROPERTIES:
    char *name;
};

#endif /* MATERIAL_HPP */

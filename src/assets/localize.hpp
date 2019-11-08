#ifndef LOCALIZE_HPP
#define LOCALIZE_HPP

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"

class Localize : public Asset
{
public:
    Localize(void);
    ~Localize(void);
    void Release(void) noexcept;

    void Load(class FastFile *ff, address_t *handle);
    void Store(class FastFile *ff, address_t *handle);

ASSET_PROPERTIES:
    char *key;
    char *value;
};

#endif /* LOCALIZE_HPP */

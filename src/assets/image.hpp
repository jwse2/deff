#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"

class Image : public Asset
{
public:
    Image(void);
    ~Image(void);
    void Release(void) noexcept;

    void Load(class FastFile *ff, address_t *handle);
    void Store(class FastFile *ff, address_t *handle);

ASSET_PROPERTIES:
    char *name;
    struct
    {
        uint8_t type;
        uint8_t usage;

        uint16_t width;
        uint16_t height;
        uint16_t flags;

        uint32_t format;
        uint32_t zero;

    } metadata;
};

#endif /* IMAGE_HPP */

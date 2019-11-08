#ifndef RAWFILE_HPP
#define RAWFILE_HPP

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"

class Rawfile : public Asset
{
public:
    Rawfile(void);
    ~Rawfile(void);
    void Release(void) noexcept;
    
    void Load(class FastFile *ff, address_t *handle);
    void Store(class FastFile *ff, address_t *handle);

ASSET_PROPERTIES:
    char *name;
    char *data;
    int data_s;
};

#endif /* RAWFILE_HPP */

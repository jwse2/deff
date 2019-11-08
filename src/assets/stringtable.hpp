#ifndef STRINGTABLE_HPP
#define STRINGTABLE_HPP

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"

class Stringtable : public Asset
{
public:
    Stringtable(void);
    ~Stringtable(void);
    void Release(void) noexcept;

    void Load(class FastFile *ff, address_t *handle);
    void Store(class FastFile *ff, address_t *handle);

ASSET_PROPERTIES:
    char *name;
    int columns;
    int rows;
    char **cells;
};

#endif /* STRINGTABLE_HPP */

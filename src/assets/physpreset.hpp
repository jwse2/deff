#ifndef PHYSPRESET_HPP
#define PHYSPRESET_HPP

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"

class Physpreset : public Asset
{
public:
    Physpreset(void);
    ~Physpreset(void);
    void Release(void) noexcept;

    void Load(class FastFile *ff, address_t *handle);
    void Store(class FastFile *ff, address_t *handle);

ASSET_PROPERTIES:
    char *name;
    char *sndAliasPrefix;
    float mass;
    int friction;
    bool isFrictionInfinity;
    float bounce;
    float bulletForceScale;
    float explosiveForceScale;
    float piecesSpreadFraction;
    float piecesUpwardVelocity;
    bool tempDefaultToCylinder;
};

#endif /* PHYSPRESET_HPP */

#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"
#include "physpreset.hpp"

Physpreset::Physpreset(void)
{
    name = nullptr;
    sndAliasPrefix = nullptr;
    mass = 0.0f;
    friction = 0;
    isFrictionInfinity = false;
    bounce = 0.0f;
    bulletForceScale = 0.0f;
    explosiveForceScale = 0.0f;
    piecesSpreadFraction = 0.0f;
    piecesUpwardVelocity = 0.0f;
    tempDefaultToCylinder = false;
}

Physpreset::~Physpreset(void)
{
    Release();
}

void Physpreset::Release(void) noexcept
{
    name = nullptr;
    sndAliasPrefix = nullptr;
    mass = 0.0f;
    friction = 0;
    isFrictionInfinity = false;
    bounce = 0.0f;
    bulletForceScale = 0.0f;
    explosiveForceScale = 0.0f;
    piecesSpreadFraction = 0.0f;
    piecesUpwardVelocity = 0.0f;
    tempDefaultToCylinder = false;
}

void Physpreset::Load(class FastFile *ff, address_t *handle)
{
    union
    {
        address_t handler[11];
        int properties_i[11];
        float properties_f[11];
    };

    ASSERT(
        *handle == ADDRESS_MISSING || *handle == ADDRESS_FOLLOWING,
        "Internal error (0x%08X)",
            *handle
    );

    // Only load when the data is there.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the physpreset values.
        ff->ReadMemory(handler, 44);
        ASSERT(
            handler[0] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X)",
                handler[0]
        );

        // Copy the settings.
        isFrictionInfinity      = (properties_i[1] != 0) ? true : false;
        mass                    = (properties_f[2]);
        bounce                  = (properties_f[3]);
        friction                = (properties_i[4]);
        bulletForceScale        = (properties_f[5]);
        explosiveForceScale     = (properties_f[6]);
        piecesSpreadFraction    = (properties_f[8]);
        piecesUpwardVelocity    = (properties_f[9]);
        tempDefaultToCylinder   = (properties_i[10] != 0) ? true : false;

        // Load the name.
        if (handler[0] != ADDRESS_FOLLOWING)
        {
            name = ff->GetPointer(handler[0]);
        }
        else
        {
            // Read the name of the physicspreset.
            name = ff->ReadSharedString(64);
        }

        // Load the sound alias prefix.
        if (handler[7] != ADDRESS_MISSING)
        {
            if (handler[7] != ADDRESS_FOLLOWING)
            {
                sndAliasPrefix = ff->GetPointer(handler[7]);
            }
            else
            {
                // Read the sound alias prefix.
                sndAliasPrefix = ff->ReadSharedString(64);
            }
        }
        else
        {
            sndAliasPrefix = nullptr;
        }
    }

    Store(ff, handle);
}

void Physpreset::Store(class FastFile *ff, address_t *handle)
{
    UNREFERENCED_PARAMETER(ff);
    UNREFERENCED_PARAMETER(handle);
}

/**
 * FORMAT DOCUMENTATION
 * [00] int32       name_p
 * [04] bool        isFrictionInfinity      | Object Properties
 * [08] float       mass
 * [0C] float       bounce
 * [10] float       friction
 * [14] float       bulletForceScale        | Force Scaling
 * [18] float       explosiveForceScale
 * [1C] string      sndAliasPrefix          | Audio Physics
 * [20] float       piecesSpreadFraction    | Pieces
 * [24] float       piecesUpwardVelocity
 * [28] bool        tempDefaultToCylinder   | Globals
 */

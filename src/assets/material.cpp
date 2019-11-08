#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "material.hpp"

Material::Material(void)
{
    name = nullptr;
}

Material::~Material(void)
{
    Release();
}

void Material::Release(void) noexcept
{
    name = nullptr;
}

void Material::Load(class FastFile *ff, address_t *handle)
{
    union
    {
        address_t handler[20];
        int values[20];
    };

    // Only load when the data is there.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the material values.
        ff->ReadMemory(handler, 20*4);
        ASSERT(
            handler[0] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X)",
                handler[0]
        );

        // Read the material name.
        if (handler[0] != ADDRESS_FOLLOWING)
        {
            name = ff->GetPointer(handler[0]);
        }
        else
        {
            // Read the name of the material.
            name = ff->ReadSharedString(64);
            handler[0] = ff->GetAddress(4, name);
        }
        VERBOSE("material->name = '%s'\n", name);

        // NOTE: clusters of 3-bytes
        address_t buffer[3];
        ff->ReadMemory(buffer, 3*4);

    }

    ASSERT(FALSE, "Not yet implemented.\n");
    Store(ff, handle);
}

// 0: 256
// 1: 128
// 2:  64
// 3:  32
// 4:  16
// 5:   8
// 6:   4
// 7:   2
// 8:   1

void Material::Store(class FastFile *ff, address_t *handle)
{
    UNREFERENCED_PARAMETER(ff);
    UNREFERENCED_PARAMETER(handle);
}

/**
 * FORMAT DOCUMENTATION
 * [00] int32       name_p
 * [04] int32       . <NOT address>
 * [08] int32       . <NOT address>     contains some of the surface/render flags/states
 * [0C] int32       . <NOT address>
 * [10] int32       . <NOT address>
 * [14] int32       . <NOT address>
 * [18] int32       . <NOT address>
 * [1C] int32       . <NOT address>
 * [20] int32       . <NOT address>
 * [24] int32       . <NOT address>
 * [28] int32       . <NOT address>
 * [2C] int32       . <NOT address>
 * [30] int32       .
 * [34] int32       . <NOT address>
 * [38] int32       . <NOT address>
 * [3C] int32       . <NOT address>
 * [40] int32       . <address>
 * [44] int32       .
 * [48] int32       . <colorTint?>
 * [4C] int32       .
 * 
 * [__] char[]      material_name
 * [__] int32       <?address?>
 * [__] int64       ^^ if -1
 * 
 * 
 * COLORMAP
 * [??] int23       name_p
 * [??] char[]      name
 * [??] int64
 *      0000_0001_0001_0100             | color map specs?
 *      0000_0001_0001_0100             | 0, 256, 256, 1
 * [??] int32       format              | DXT1, DXT3, DXT5, etc.
 * 
 * 000000001288121802000EE0
 * 000000003A3B0CB6
 * 
 * UNKNOWN - Describes the image file
 * [??] int16       0
 * [??] int16       width
 * [??] int16       height
 * [??] int16       1
 * [??] int32       format              | DXT1, DXT3, DXT5, etc.
 * [??] int32       0
 * 
 * UNKNOWN
 * 
 * 
 * COLORTINT - Describes the color tint
 * [??] char[]      name
 * [??] int16       . <possible alignment>
 * [??] float       a
 * [??] float       r
 * [??] float       g
 * [??] float       b
 * 
 * UNKOWN
 * [??] int64[3]    .
 *  
 */


/* <[ RESEARCH ]>


< 2D >
FFFFFFFF2C04010100000000000000000010000000000000FFFFFFFF00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0100010003000D000040FFFFFFFF00000000FFFFFFFF
FFFFFFFF50040101000000000000000000100000000000000001020301FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF040404FFFFFFFF05FF010601FF02010738030015000040FFFFFFFFFFFFFFFFFFFFFFFF
< MODEL PHONG >

*/

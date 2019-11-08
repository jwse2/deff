#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "image.hpp"

Image::Image(void)
{
    name = nullptr;
}

Image::~Image(void)
{
    Release();
}

void Image::Release(void) noexcept
{
    name = nullptr;
}

void Image::Load(class FastFile *ff, address_t *handle)
{
    address_t handler[9];

    ASSERT(
        *handle == ADDRESS_MISSING || *handle == ADDRESS_FOLLOWING,
        "Internal error (0x%08X)",
            *handle
    );

    // Only load when the data is there.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the image values.
        ff->ReadMemory(handler, 9*4);
        ASSERT(
            handler[8] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X)",
                handler[8]
        );

        #if 0 // DEBUG
        for (int i = 0; i < 9; i++)
        {
            VERBOSE("image->handler[%02i] = %8x\n", i, handler[i]);
        }
        #endif

        // Read the image name.
        if (handler[8] != ADDRESS_FOLLOWING)
        {
            name = ff->GetPointer(handler[8]);
        }
        else
        {
            // Read the name of the image.
            name = ff->ReadSharedString(64);
            handler[8] = ff->GetAddress(4, name);
        }

        // Read the referenced image data.
        uint32_t buffer[3];
        ff->ReadMemory(buffer, 3*4);

        metadata.type = static_cast<uint8_t>((buffer[0] >> 0x00) & 0xFF);
        metadata.type = static_cast<uint8_t>((buffer[0] >> 0x08) & 0xFF);

        metadata.width = static_cast<uint16_t>((buffer[0] >> 0x10) & 0xFFFF);
        metadata.height = static_cast<uint16_t>((buffer[1] >> 0x00) & 0xFFFF);
        metadata.flags = static_cast<uint16_t>((buffer[1] >> 0x10) & 0xFFFF);

        metadata.format = static_cast<uint32_t>(buffer[2]);

        // TODO: NOTE:
        //   Usage unknown; but a pointer to the image data is most likely.
        uint32_t *ref = (uint32_t*)ff->ReadSharedMemory(4, 4);

        ASSERT(
            *ref == 0u,
            "Corrupted data. (0x%08X)",
                *ref
        );
    }

    Store(ff, handle);
}

void Image::Store(class FastFile *ff, address_t *handle)
{
    UNREFERENCED_PARAMETER(ff);
    UNREFERENCED_PARAMETER(handle);
}

/**
 * FORMAT DOCUMENTATION
 * [00] int32       .                   | 3 = 2D / 5 = Skybox
 * [04] int32       .                   | image->texture.basemap
 *
 * [08] int16       . picmip            | image->picmip
 * [0A] int8        . nopicmip          | skybox has this!
 * [0B] int8        . format            | image->format
 * [0C] int32       .
 * [10] int32       . mip#0 ?           | image->cardMemory.platform[PICMIP_PLATFORM_USED]
 * [14] int32       . mip#1 ?
 *
 * [18] int16       width               | iwiHeader->width
 * [1A] int16       height              | iwiHeader->height
 * [1C] int16       usage               | iwiHeader->usage
 * [1E] int8        .                   | image->category
 * [1F] int8        .                   | image->delayLoadPixels
 * [20] int32       name_p
 * 
 * [24] int8        . type              | 0 on 2D / 1 on Cube
 * [25] int8        . ?                 | 0 on 2D / 6 on Cube
 * [26] int16       . width             | actual .iwi width
 * [28] int16       . height            | actual .iwi height
 * [2A] int16       . flags
 * [2C] int32       . format            | DXT1, DXT5, etc.
 * [30] int32       . 0                 | (yet to encounter)
 */

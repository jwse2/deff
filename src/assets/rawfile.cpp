#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"
#include "rawfile.hpp"

Rawfile::Rawfile(void)
{
    name = nullptr;
    data = nullptr;
    data_s = 0;
}

Rawfile::~Rawfile(void)
{
    Release();
}

void Rawfile::Release(void) noexcept
{
    name = nullptr;
    data = nullptr;
    data_s = 0;
}

void Rawfile::Load(class FastFile *ff, address_t *handle)
{
    address_t handler[3];

    ASSERT(
        *handle == ADDRESS_MISSING || *handle == ADDRESS_FOLLOWING,
        "Internal error (0x%08X)",
            *handle
    );

    // Only load when the data is there.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the rawfile values.
        ff->ReadMemory(handler, 12);
        ASSERT(
            handler[0] != ADDRESS_MISSING && handler[2] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X)",
                handler[0]
        );

        // Load the name.
        if (handler[0] != ADDRESS_FOLLOWING)
        {
            name = ff->GetPointer(handler[0]);
        }
        else
        {
            // Read the name of the raw file.
            name = ff->ReadSharedString(64);
        }

        // Check the size of the rawfile.
        ASSERT(
            handler[1] >= 0,
            "Corrupted data. (%i)",
                handler[1]
        );

        // Store the data size.
        data_s = handler[1] + 1; // There is always an additional string terminator

        // Load the data.
        if (handler[2] != ADDRESS_FOLLOWING)
        {
            data = ff->GetPointer(handler[2]);
        }
        else
        {
            // Read the data of the raw file.
            data = (char*)ff->ReadSharedMemory(data_s);
        }
    }

    Store(ff, handle);
}

void Rawfile::Store(class FastFile *ff, address_t *handle)
{
    UNREFERENCED_PARAMETER(ff);
    UNREFERENCED_PARAMETER(handle);
}

/**
 * FORMAT DOCUMENTATION
 * [00] int32       name_p
 * [04] int32       data_s
 * [08] int32       data_p
 *      char[]      name
 *      char[]      data        | Data does not have to be C-strings.
 */

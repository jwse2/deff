#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "localize.hpp"

Localize::Localize(void)
{
    key = nullptr;
    value = nullptr;
}

Localize::~Localize(void)
{
    Release();
}

void Localize::Release(void) noexcept
{
    key = nullptr;
    value = nullptr;
}

void Localize::Load(class FastFile *ff, address_t *handle)
{
    address_t handler[2];

    ASSERT(
        *handle == ADDRESS_MISSING || *handle == ADDRESS_FOLLOWING,
        "Internal error (0x%08X)",
            *handle
    );

    // Only load when the data is there.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the localize values.
        ff->ReadMemory(handler, 8);
        ASSERT(
            handler[0] != ADDRESS_MISSING && handler[1] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X, 0x%08X)",
                handler[0], handler[1]
        );

        // Value
        if (handler[0] != ADDRESS_FOLLOWING)
        {
            value = ff->GetPointer(handler[0]);
        }
        else
        {
            // Read the value of the localize
            value = ff->ReadSharedString(64);
        }

        // Key
        if (handler[1] != ADDRESS_FOLLOWING)
        {
            key = ff->GetPointer(handler[1]);
        }
        else
        {
            // Read the key of the localize
            key = ff->ReadSharedString(64);
        }
    }

    Store(ff, handle);
}

void Localize::Store(class FastFile *ff, address_t *handle)
{
    UNREFERENCED_PARAMETER(ff);
    UNREFERENCED_PARAMETER(handle);
}


/**
 * FORMAT DOCUMENTATION
 * [00] int32       value_p
 * [04] int32       reference_p
 *      char[]      value
 *      char[]      reference
 */

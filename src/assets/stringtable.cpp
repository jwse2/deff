#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"
#include "stringtable.hpp"

Stringtable::Stringtable(void)
{
    name = nullptr;
    columns = 0;
    rows = 0;
    cells = nullptr;
}

Stringtable::~Stringtable(void)
{
    Release();
}

void Stringtable::Release(void) noexcept
{
    name = nullptr;
    columns = 0;
    rows = 0;

    if (cells != nullptr)
    {
        free(cells);
        cells = nullptr;
    }
}

void Stringtable::Load(class FastFile *ff, address_t *handle)
{
    union
    {
        address_t *handler;
        int *values;
    };

    ASSERT(
        *handle == ADDRESS_MISSING || *handle == ADDRESS_FOLLOWING,
        "Internal error (0x%08X)",
            *handle
    );

    // Only load when the data is there.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the stringtable values.
        handler = (address_t*)ff->ReadSharedMemory(4*4, 4);
        *handle = ff->GetAddress(4, handler);

        ASSERT(
            handler[0] != ADDRESS_MISSING && handler[3] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X, 0x%08X)",
                handler[0], handler[3]
        );

        // Read the name of the string table.
        if (handler[0] == ADDRESS_FOLLOWING)
        {
            char *name = (char*)ff->ReadSharedString(64);
            handler[0] = ff->GetAddress(4, name);
        }

        // NOTE: I don't know the exact min/max, but these seem to be it.
        ASSERT(values[1] >= 0x01 && values[2] >= 0x01,
            "Stringtable size out of bounds. (%i, %i)", 
                values[1], values[2]);
        ASSERT(values[1] <= 0xFF && values[2] <= 0xFF,
            "Stringtable size out of bounds. (%i, %i)",
                values[1], values[2]);

        // Load the data of the various cells.
        if (handler[3] == ADDRESS_FOLLOWING)
        {
            int index_s;
            address_t *index;
            
            // Precalculate the size of the index.
            index_s = (values[1] * values[2]);

            // Store the fast file address of the stringtable index.
            index = (address_t*)ff->ReadSharedMemory((index_s * 4), 4);
            handler[3] = ff->GetAddress(4, index);

            // Copy each string and store their values.
            for (int i = 0; i < index_s; i++)
            {
                // Determine where the string is...
                if (index[i] == ADDRESS_FOLLOWING)
                {
                    // Copy the string.
                    char *value = ff->ReadSharedString(64);
                    index[i] = ff->GetAddress(4, value);
                }
            }
        }
    }

    Store(ff, handle);
}

/**
 * Stores the data from memory in a 64-bit compatible way in the class object.
 * @param handle The handle to the stringtable in memory.
 */
void Stringtable::Store(class FastFile *ff, address_t *handle)
{
    union
    {
        address_t *handler;
        int *values;
    };
    address_t *index;

    ASSERT(
        *handle != ADDRESS_FOLLOWING,
        "Corrupted data. (0x%08X)",
            *handle
    );

    // Only load the data if there is data.
    if (*handle != ADDRESS_MISSING)
    {
        // Gets the pointer to the string table in memory.
        handler = (address_t*)ff->GetPointer(*handle);

        // Store simple types first.
        name = ff->GetPointer(handler[0]);
        columns = values[1];
        rows = values[2];

        // Get the pointer to the cells index.
        index = (address_t*)ff->GetPointer(handler[3]);

        // Allocate room for the cells.
        cells = (char**)calloc(columns * rows, sizeof(char*));
        if (cells == nullptr)
        {
            throw std::exception("Out of memory (stringtable)");
        }

        // Translate all the cell pointers.
        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < columns; x++)
            {
                int z = ((y * columns) + x);

                ASSERT(
                    index[z] != ADDRESS_FOLLOWING,
                    "Corrupted data. (0x%08X)",
                        index[z]
                );

                if (index[z] != ADDRESS_MISSING)
                {
                    cells[z] = ff->GetPointer(index[z]);
                }
            }
        }
    }
}

/**
 * FORMAT DOCUMENTATION
 * [00] int32       name_p
 * [04] int32       columns
 * [08] int32       rows
 * [0C] int32       cells_p
 *      char[]      name
 *      int32[]     cells       | Contains the address of each cell.
 *      char[]      strings     | Are always C-strings.
 */

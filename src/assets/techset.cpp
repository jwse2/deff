#include "../utility.hpp"
#include "../stream.hpp"
#include "../fastfile.hpp"
#include "../asset.hpp"
#include "techset.hpp"

#ifndef HANDLE_CHECK
#define HANDLE_CHECK
#endif

Techset::Techset(void)
{
    name = nullptr;
}

Techset::~Techset(void)
{
    Release();
}

void Techset::Release(void) noexcept
{
    name = nullptr;
}

void Techset::Load(class FastFile *ff, address_t *handle)
{
    address_t handler[37];

#ifdef HANDLE_CHECK
    ASSERT(
        *handle == ADDRESS_MISSING || *handle == ADDRESS_FOLLOWING,
        "Internal error (0x%08X)",
            *handle
    );
#endif

    // Only load when the data is there.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the techset values.
        ff->ReadMemory(handler, 37*4);
        ASSERT(
            handler[0] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X)",
                handler[0]
        );
        ASSERT(
            handler[1] == 0 && handler[2] == 0,
            "Corrupted data. (0x%08X, 0x%08X)",
                handler[1], handler[2]
        );

        // Read the techset name.
        if (handler[0] != ADDRESS_FOLLOWING)
        {
            name = ff->GetPointer(handler[0]);
        }
        else
        {
            // Read the name of the techset.
            name = ff->ReadSharedString(64);
        }

        // Read the technique handlers.
        //ff->ReadMemory(techniques, MAX_TECHNIQUES*4);
        memcpy(techniques, (handler + 3), (MAX_TECHNIQUES * 4));

        // Load all the individual techniques.
        for (int i = 0; i < MAX_TECHNIQUES; i++)
        {
            LoadTechnique(ff, (techniques + i));
        }
    }

    Store(ff, handle);
}

void Techset::LoadTechnique(class FastFile *ff, address_t *handle)
{
    address_t *handler = nullptr;

    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the technique values.
        handler = (address_t*)ff->ReadSharedMemory(7*4, 4);
        *handle = ff->GetAddress(4, handler);

        ASSERT(
            handler[0] != ADDRESS_MISSING,
            "Corrupted data. (0x%08X)",
                handler[0]
        );

        ASSERT(
            ((handler[1] >> 16) & 0xFFFF) == 1,
            "Corrupted data. (0x%08X)",
                handler[1]
        );

        // Technique dependencies
        LoadStateMap(ff, (handler + 2));
        LoadShader(ff, (handler + 3));
        LoadShader(ff, (handler + 4));
        LoadBinds(ff, (handler + 6), handler[5], 0);

        // Load the name.
        if (handler[0] == ADDRESS_FOLLOWING)
        {
            // Read the name of the technique.
            char *name = ff->ReadSharedString(64);
            handler[0] = ff->GetAddress(4, name);
        }
    }
}

/** TODO: FIXME: NOTE: The order may not be correct, needs to be fixed later. */
void Techset::LoadStateMap(class FastFile *ff, address_t *handle)
{
    int *map;

#ifdef HANDLE_CHECK
    ASSERT(
        *handle != ADDRESS_MISSING,
        "Corrupted data. (0x%08X)",
            *handle
    );
#endif

    // Load the state map.
    if (*handle == ADDRESS_FOLLOWING)
    {
        // Load the state map.
        map = (int*)ff->ReadSharedMemory(100, 4);
        *handle = ff->GetAddress(4, map);
    }
}

void Techset::LoadShader(class FastFile *ff, address_t *handle)
{
    int *handler;

#ifdef HANDLE_CHECK
    ASSERT(
        *handle != ADDRESS_MISSING,
        "Corrupted data. (0x%08X)",
            *handle
    );
#endif

    if (*handle == ADDRESS_FOLLOWING)
    {
        // Read the shader values.
        handler = (int*)ff->ReadSharedMemory(4*4, 4);
        *handle = ff->GetAddress(4, handler);

        ASSERT(
            handler[0] != 0 && handler[1] == 0 && handler[2] != 0,
            "Corrupted data. (0x%08X, 0x%08X, 0x%08X)",
                handler[0], handler[1], handler[2]
        );

        // Load the shader name.
        if (handler[0] == ADDRESS_FOLLOWING)
        {
            // Read the name of the shader.
            char *name = ff->ReadSharedString(64);
            handler[0] = ff->GetAddress(4, name);
        }

        // Load the shader data.
        if (handler[2] == ADDRESS_FOLLOWING)
        {
            // Calculate the size of the data.
            int data_s = ((handler[3] & 0xFFFF) * 4);

            // Read the data of the shader.
            int *data = (int*)ff->ReadSharedMemory(data_s, 4);
            handler[2] = ff->GetAddress(4, data);
        }
    }
}

void Techset::LoadBinds(class FastFile *ff, address_t *handle, int bindInfo, int flags)
{
    int count =
        (((bindInfo >> 0x10) & 0xFF) * 8) +
        (((bindInfo >> 0x08) & 0xFF) * 8) +
        (((bindInfo >> 0x00) & 0xFF) * 8);

    if (*handle == ADDRESS_FOLLOWING)
    {
        // Load the bindings.
        void *binds = ff->ReadSharedMemory(count, 4);
        *handle = ff->GetAddress(4, binds);

        // Bindings may contain an extension.
        address_t *extension = (address_t*)(((char*)binds) + count - 4);
        if (*extension == ADDRESS_FOLLOWING)
        {
            // Load the extension
            void *data = ff->ReadSharedMemory(16, -1);
            *extension = ff->GetAddress(4, data);
        }
    }
}

void Techset::Store(class FastFile *ff, address_t *handle)
{
    UNREFERENCED_PARAMETER(ff);
    UNREFERENCED_PARAMETER(handle);
}


const char *lpTechSetName[MAX_TECHNIQUES] = {

    "depth prepass",
    "build floatz",
    "build shadowmap depth",
    "build shadowmap color",
    "unlit",
    "emissive",
    "emissive shadow",
    "lit",
    "lit sun",
    "lit sun shadow",
    "lit spot",
    "lit spot shadow",
    "lit omni",
    "lit omni shadow",
    "lit instanced",
    "lit instanced sun",
    "lit instanced sun shadow",
    "lit instanced spot",
    "lit instanced spot shadow",
    "lit instanced omni",
    "lit instanced omni shadow",
    "light spot",
    "light omni",
    "light spot shadow",
    "fakelight normal",
    "fakelight view",
    "sunlight preview",
    "case texture",
    "solid wireframe",
    "shaded wireframe",
    "shadowcookie caster",
    "shadowcookie receiver",
    "debug bumpmap",
    "debug bumpmap instanced"
};


/**
 * FORMAT DOCUMENTATION
 * [00] int32       name_p
 * [04] int32       reserved
 * [08] int32       reserved
 * [0C] int32[34]   techniques_p        | Names above are in the correct order.
 *      char[]      name
 *      void[]      techniques          | Described below
 * 
 * TECHNIQUE - Describes a technique
 * [00] int32       name_p
 * [04] int32       stateBits           | unknown usage but is for the state map
 * [08] int32       stateMap_p
 * [0C] int32       vertexShader_p
 * [10] int32       pixelShader_p
 * [14] int32       bindings_p
 *      void[]      stateMap
 *      void[]      vertexShader
 *      void[]      pixelShader
 *      void[]      bindings
 *      char[]      name
 * 
 * STATEMAP - Describes a state map
 * [00] int32       count
 * [04] int64[12]   states
 *
 * SHADER - Describes a shader
 * [00] int32       name_p
 * [04] int32       reserved
 * [08] int16       data_s
 * [0A] int16       model               | Shader Model 3.0 / 2.0
 * [0C] int32       data_p
 *      DWORD[]     data
 */

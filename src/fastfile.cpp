#include <cstdio>
#include <exception>
#include <new>

#include "utility.hpp"
#include "stream.hpp"
#include "fstream.hpp"
#include "zstream.hpp"
#include "fastfile.hpp"

// Assets
#include "asset.hpp"
#include "assets/physpreset.hpp"        /* x01 */
#include "assets/material.hpp"          /* x04 */
#include "assets/techset.hpp"           /* x05 */
#include "assets/image.hpp"             /* x06 */
#include "assets/localize.hpp"          /* x16 */
#include "assets/rawfile.hpp"           /* x1F */
#include "assets/stringtable.hpp"       /* x20 */

#define FASTFILE_CHUNK      0x10000


/**
 * Creates a new FastFile object using an ANSI path.
 * @param path The ANSI path of the file.
 */
FastFile::FastFile(const char *path)
{
    if (MultiByteToWideChar(CP_ACP, 0, path, -1, (LPWSTR)path, MAX_PATH))
    {
        throw Exception("Could not convert path from ANSI to UNICODE.");
    }
    Initialize();
}

/**
 * Creates a new FastFile object using an UNICODE path.
 * @param path The UNICODE path of the file.
 */
FastFile::FastFile(const wchar_t *path)
{
    if (wcscpy_s(this->path, path))
    {
        throw Exception("Could not set UNICODE path.");
    }
    Initialize();
}

/**
 * Releases all the resources used by the FastFile object.
 */
FastFile::~FastFile(void)
{
    int id;

    if (stream != nullptr)
    {
        delete stream;
        stream = nullptr;
    }

    if (file != nullptr)
    {
        // NOTE: We use the Windows/WIN32 implementation not STD. Hence the STD
        //       namespace prefix is not there.
        fclose(file);
        file = nullptr;
    }

    if (data != nullptr)
    {
        // NOTE: In the origianl implementation data would contain the pointers.
        //       However due to the fact that those pointers are 32-bit wide and
        //       ours are 64-bit wide we can NOT store them there. Hence the
        //       pointers shall be replaced with fast file address pointers.
        //         Setting the pointers to their fast file address allows one to
        //       reference the data if necessary. This however takes longer than
        //       accessing it through the index. This is because there are thus
        //       two arrays. One in the data containing fast file address
        //       pointers. And one in memory containing system/memory pointers.
        free(data);
        data = nullptr;
    }

    // Release all the tag pointers.
    id = SECTION_ID_TAGS;
    if (section[id].tags != nullptr)
    {
        free(section[id].tags);
        section[id].tags = nullptr;
    }

    // Ensure all the individual assets are released.
    id = SECTION_ID_ASSETS;
    if (section[id].assets != nullptr)
    {
        for (int i = 0; i < section[id].count; i++)
        {
            // The assets array contains assets, these need to be released.
            if (section[id].assets[i].asset != nullptr)
            {
                section[id].assets[i].asset->Release();
                delete section[id].assets[i].asset;
            }
        }
        free(section[id].assets);
        section[id].assets = nullptr;
    }
}

/**
 * Initializes the FastFile object for use.
 */
void FastFile::Initialize(void)
{
    // First set all dynamic variables to their default values.
    stream = nullptr;
    data = nullptr;
    section[0].count = 0;
    section[0].tags = nullptr;
    section[1].count = 0;
    section[1].assets = nullptr;

    // As last open the file.
    if (_wfopen_s(&file, path, L"rb"))
    {
        throw Exception("Could not open file at path '%ls'.", path);
    }

    VERBOSE("Loading FastFile '%ls'\n", path);
}

/**
 * Loads the FastFile from file into memory.
 */
void FastFile::Load(void)
{
    // Ensure the file is valid.
    Validate();

    // Realign to the starting of the encoded part.
    if (fseek(file, 12, SEEK_SET))
    {
        throw Exception("Could not reposition within the file.");
    }

    // Create a ZLib stream to use for reading.
    stream = (Stream*) new ZLibStream(file);
    if (stream == nullptr)
    {
        throw Exception("Could not allocate ZLibStream.");
    }

    // Load the fast file data in the correct order.
    LoadHeader(stream);
    LoadTags(stream);
    LoadAssets(stream);

    // Clean up
    delete stream;
    stream = nullptr;
}

/**
 * Gets the fast file address.
 * @param group The group to get the address of.
 * @param address The pointer to the data within memory.
 * @return The address within the fast file.
 */
address_t FastFile::GetAddress(int group, void *address)
{
    size_t intermediate;

    ASSERT(group == 4, "Only group 4 is currently supported. (%i)", group);

    if (address == nullptr)
    {
        intermediate = (current - data + 1);
    }
    else
    {
        ASSERT(
            address >= data && (address < (data + header[6])),
            "Address is not of the fast file. %p not in [%p, %p)",
                address, data, (data + header[6])
        );

        intermediate = (((char*)address) - data + 1);
    }

    ASSERT(
        intermediate >= 1 && intermediate <= header[6],
        "Corrupted pointer requested.  1 <= %i >= %i",
            intermediate, header[6]
    );

    return (address_t)(intermediate | 0x40000000);
}

/**
 * Validate a fast file address.
 * @param address The address to validate.
 * @return true if valid; otherwise, false.
 */
bool FastFile::IsValidAddress(address_t address)
{
    int group, offset, intermediate;

    if (address == ADDRESS_MISSING || address == ADDRESS_FOLLOWING)
    {
        return true;
    }

    intermediate = (int)(address);
    group   = ((intermediate >> 28) & 7);
    offset  = ((intermediate >>  0) & 0x0FFFFFFF);

    return (group == 4 && offset > 0 && offset <= header[6]);
}

/**
 * Get the address pointer for the fast file address.
 * @param address The fast file address.
 */
char* FastFile::GetPointer(address_t address)
{
    int group, offset, intermediate;

    ASSERT(address != ADDRESS_MISSING && address != ADDRESS_FOLLOWING,
        "Invalid address passed. (0x%08X)", address);

    intermediate = (int)(address);
    group   = ((intermediate >> 28) & 7);
    offset  = ((intermediate >>  0) & 0x0FFFFFFF);

    ASSERT(group == 4 && offset > 0 && offset <= header[6],
        "Currently unsupported. (0x%08X)",
            address
    );

    return (data - 1 + offset);
}

/**
 * Align the current address to the given offset.
 * @param alignment The alignment to use.
 */
void FastFile::Align(int alignment)
{
    size_t diff;

    // Ensure the alignment is a multiple of 4.
    ASSERT(
        (alignment & 3) == 0,
        "Invalid alignment value. (0x%08X)",
            alignment
    );

    // Determine the difference and realign.
    diff = (current - data);
    current = (data + ALIGN(diff, alignment));
}

/**
 * Allocates a predefined number of bytes from the memory.
 * @param size The number of bytes to allocate.
 * @param alignment The required allignment.
 * @return A pointer to the allocated address.
 */
void* FastFile::Alloc(int size, int alignment)
{
    ASSERT(
        size > 0,
        "Invalid size of %i used for allocation.",
            size
    );

    // -1 is to indicate no alignment is required
    if (alignment != -1)
    {
        Align(alignment);
    }

    // Boundaries check is necessary
    if ((current + size) > (data + header[6]))
    {
        throw Exception("Tried to allocate %i bytes beyond the memory boundary.",
            (current + size) - (data + header[6]));
    }

    // Allocate and advance the pointer for the next allocation
    void *ptr = current;
    current += size;
    return ptr;
}

/**
 * Reads a block of bytes from the stream into seperated memory.
 * @param dest The destination to read to.
 * @param size The number of bytes to read.
 */
void* FastFile::ReadMemory(void *dest, int size)
{
    int result;

    ASSERT(
        dest != nullptr && size > 0,
        "Invalid parameter(s) passed. (%p / %i)",
            dest, size
    );

    // Read the data from the stream into memory
    result = stream->ReadMemory(dest, size);
    if (result != size)
    {
        throw Exception("Could not read requested number of bytes. %i of %i read.", result, size);
    }

    return dest;
}

/**
 * Reads a block of bytes from the stream into shared memory.
 * @param size The number of bytes to read.
 * @param alignment The requested alignment.
 * @return A pointer to the data.
 */
void* FastFile::ReadSharedMemory(int size, int alignment)
{
    void *dest;
    int result;

    ASSERT(
        size >= 1 && size <= 0x2000000,
        "Single memory allocation size exceeded. %i not in [%i, %i)",
            size, 1, 0x2000000 /* 32 MB */
    );

    // Allocate the space
    dest = Alloc(size, alignment);

    // Read the data from the stream into the memory
    result = stream->ReadMemory(dest, size);
    if (result != size)
    {
        throw Exception("Could not read desired memory. %i of %i read.", result, size);
    }

    return dest;
}

/**
 * Reads a string from the stream into seperated memory.
 * @param dest The destination to read to.
 * @param max The maximum number of bytes to read.
 */
char* FastFile::ReadString(char *dest, int max)
{
    int read;

    ASSERT(
        dest != nullptr && max > 0,
        "Invalid parameter(s) passed. (%p / %i)",
            dest, max
    );

    // Read the string
    read = stream->ReadString(dest, max);
    if (read < 1)
    {
        throw Exception("Could not read string. (%d)", read);
    }

    return dest;
}

/**
 * Allocates a string with a maximum number of characters.
 * @param max The maximum number of characters to read.
 * @return A pointer to the string.
 */
char* FastFile::ReadSharedString(int max, int alignment)
{
    char buffer[256], *dest;
    int read;

    ASSERT(
        max >= 1 && max <= 256,
        "Max string size must be in [%i, %i) but %i is used instead.",
            1, 256, max
    );

    // Read the string
    read = stream->ReadString(buffer, max);
    if (read < 1)
    {
        throw Exception("Could not read string. (%d)", read);
    }

    // Allocate memory and store the string in it
    dest = (char*)Alloc(read, alignment);
    if (dest == nullptr)
    {
        throw Exception("Could not allocate memory for a string.");
    }

    // Copy the string
    memcpy(dest, buffer, read);
    return dest;
}

void* FastFile::AllocSharedMemory(int size, int alignment)
{
    void *dest;

    ASSERT(
        size > 0,
        "Size must be above 0, but %i has been given.",
            size
    );

    // Allocate the memory
    dest = Alloc(size, alignment);
    if (dest == nullptr)
    {
        throw Exception("Could not allocate memory for a string.");
    }

    return dest;
}

/**
 * Validates the file is a FastFile by inspecting its header data.
 */
void FastFile::Validate(void)
{
    Stream *stream;
    union
    {
        int header[3];
        short encoding[7];
    };
    
    // Create the stream.
    stream = (Stream*) new FileStream(file);
    if (stream == nullptr)
    {
        throw Exception("Could not allocate FileStream.");
    }

    // Load the data into encoding, this is guarenteed to hold the data.
    stream->ReadMemory(encoding, 14);

    // Verify the version
    if (header[0] != (int)0x66665749 ||
        header[1] != (int)0x30303175 ||
        header[2] != (int)0x00000005)
    {
        if (header[2] != (int)0x00000005)
        {
            throw Exception("Fast file is not from MW1.");
        }
        else
        {
            throw Exception("File is not a Fast File.");
        }
    }

    // MW1 on PC only uses best compression
    if (encoding[6] != (short)0xDA78)  
    {
        VERBOSE("encoding[6] = 0x%04hX\n", encoding[6]);
        throw Exception("Corrupted Fast File data.");
    }

    // Clean up
    delete stream;
}

/**
 * Load the header and fixed values into memory.
 * @param stream The stream to load the data from.
 */
void FastFile::LoadHeader(Stream *stream)
{
    int file_s, data_s;
    struct
    {
        int count;
        int address;
    } section[2];

    // The header can be read as is.
    stream->ReadMemory(header, 44);
    stream->ReadMemory(section, 16);

    // Grab the file size and data size so they can be checked quickly.
    file_s = header[0];
    //dds_s = header[1];
    data_s = header[6];

    // MIN: 60-bytes / MAX: 256 MB
    ASSERT(file_s >= 0x3C && file_s <= 0x10000000, "File size is out of bounds. (0x%08X)", file_s);
    ASSERT(data_s >= 0x00 && data_s <= 0x0FFFFFC4, "Data size is out of bounds. (0x%08X)", data_s);

    // Allocate the data buffer.
    data = (char*)calloc(data_s, 1);
    if (data == nullptr)
    {
        throw Exception("Out of memory (data_s)");
    }
    current = data;

    // Set the variables.
    this->section[SECTION_ID_TAGS].count = (
        (((long long int)(section[SECTION_ID_TAGS].address)) << 32) |
        (long long int)section[SECTION_ID_TAGS].count
    );

    this->section[SECTION_ID_ASSETS].count = (
        (((long long int)(section[SECTION_ID_ASSETS].address)) << 32) |
        (long long int)section[SECTION_ID_ASSETS].count
    );
}

/**
 * Load the tags data into memory.
 * @param stream The stream to load the data from.
 */
void FastFile::LoadTags(Stream *stream)
{
    int count, address;
    char **tags;

    // Grab the count and starting address.
    count = ((section[SECTION_ID_TAGS].count >> 0) & 0xFFFFFFFF);
    address = ((section[SECTION_ID_TAGS].count >> 32) & 0xFFFFFFFF);

    // Verify the count and address combinations
    ASSERT(
        (count == 0 && address == ADDRESS_MISSING) ||
        (count >= 1 && address == ADDRESS_FOLLOWING),
        "Invalid section combination. (%i, 0x%08X)",
            count, address
    );

    if (count == 0)
    {
        section[SECTION_ID_TAGS].tags = nullptr;
        section[SECTION_ID_TAGS].count = 0;
    }
    else
    {
        // Allocate memory for the tags index.
        tags = (char**)calloc(count, sizeof(char*));
        if (tags == nullptr)
        {
            throw Exception("Could not allocate tags array.");
        }

        // Assign the class variables.
        section[SECTION_ID_TAGS].tags = tags;
        section[SECTION_ID_TAGS].count = count;

        // Start reading the tags into memory.
        ReadTags(stream, count, address);
    }
}

/**
 * Reads the tags data.
 * @param stream The stream to load the data from.
 * @param count The number of tag entries.
 * @param address The address to data resides at.
 */
void FastFile::ReadTags(Stream *stream, int count, address_t address)
{
    int *index;

    UNREFERENCED_PARAMETER(address);
    ASSERT(count > 0 && count == section[SECTION_ID_TAGS].count,
        "Internal error (%i != %lli)",
            count, section[SECTION_ID_TAGS].count
    );

#if 0
    // Ensure the index is aligned.
    Align(4);
#endif

    // Preserve a pointer into the index.
    index = (int*)current;

    // Copy the index to the data.
    stream->ReadMemory(current, (count * 4));
    current += (count * 4);

    // Copy the strings and set the variables.
    for (int i = 0; i < count; i++)
    {
        // Could be something else but would be bad optimization.
        ASSERT(
            index[i] == ADDRESS_MISSING ||
            index[i] == ADDRESS_FOLLOWING,
            "Bad optimization detected. (%i, 0x%08X)",
                i, index[i]
        );

        // Either read or set the pointers
        if (index[i] == ADDRESS_MISSING)
        {
            section[SECTION_ID_TAGS].tags[i] = nullptr;
        }
        else
        {
            // Set the variables.
            index[i] = GetAddress(4);
            section[SECTION_ID_TAGS].tags[i] = current;

            // Copy the string.
            stream->ReadString(current, -1);
            current += (strlen(current) + 1);
        }
    }
}

/**
 * Load the assets data into memory.
 * @param stream The stream to load the data from.
 */
void FastFile::LoadAssets(Stream *stream)
{
    int count, address;
    struct AssetEntry *assets;

    // Grab the count and starting address.
    count = ((section[SECTION_ID_ASSETS].count >> 0) & 0xFFFFFFFF);
    address = ((section[SECTION_ID_ASSETS].count >> 32) & 0xFFFFFFFF);

    // A fast file has a minimum of one asset.
    ASSERT(
        (count == 0 && address == ADDRESS_MISSING) ||
        (count >= 1 && address == ADDRESS_FOLLOWING),
        "Invalid section combination. (%i, 0x%08X)",
            count, address
    );

    if (count == 0)
    {
        section[SECTION_ID_ASSETS].assets = nullptr;
        section[SECTION_ID_ASSETS].count = 0;
    }
    else
    {
        // Allocate memory for the assets index.
        assets = (AssetEntry*)calloc(count, sizeof(AssetEntry));
        if (assets == nullptr)
        {
            throw Exception("Could not allocate assets structures array.");
        }

        // Assign the class variables.
        section[SECTION_ID_ASSETS].assets = assets;
        section[SECTION_ID_ASSETS].count = count;

        // Start reading the assets into memory.
        ReadAssets(stream, count, address);
    }
}

/**
 * Reads the assets data.
 * @param stream The stream to load the data from.
 * @param count The number of assets entries.
 * @param address The address to data resides at.
 */
void FastFile::ReadAssets(Stream *stream, int count, address_t address)
{
    int id;
    address_t *index;

    UNREFERENCED_PARAMETER(address);
    ASSERT(count > 0 && count == section[SECTION_ID_ASSETS].count,
        "Internal error (%i != %lli)",
            count, section[SECTION_ID_ASSETS].count
    );

    // We are only accessing the assets.
    id = SECTION_ID_ASSETS;

    // Pointers require four bye alignment.
    Align(4);

    // Preserve a pointer into the index.
    index = (address_t*)current;

    // Copy the index
    stream->ReadMemory(current, (count * 8));
    current += (count * 8);

    // Fill the memory index. (For 64-bit compatibility.)
    for (int i = 0; i < count; i++)
    {
        int type, addr;
        type = index[(i * 2)];
        addr = index[(i * 2) + 1];

        ASSERT(
            addr == ADDRESS_MISSING || addr == ADDRESS_FOLLOWING,
            "Asset addressing error. (0x%08X)",
                addr
        );

        // Store the type.
        section[id].assets[i].type = type;

        // Each type requires individual allocation.
        switch (type)
        {
        case ASSET_TYPE_PHYSPRESET:
            section[id].assets[i].asset = (Asset*) new Physpreset();
            break;

        case ASSET_TYPE_MATERIAL:
            section[id].assets[i].asset = (Asset*) new Material();
            break;

        case ASSET_TYPE_TECHSET:
            section[id].assets[i].asset = (Asset*) new Techset();
            break;

        case ASSET_TYPE_IMAGE:
            section[id].assets[i].asset = (Asset*) new Image();
            break;

        case ASSET_TYPE_LOCALIZE:
            section[id].assets[i].asset = (Asset*) new Localize();
            break;

        case ASSET_TYPE_RAWFILE:
            section[id].assets[i].asset = (Asset*) new Rawfile();
            break;

        case ASSET_TYPE_STRINGTABLE:
            section[id].assets[i].asset = (Asset*) new Stringtable();
            break;

        default:
            ASSERT(
                type >= 1 && type <= 0x20,
                "Invalid asset type. (0x%02X)",
                    type
            );
            section[id].assets[i].asset = nullptr;
            break;
        }
    }

    // Load individual assets.
    for (int i = 0; i < count; i++)
    {
        Asset *asset = section[id].assets[i].asset;

        if (asset == nullptr)
        {
            throw Exception(
                "Encountered unsupported asset type. (%s / type: %i))",
                lpAssetType[section[id].assets[i].type],
                section[id].assets[i].type
            );
        }

        VERBOSE("Parsing asset %i/%i of type %s\n", i+1, count, lpAssetType[section[id].assets[i].type]);
        asset->Load(this, (index + (i * 2) + 1));
        VERBOSE("DONE.");

#ifdef DEBUG

#define ASSET_NOT(x) \
    case (ASSET_TYPE_ ##x): \
        throw Exception( "" #x " not yet implemented." ); \
        break

        const int OFFSET = 32;

        switch (section[id].assets[i].type)
        {
        case ASSET_TYPE_PHYSPRESET:
            {
                class Physpreset *phys = (class Physpreset *)section[id].assets[i].asset;
                if (phys != nullptr)
                {
                    VERBOSE("\nPYSPRESET\n\t%-32s%s\n\t%-32s%s\n\t%-32s%f\n\t%-32s0x%08X\n\t%-32s%i\n\t%-32s%f\n\t%-32s%f\n\t%-32s%f\n\t%-32s%f\n\t%-32s%f\n\t%-32s%i\n",
                        "name", phys->name,
                        "sndAliasPrefix", phys->sndAliasPrefix,
                        "mass", phys->mass,
                        "friction", phys->friction,
                        "isFrictionInfinity", phys->isFrictionInfinity,
                        "bounce", phys->bounce,
                        "bulletForceScale", phys->bulletForceScale,
                        "explosiveForceScale", phys->explosiveForceScale,
                        "piecesSpreadFraction", phys->piecesSpreadFraction,
                        "piecesUpwardVelocity", phys->piecesUpwardVelocity,
                        "tempDefaultToCylinder", phys->tempDefaultToCylinder
                    );
                }
            }
            break;

        case ASSET_TYPE_MATERIAL:
            {
                class Material *mat = (class Material *)section[id].assets[i].asset;
                if (mat != nullptr)
                {
                    VERBOSE("\nMATERIAL\n\t%-*s%s\n", OFFSET,
                        "material->name", mat->name);
                }
            }
            break;

        case ASSET_TYPE_TECHSET:
            {
                class Techset *set = (class Techset *)section[id].assets[i].asset;
                if (set != nullptr)
                {
                    VERBOSE("\nTECHSET\n\t%-*s%s\n", OFFSET, "techset", set->name);

                    for (int j = 0; j < MAX_TECHNIQUES; j++)
                    {
                        address_t *address_p, address;
                        bool validAddress = false;

                        address = set->techniques[j];
                        VERBOSE("\t%-32s", lpTechSetName[j]);

                        if (address != ADDRESS_MISSING)
                        {
                            if (IsValidAddress(address))
                            {
                                address_p = (address_t*)GetPointer(address);

                                if (*address_p != ADDRESS_MISSING)
                                {
                                    if (IsValidAddress(*address_p))
                                    {
                                        char *techName = GetPointer(*address_p);

                                        if (techName[0] != 0)
                                        {
                                            VERBOSE("%s\n", techName);
                                            validAddress = true;
                                        }
                                    }
                                }
                                else
                                {
                                    validAddress = true;
                                    VERBOSE("<null>\n");
                                }

                                if (!validAddress)
                                {
                                    VERBOSE("<error>\n");
                                    validAddress = true; // The other was valid
                                }
                            }
                        }
                        else
                        {
                            validAddress = true;
                            VERBOSE("<null>\n");
                        }

                        if (!validAddress)
                        {
                            VERBOSE("<error>\n");
                        }
                    }
                }
            }
            break;

        case ASSET_TYPE_IMAGE:
            {
                class Image *img = (class Image *)section[id].assets[i].asset;
                if (img != nullptr)
                {
                    VERBOSE("\nIMAGE\n\t%-*s%s\n", OFFSET, "image->name", img->name);

                    VERBOSE("\t%-*s%hhu\n", OFFSET, "image->metadata.type", img->metadata.type);
                    VERBOSE("\t%-*s%hhu\n", OFFSET, "image->metadata.usage", img->metadata.usage);

                    VERBOSE("\t%-*s%hu\n", OFFSET, "image->metadata.width", img->metadata.width);
                    VERBOSE("\t%-*s%hu\n", OFFSET, "image->metadata.height", img->metadata.height);
                    VERBOSE("\t%-*s%hu\n", OFFSET, "image->metadata.flags", img->metadata.flags);

                    uint32_t format = img->metadata.format;

                    if (format == 0x31545844 ||
                        format == 0x33545844 ||
                        format == 0x35545844)
                    {
                        VERBOSE("\t%-*s%.4s\n", OFFSET, "image->metadata.format",
                            (char*)(&(img->metadata.format)));
                    }
                    else
                    {
                        VERBOSE("\t%-*s%i\n", OFFSET, "image->metadata.format", img->metadata.format);
                    }
                }
            }
            break;

        case ASSET_TYPE_LOCALIZE:
            {
                class Localize *loc = (class Localize *)section[id].assets[i].asset;
                if (loc != nullptr)
                {
                    VERBOSE("\nLOCALIZE\n\t%s\n\t%s\n",
                        loc->key, loc->value);
                }
            }
            break;

        case ASSET_TYPE_RAWFILE:
            {
                class Rawfile *raw = (class Rawfile *)section[id].assets[i].asset;
                if (raw != nullptr)
                {
                    VERBOSE("\nRAWFILE\n\t%s (%i)\n\t%s\n",
                        raw->name, raw->data_s, raw->data);
                }
            }
            break;

        case ASSET_TYPE_STRINGTABLE:
            {
                class Stringtable *table = (class Stringtable *)section[id].assets[i].asset;
                if (table != nullptr)
                {
                    VERBOSE("\nSTRINGTABLE\n");

                    for (int y = 0; y < table->rows; y++)
                    {
                        fputc('\t', stderr);

                        for (int x = 0; x < table->columns; x++)
                        {
                            int z = ((y * table->columns) + x);

                            if (x == 0)
                            {
                                VERBOSE("%s", table->cells[z]);
                            }
                            else
                            {
                                VERBOSE(",%s", table->cells[z]);
                            }
                        }

                        fputc('\n', stderr);
                    }
                }
            }
            break;

        // Types for future implementation
        ASSET_NOT( XANIM );
        ASSET_NOT( XMODEL );
        ASSET_NOT( SOUND );
        ASSET_NOT( SNDCURVE );
        ASSET_NOT( COL_MAP_SP );
        ASSET_NOT( COL_MAP_MP );
        ASSET_NOT( COM_MAP );
        ASSET_NOT( GAME_MAP_SP );
        ASSET_NOT( GAME_MAP_MP );
        ASSET_NOT( GFX_MAP );
        ASSET_NOT( LIGHTDEF );
        ASSET_NOT( FONT );
        ASSET_NOT( MENUFILE );
        ASSET_NOT( WEAPON );
        ASSET_NOT( SNDDRIVERGLOBALS );
        ASSET_NOT( FX );
        ASSET_NOT( IMPACTFX );
        
        default:
            break;
        }
        fputc('\n', stderr);
#endif
    }

    // Verify we read exactly.
    size_t size = header[6];
    size_t read = (current - data);

    if (read != size)
    {
        DumpMemory();
    }

    ASSERT(
        read == size,
        "NOTICE: Did not read all the data. (0x%08zX != 0x%08zX)\n",
            size, read
    );
}

/**
 * Generates a memory dump into the output stream.
 */
void FastFile::DumpMemory(void)
{
#ifdef DEBUG

    fputs("\n\nMEMORY DUMP\n", stdout);

    // Generate a dump of the data buffer.
    for (int i = 0; i < header[6]; i++)
    {
        if ((i % 8) == 0)
        {
            fputs("  ", stdout);
        }

        fprintf(stdout, "0x%02hhX ", data[i]);

        if ((i % 8) == 7)
        {
            fputc('\n', stdout);
        }
    }

    fputc('\n\n', stdout);

    // Ensure a dump is written
    fflush(stdout);

#endif
}

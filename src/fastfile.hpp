#ifndef FASTFILE_HPP
#define FASTFILE_HPP

#include <cstdio>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include "utility.hpp"
#include "stream.hpp"
#include "asset.hpp"

#define SECTION_ID_TAGS     0
#define SECTION_ID_ASSETS   1

#define ADDRESS_MISSING     (0)
#define ADDRESS_FOLLOWING   (-1)


struct AssetEntry
{
    long long int type;
    class Asset *asset;
};

struct Section
{
    long long int count;
    union
    {
        char **tags;
        struct AssetEntry *assets;
    };
};

class FastFile
{
public:
    FastFile(const char *path);
    FastFile(const wchar_t *path);
    ~FastFile(void);

    void Load(void);
    void DumpMemory(void);

    // Address and pointer manipulation
    bool IsValidAddress(address_t address);
    address_t GetAddress(int group, void *address = nullptr);
    char* GetPointer(address_t address);

    // Allocates memory, used only during asset loading
    void* ReadMemory(void *dest, int size);
    char* ReadString(char *dest, int max);
    void* ReadSharedMemory(int size, int alignment = -1);
    char* ReadSharedString(int max, int alignment = -1);
    void* AllocSharedMemory(int size, int alignment = -1);
   
private:
    void* Alloc(int size, int alignment);
    void Initialize(void);
    void Validate(void);
    void LoadHeader(Stream *stream);
    void LoadTags(Stream *stream);
    void ReadTags(Stream *stream, int count, address_t address);
    void LoadAssets(Stream *stream);
    void ReadAssets(Stream *stream, int count, address_t address);
    void Align(int alignment);

private:
    wchar_t path[MAX_PATH];
    std::FILE *file;
    Stream *stream;

    // FastFile data
    int header[11];
    char *data;
    char *current;
    struct Section section[2];
};

#endif /* FASTFILE_HPP */

#include <cstdio>
#include <exception>
#include <zlib.h>
#include "utility.hpp"
#include "version.h"
#include "fastfile.hpp"



int getMaxSize(int argc, wchar_t **argv)
{
    int len, size = 0;

    for (int i = 0; i < argc; i++)
    {
        len = (int)wcslen(argv[i]);
        if (len > size)
        {
            size = len;
        }
    }

    // 3 dots spacing
    return size + 3;
}


int wmain(int argc, wchar_t **argv)
{
    std::fprintf(stdout, "DEFF: %s\n", DEFF_VERSION_LONG);

#ifdef DEBUG
    std::fprintf(stderr, "DEFF: %s\nZLIB: %s\n", DEFF_VERSION_LONG, zlibVersion());
#endif

    if (argc < 2)
    {
        fputs("USAGE: deff.exe < files >\n", stdout);
        return 0;
    }

    int maxSize = getMaxSize(argc - 1, argv + 1);
    wchar_t *filepath = new wchar_t[maxSize];

    for (int i = 1; i < argc; i++)
    {
        for (int i = 0; i < maxSize; i++)
            filepath[i] = L'.';
        memcpy_s(filepath, maxSize * sizeof(wchar_t), argv[i],
            (wcslen(argv[i]) * sizeof(wchar_t)));

        fprintf(stdout, "Loading %-*ls", maxSize, filepath);
        fflush(stdout);

        try
        {
            FastFile *ff = new FastFile(argv[i]);
            if (ff)
            {
                ff->Load();
                fputs("SUCCESS\n", stdout);
                delete ff;
            }
            else
            {
                fputs("FAILED\n", stdout);
                fprintf(stderr, "Could not create FastFile object for '%ls'.\n", argv[i]);
            }
        }
        catch (const Exception &ex)
        {
            const struct StackTrace &trace = ex.stackTrace();

            fputs("FAILED\n", stdout);
            fprintf(stderr, "\nEXCEPTION\n\t%ls\n\t%s\n", argv[i], ex.what());

            fprintf(stderr, "\n==== STACK TRACE (%i) ============================\n", trace.argc);
            for (int e = 0; e < trace.argc; e++)
            {
                fprintf(stderr, "\n%s", trace.argv[e]);
            }
            fputs("\n==================================================\n", stderr);

            fflush(stderr);
        }
        catch (const std::exception &ex)
        {
            fputs("FAILED\n", stdout);
            fprintf(stderr, "\nEXCEPTION\n\t%ls\n\t%s\n\n", argv[i], ex.what());
            fflush(stderr);
        }
    }

    delete[] filepath;
    return 0;
}

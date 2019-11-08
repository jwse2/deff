/**
 * cl -c -W3 -MT -EHsc -D_CRT_SECURE_NO_WARNINGS main.cpp
 * link main.obj kernel32.lib user32.lib zlib.lib
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#define MSG_SIZE        (256 + MAX_PATH)    /* Message is max 256-bytes but contains the filename/path. */
#define SEEK_BUFFER     0x40                /* The ZLib header should occur in the first 0x40 bytes. */

/** zpipe.c - Example functions */
void zerr(int ret);                     /* ZLib errors in human readable format. */
int inf(FILE *source, FILE *dest);      /* Decodes data that is encoded with ZLib. */

/* dumper - Forward declarations */
void search(const wchar_t *path_in, const wchar_t *path_out);


/**
 * Main program entry point. Uses UNICODE for Windows.
 * @param argc The number of arguments passed.
 * @param argv The arguments as passed.
 * @return 0
 */
int wmain(int argc, wchar_t **argv)
{
    wchar_t message[MSG_SIZE];
    wchar_t filepath[MAX_PATH];
    wchar_t *filename, *last;
    size_t len, offset;

    const wchar_t *EXT = L".dmp";

    // Notify
    fputs("dump360 v2\n", stdout);

    // Loop over all the input parameters
    for (int i = 1; i < argc; i++)
    {
        filename = argv[i];
        fprintf(stdout, "Parsing '%ls'...\n", filename);

        // Determine the length
        len = wcslen(filename);
        if (len >= MAX_PATH)
        {
            fprintf(stderr, "Filepath '%ls' has a length of %zu but must be below %u\n",
                filename, len, MAX_PATH);
            continue;
        }

        // Copy the string into the buffer
        memset(filepath, 0, MAX_PATH * sizeof(wchar_t));
        wcscpy(filepath, filename);

        // Find the last dot
        last = wcsrchr(filepath, L'.');
        if (last == nullptr || wcscmp(last, L".ff"))
        {
            _snwprintf(message, MSG_SIZE, L"File '%ls' does not have the .FF extension. "
                "Are you sure it is a Fast File?", filename);

            int result = MessageBoxW(NULL, message, L"Should we continue?", MB_ICONQUESTION | MB_YESNO);
            if (result != IDYES)
            {
                fputs("Missing extension, skipping at the user's request.\n", stderr);
                continue;
            }
            else
            {
                fputs("Continuing parsing of file...\n", stderr);
            }
        }

        // Calculate the offset to the extension
        if (last == nullptr)
        {
            offset = wcslen(filepath);
        }
        else
        {
            offset = (last - filepath);
        }

        // Ensure MAX_PATH is not exceeded
        if ((offset + wcslen(EXT)) >= MAX_PATH)
        {
            fprintf(stderr, "Could not replace the extension of '%ls'. The resulting filepath "
                "would exceed the maximum permitted length.", filename);
            continue;
        }

        // Replace the extension
        wcscpy(filepath + offset, EXT);

        // Search and dump
        search(filename, filepath);
    }

    return 0;
}

/**
 * Searches the file at the input path for the ZLib best compression header and
 * if found it will try to decode it into the output file.
 * @param path_in The path of the input file.
 * @param path_out The path of the output file.
 * @return Zero if successful; otherwise, a non-zero value.
 */
void search(const wchar_t *path_in, const wchar_t *path_out)
{
    char buffer[SEEK_BUFFER];
    FILE *iFile, *oFile;
    size_t read;
    int offset;

    // Open the input file
    if (!_wfopen_s(&iFile, path_in, L"rb"))
    {
        read = fread(buffer, 1, SEEK_BUFFER, iFile);

        // Ensure we got the data
        if (!ferror(iFile) && read > 0)
        {
            // Look for the header
            for (offset = 0; offset < (read - 1); offset++)
            {
                // Best compression match
                if (buffer[offset] == (char)0x78 &&
                    (
                        buffer[offset+1] == (char)0xDA ||   /* Best - MW1 */
                        buffer[offset+1] == (char)0x01      /* Fast - WAW */
                    ))
                {
                    if (!fseek(iFile, offset, SEEK_SET))
                    {
                        // Open the output file
                        if (!_wfopen_s(&oFile, path_out, L"wb"))
                        {
                            int ret = inf(iFile, oFile);
                            if (ret == Z_OK)
                            {
                                fprintf(stdout, "Dumpted '%ls' to '%ls'\n", path_in, path_out);
                            }
                            else
                            {
                                zerr(ret);
                            }
                        }
                        else
                        {
                            fprintf(stderr, "Could not open output file '%ls' for writing.\n", path_out);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Could not reposition in file '%ls'.\n", path_in);
                    }

                    // Set to zero to prevent tripping over messages
                    offset = 0;
                    break;
                }
            }

            // Could not find ZLib best compression
            if (offset == (read - 1))
            {
                fprintf(stderr, "Could not find ZLib best compression header for '%ls'\n", path_in);
            }
        }

        fclose(iFile);
    }
    else
    {
        fprintf(stderr, "Could not open input file '%ls' for reading.\n", path_in);
    }
}


#define CHUNK 16384

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = (uInt)fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

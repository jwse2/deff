#include <stdio.h>
#include <assert.h>
#include <zlib.h>

/** zpipe.c - Example functions */
void zerr(int ret);
int def(FILE *source, FILE *dest, int level);

int wmain(int argc, wchar_t **argv)
{
    FILE *iFile;
    FILE *oFile;
    char buffer[] = {
        0x49, 0x57, 0x66, 0x66, // IWff
        0x75, 0x31, 0x30, 0x30, // u100
        0x05, 0x00, 0x00, 0x00  // 5 = MW1
    };

    if (argc != 3)
    {
        fputs("USAGE: linker.exe <path_in> <path_out>\n", stderr);
        return 0;
    }

    if (!_wfopen_s(&iFile, argv[1], L"rb"))
    {
        if (!_wfopen_s(&oFile, argv[2], L"wb"))
        {
            fwrite(buffer, 1, 12, oFile);

            int ret = def(iFile, oFile, Z_BEST_COMPRESSION);
            if (ret != Z_OK)
            {
                zerr(ret);
            }

            fclose(oFile);
        }
        else
        {
            fputs("Could not open output file for writing...\n", stderr);
        }

        fclose(iFile);
    }
    else
    {
        fputs("Could not open input file for reading...\n", stderr);
    }

    return 0;
}



#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = (uInt)fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
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

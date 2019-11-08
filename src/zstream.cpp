#include <cstdio>
#include <cstring>
#include <assert.h>
#include <exception>
#include <zlib.h>
#include "stream.hpp"
#include "zstream.hpp"

#define ZSTREAM_OK      0
#define ZSTREAM_FREAD   (-1)
#define ZSTREAM_ZERROR  (-2)
#define ZSTREAM_ZDATA   (-3)

ZLibStream::ZLibStream(std::FILE *source) :
    Stream(source)
{
    std::memset(&zStruct, 0, sizeof(z_stream));
    zStruct.zalloc = Z_NULL;
    zStruct.zfree = Z_NULL;
    zStruct.opaque = Z_NULL;
    zStruct.avail_in = 0;
    zStruct.next_in = Z_NULL;

    if (Z_OK != inflateInit(&zStruct))
    {
        throw new std::exception("Failed to initialize ZLib.\n");
    }
}

ZLibStream::~ZLibStream(void)
{
    inflateEnd(&zStruct);
}

int ZLibStream::FillIn(void)
{
    size_t read;

    read = fread_s(zBuffer, BUFFER_SIZE, 1, BUFFER_SIZE, source);
    if (ferror(source) || read == 0)
    {
        return ZSTREAM_FREAD;
    }

    zStruct.avail_in = (uInt)read;
    zStruct.next_in = (Bytef*)zBuffer;

    return ZSTREAM_OK;
}

int ZLibStream::FillOut(void)
{
    int result;

    zStruct.avail_out = BUFFER_SIZE;
    zStruct.next_out = (Bytef*)buffer;

    result = inflate(&zStruct, Z_NO_FLUSH);

    /* Fatal */
    if (result == Z_STREAM_ERROR)
    {
        return ZSTREAM_ZERROR;
    }

    /* Corrupted data */
    if (result == Z_NEED_DICT ||
        result == Z_DATA_ERROR ||
        result == Z_MEM_ERROR)
    {
        return ZSTREAM_ZDATA;
    }

    available = (BUFFER_SIZE - zStruct.avail_out);
    relPosition = 0;

    return ZSTREAM_OK;
}

int ZLibStream::Refill(void)
{
    int result;

    /* Double check */
    if (available == 0)
    {
        /* Refill the output stream. */
        result = FillOut();
        if (result)
        {
            return result;
        }

        /* Output buffer may be empty, in which case we need to refill the input. */
        if (available == 0)
        {
            /* Ensure we have zero input. */
            assert(zStruct.avail_in == 0);

            /* Refill the input stream. */
            result = FillIn();
            if (result)
            {
                return result;
            }

            /* Refill the output. */
            result = FillOut();
            if (result)
            {
                return result;
            }
        }
    }

    assert(available > 0);
    return ZSTREAM_OK;
}

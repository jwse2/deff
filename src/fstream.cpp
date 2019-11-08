#include <cstdio>
#include "stream.hpp"
#include "fstream.hpp"

FileStream::FileStream(std::FILE *source) :
    Stream(source)
{
    // Nothing...
}

FileStream::~FileStream(void)
{
    // Nothing...
}

int FileStream::Refill(void)
{
    size_t read;

    read = fread_s(buffer, BUFFER_SIZE, 1, BUFFER_SIZE, source);
    if (ferror(source) || read == 0)
    {
        return -1;
    }

    relPosition = 0;
    available = (int)read;

    return 0;
}

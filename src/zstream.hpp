#ifndef ZSTREAM_HPP
#define ZSTREAM_HPP

#include <cstdio>
#include <zlib.h>
#include "stream.hpp"

class ZLibStream : public Stream
{
public:
    ZLibStream(std::FILE *source);
    ~ZLibStream(void);

protected:
    int Refill(void);

private:
    int FillIn(void);
    int FillOut(void);

private:
    z_stream zStruct;
    char zBuffer[BUFFER_SIZE];
};

#endif /* ZSTREAM_HPP */

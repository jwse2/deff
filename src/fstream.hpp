#ifndef FSTREAM_HPP
#define FSTREAM_HPP

#include <cstdio>
#include "stream.hpp"

class FileStream : public Stream
{
public:
    FileStream(std::FILE *source);
    ~FileStream(void);

protected:
    int Refill(void);
};

#endif /* FSTREAM_HPP */

#ifndef STREAM_HPP
#define STREAM_HPP

#include <cstdio>

#define BUFFER_SIZE 16384

class Stream
{
public:
    Stream(std::FILE *source);
    ~Stream(void);

    int ReadString(char *dest, int max);
    int ReadMemory(void *dest, int size);

    int GetPosition(void);

protected:
    virtual int Refill(void) = 0;

protected:
    char buffer[BUFFER_SIZE];
    std::FILE *source;
    int absPosition;
    int relPosition;
    int available;
};

#endif /* STREAM_HPP */

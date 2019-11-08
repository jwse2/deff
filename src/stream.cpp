#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include "stream.hpp"

Stream::Stream(std::FILE *source)
{
    this->source = source;
    std::memset(buffer, 0, BUFFER_SIZE);
    this->absPosition = 0;
    this->relPosition = 0;
    this->available = 0;
}

Stream::~Stream(void)
{
    // Nothing ATM...s
}

int Stream::GetPosition(void)
{
    return absPosition;
}

int Stream::ReadString(char *dest, int max)
{
    int index;

    /* Keep looping till the maximum amount of characters. */
    for (index = 0; (max == -1 || index < max); index++)
    {
        /* Check for a refill. */
        if (available <= 0)
        {
            if (Refill())
            {
                throw std::exception("Could not refill STRING buffer.\n");
                return -1;
            }
        }

        /* Copy the character. */
        dest[index] = buffer[relPosition];

        /* Update the positions. */
        absPosition++;
        relPosition++;
        available--;

        /* Check for the string-terminator. */
        if (dest[index] == 0)
        {
            /* Return immediately. */
            return (index + 1);
        }
    }

    /* Ensure the string is zero-terminated. */
    if (max != -1 && index == max)
    {
        dest[max-1] = 0;
        throw std::exception("String buffer too small.\n");
    }

    return -1;
}

int Stream::ReadMemory(void *dest, int size)
{
    char *buffer = (char*)dest;
    int result, written;

    /* Keep a copy. */
    written = size;

    do
    {
        /* Check for a refill. */
        if (available <= 0)
        {
            result = Refill();
            if (result)
            {
                throw std::exception("Could not refill BYTE buffer.\n");
                return -1;
            }
        }

        if (size > available)
        {
            /* Copy the available bytes. */
            memcpy(buffer, this->buffer + relPosition, available);
            
            /* Update the destination to copy to. */
            buffer += available;

            /* Calculate the remaining number of bytes to copy. */
            size -= available;

            /* Update positions. */
            absPosition += available;
            relPosition += available;
            available = 0;
        }
        else
        {
            /* Copy the available bytes. */
            memcpy(buffer, this->buffer + relPosition, size);

            /* Update positions. */
            absPosition += size;
            relPosition += size;
            available -= size;

            /* Update the size variable. */
            size = 0;

            /* Return immediately. */
            return written;
        }
    }
    while (size > 0);

    /* An error occured. */
    return -1;
}

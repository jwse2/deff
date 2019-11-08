#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <stdexcept>
#include <cstdio>
#include <cstdarg>

#define NUM_SKIP_FRAMES         2       /* Frames to skip, using this setup it skips the code in the exception class. */
#define MAX_STACK_FRAMES        5       /* Maximum number of frames to write. */
#define MAX_STACK_LENGTH        256     /* Maximum length per string for each frame. */
#define MAX_MESSAGE_LENGTH      512     /* Maximum length of the default .what() message. */

struct StackTrace
{
public:
    StackTrace(void)
    {
        argc = 0;
        for (int i = 0; i < MAX_STACK_FRAMES; i++)
        {
            argv[i][0] = '\0';
        }
    }

    ~StackTrace(void)
    {
        ; // Do nothing
    }

    StackTrace(const StackTrace &other)
    {
        argc = other.argc;
        memcpy(argv, other.argv, (sizeof(char) * MAX_STACK_FRAMES * MAX_STACK_LENGTH));
        memcpy(length, other.length, (sizeof(int) * MAX_STACK_FRAMES));
    }

public:
    char argv[MAX_STACK_FRAMES][MAX_STACK_LENGTH];
    int length[MAX_STACK_FRAMES];
    int argc;
};


class Exception : public std::runtime_error
{
public:
    explicit Exception(const char *format, ...);
    explicit Exception(int line, const char *file, const char *format, ...);
    virtual ~Exception(void) noexcept;
    virtual const char * what(void) const noexcept;
    const struct StackTrace& stackTrace(void) const noexcept;

protected:
    char message[MAX_MESSAGE_LENGTH];
    struct StackTrace trace;
};


#endif /* EXCEPTION_HPP */

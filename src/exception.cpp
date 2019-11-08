#include <stdexcept>
#include <cstdio>
#include <cstdarg>

#include <Windows.h>
#include <dbghelp.h>
#pragma comment(lib, "DbgHelp.lib")

#include "exception.hpp"

#define TRACE_FORMAT \
    "FILE: %s\nLINE: %i\nADDR: 0x%012llX\nFUNC: %s\n"

#define MESSAGE_PREFIX \
    "\nFILE: %s\nLINE: %i\nMSG.: "



static void _StackTrace(struct StackTrace *trace)
{
    unsigned int    i;
    void            *stack[MAX_STACK_FRAMES];
    unsigned short  frames;
    HANDLE          process;
    DWORD           displacement;
    DWORD64         dwAddress;
    IMAGEHLP_LINE64 line;
    char            buffer[sizeof(SYMBOL_INFO) + (sizeof(CHAR) * MAX_STACK_LENGTH)];
    PSYMBOL_INFO    symbol = (PSYMBOL_INFO)buffer;
    int             length;

    // Defaults
    trace->argc = 0;

    process = GetCurrentProcess();
    if (SymInitialize(process, NULL, TRUE) == TRUE)
    {
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        frames = CaptureStackBackTrace(NUM_SKIP_FRAMES, MAX_STACK_FRAMES, stack, NULL);
        
        symbol->MaxNameLen   = (MAX_STACK_LENGTH - 1); // MSDN: minus for the zero-terminator
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        line.SizeOfStruct    = sizeof(IMAGEHLP_LINE64);

        for (i = 0; i < frames; i++)
        {
            dwAddress = (DWORD64)(stack[i]);

            // Load the all the information we wish to display.
            if (SymGetLineFromAddr64(process, dwAddress, &displacement, &line) == TRUE &&
                SymFromAddr(process, dwAddress, NULL, symbol) == TRUE)
            {
                // Print it formatted into the buffer.
                length = std::snprintf(
                    trace->argv[trace->argc],
                    MAX_STACK_LENGTH, TRACE_FORMAT,
                    line.FileName, line.LineNumber, symbol->Address, symbol->Name
                );

                // Cap the length
                if (length > MAX_STACK_LENGTH)
                {
                    length = MAX_STACK_LENGTH;
                }

                // Store the values.
                trace->length[trace->argc] = length;
                trace->argv[trace->argc][MAX_STACK_LENGTH - 1] = 0;
                trace->argc++;
            }
            else
            {
                // This error occurs when the .pdb file has not been found.
                if (GetLastError() == ERROR_INVALID_ADDRESS)
                {
                    break;
                }
            }
        }

        SymCleanup(process);
    }

    // NOTE: Not currently needed because of the 'virtual' handle value. But may
    //       be needed in the future and is thus here for safety reasons.
    CloseHandle(process);
}



Exception::Exception(const char *format, ...) :
    runtime_error(format)
{
    std::va_list argptr;

    // Generate the trace data
    _StackTrace(&trace);

    // Print the format message.
    va_start(argptr, format);
    std::vsnprintf(
        message,
        MAX_MESSAGE_LENGTH,
        format,
        argptr
    );
    va_end(argptr);

    // Ensure zero termination
    message[MAX_MESSAGE_LENGTH - 1] = 0;   
}

Exception::Exception(int line, const char *file, const char *format, ...) :
    runtime_error(format)
{
    char local[MAX_MESSAGE_LENGTH];
    char *cat;

    // Generate the trace data
    _StackTrace(&trace);

    // First create the prefix
    std::snprintf(local, MAX_MESSAGE_LENGTH, MESSAGE_PREFIX, file, line);
    cat = (local + std::strlen(local));

    std::va_list argptr;

    // Create the message
    va_start(argptr, format);
    std::vsnprintf(
        cat,
        MAX_MESSAGE_LENGTH,
        format,
        argptr
    );

    va_end(argptr);

    // Ensure zero termination
    message[MAX_MESSAGE_LENGTH - 1] = 0;

    // Copy to the destination
    strcpy_s(message, local);
}

Exception::~Exception(void) noexcept
{
    ; // Nothing to do here...
}

const char * Exception::what(void) const noexcept
{
    return message;
}

const struct StackTrace& Exception::stackTrace(void) const noexcept
{
    return trace;
}

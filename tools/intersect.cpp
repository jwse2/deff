#include <cstdio>
#include <stdexcept>
#include <assert.h>

#define BUFFER_SIZE         0x4000

/**
 * Hex to integer.
 */
int xtoi(wchar_t *str)
{
    int total, i;
    wchar_t c;
    bool isNegative;
    bool isHex;

    // Defaults
    isNegative = false;
    isHex = false;
    total = 0;

    // Negative/positive
    if (str[0] != L'\0' && str[0] == L'-')
    {
        isNegative = true;
        str++;
    }

    // Hexadecimal prefix
    if (str[0] != L'\0')
    {
        if (str[0] == L'0')
        {
            if (str[1] != L'\0' && (str[1] == L'x' || str[1] == L'X'))
            {
                str += 2;
                isHex = true;
            }
        }
        else if (str[0] == L'x' || str[0] == L'X')
        {
            str += 1;
            isHex = true;
        }
    }
    
    if (isHex)
    {
        // Convert hexadecimal to integer
        while ((c = *str++) != L'\0')
        {
            if (c >= L'0' && c <= '9')
            {
                i = (c - L'0');
            }
            else if (c >= L'A' && c <= L'F')
            {
                i = 10 + (c - L'A');
            }
            else
            {
                throw std::runtime_error("Invalid hexadecimal character input.");
            }

            total = ((total * 16) + i);
        }
    }
    else
    {
        // Convert from decimal to integer
        while ((c = *str++) != L'\0')
        {
            if (c >= L'0' && c <= '9')
            {
                i = (c - L'0');
            }
            else
            {
                throw std::runtime_error("Invalid decimal character input.");
            }

            total = ((total * 10) + i);
        }
    }

    return (isNegative == true ? (total * -1) : total);
}

int fsize(FILE *file)
{
    fpos_t pos;
    long int tell;

    if (fgetpos(file, &pos) || ferror(file))
        throw std::runtime_error("FILE::getpos");

    if (fseek(file, 0, SEEK_END) || ferror(file))
        throw std::runtime_error("FILE::seek");
    
    tell = ftell(file);
    if (tell == -1L || ferror(file))
        throw std::runtime_error("FILE::tell");
    
    if (fsetpos(file, &pos) || ferror(file))
        throw std::runtime_error("FILE::setpos");

    return (int)tell;
}

int wmain(int argc, wchar_t **argv)
{
    std::FILE *iFile, *oFile;
    int start, end, size;
    char buffer[BUFFER_SIZE];
    size_t read, write;

    if (argc != 5)
    {
        fputs("USAGE: intersect.exe <path_in> <path_out> <start> <end>\n", stdout);
        return 0;
    }

    // Open the input file.
    if (!_wfopen_s(&iFile, argv[1], L"rb"))
    {
        // Open the output file.
        if (!_wfopen_s(&oFile, argv[2], L"wb"))
        {
            try
            {
                // Calculate the start and end
                start = xtoi(argv[3]);
                end   = xtoi(argv[4]);
                size  = fsize(iFile);

                // Determine how much to read.
                if (end < 0)
                {
                    fprintf(stderr, "DEBUG: %i, %i, %i, ", start, end, size);
                    // We want to read till filesize - x bytes
                    size = ((size - start) + end); // NOTE: end is negative
                    fprintf(stderr, "%i\n", size);
                }
                else
                {
                    assert(end > start);

                    // We want to read the difference in bytes
                    size = (end - start);
                }

                // Reposition to the point where we want to read from.
                if (!fseek(iFile, start, SEEK_SET))
                {
                    do
                    {
                        read = fread(buffer, 1, BUFFER_SIZE, iFile);
                        if (ferror(iFile))
                            throw std::runtime_error("input error");

                        if (read > size)
                        {
                            read = (size_t)size;
                        }

                        write = fwrite(buffer, 1, read, oFile);
                        if (ferror(oFile))
                            throw std::runtime_error("output error");

                        if (read != write)
                            throw std::runtime_error("write error");

                        size -= (int)write;
                    }
                    while (read > 0 && write > 0 && size > 0);

                    // Check and notify
                    if (read <= 0 || write <= 0)
                    {
                        throw std::exception("Internal I/O error.");
                    }
                    else if (size != 0)
                    {
                        fprintf(stderr, "Could not create intersection, %i bytes are left to write.\n", size);
                    }
                }
                else
                {
                    fprintf(stderr, "Could not set position to '0x%X' in file '%ls'.\n", start, argv[1]);
                }
            }
            catch (const std::exception &ex)
            {
                fprintf(stderr, "EXCEPTION\n%s\n", ex.what());
            }

            fclose(oFile);
        }
        else
        {
            fprintf(stderr, "Could not write output file '%ls'.\n", argv[2]);
        }

        fclose(iFile);
    }
    else
    {
        fprintf(stderr, "Could not open input file '%ls'.\n", argv[1]);
    }

    return 0;
}

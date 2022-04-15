#include "png.h"
#include <algorithm>
#include <vector>
#include <map>
#include <string.h>
#include <string>
#include <iostream>

static std::string truncFileExt(const char* str)
{
    char* ext = strrchr((char*)str, '.');
    if (!ext) return str;
    if (strchr(ext, '/')) return str;
    return std::string(str, ext - str);
}

bool PrepareFont(unsigned long first_char, unsigned long charWidth, unsigned long charHeight, const char* outputFileName, const char* inputFileName)
{
    std::string name = truncFileExt(basename(inputFileName));

    Png png;
    if (!png.load(inputFileName)) return false;

    FILE* fo = fopen(outputFileName, "w");
    if (!fo)
    {
        std::cerr << "Can't create file " << outputFileName << std::endl;
        return false;
    }
    fprintf(fo, "#include \"fonts.h\"\n\n");

    fprintf(fo, "static const uint8_t font_data[] = {\n");
    unsigned n = 0;
    unsigned lc = png.getHeight() / charHeight;
    for (unsigned y = 0; y < charHeight * lc; y += (charHeight + 1))
    {
        for (unsigned x = 0; x <= png.getWidth() - charWidth ; x += (charWidth + 1))
        {
            uint8_t char_width = 0;                        
            for (unsigned ix = 0; ix < charWidth; ix++)
            {
                unsigned c = png.getPixel(x + ix, y);
                if (c != 0xFFFFFF) {
                    char_width = ix;
                }
            }

            uint8_t bytes_per_line = 0;
            unsigned long char_code = n + first_char;
            if ((char_code >= 32) && (char_code < 127)) {
                fprintf(fo, "     /* Code %u char '%c' */\n", (unsigned)char_code, (char)char_code);
            } else {
                fprintf(fo, "     /* Code %u */\n", (unsigned)char_code);
            }
            fprintf(fo, "     %u,\n", (char_width + 1));
            fprintf(fo, "    ");
            
            for (unsigned l = 0; l + 8 <= charHeight; l += 8)
            {
                for (unsigned ix = 0; ix < charWidth; ix++)
                {
                    if (bytes_per_line >= 16) {
                        fprintf(fo, "\n    ");
                        bytes_per_line = 0;
                    }
                    uint8_t b = 0;
                    for (unsigned iy = 0; iy + l < charHeight; iy++)
                    {
                        unsigned c = png.getPixel(x + ix, y + l + iy);
                        if (c == 0) b |= (1 << iy);
                    }
                    fprintf(fo, " 0x%02X,", b);
                    bytes_per_line++;
                }
            }
            fprintf(fo, "\n");
            n++;            
        }
    }
    
    unsigned bytes_per_char = charWidth * ((charHeight + 7) / 8) + 1;

    fprintf(fo, "};\n\n");
    fprintf(fo, "const Font %s = {\n", name.c_str());
    fprintf(fo, "    data: font_data,\n");
    fprintf(fo, "    first_char_code: %lu,\n", (unsigned long)first_char);
    fprintf(fo, "    chars_count: %lu,\n", (unsigned long)n);
    fprintf(fo, "    bytes_per_char: %lu,\n", (unsigned long)bytes_per_char);
    fprintf(fo, "    char_width: %lu,\n", (unsigned long)charWidth);
    fprintf(fo, "    char_height: %lu,\n", (unsigned long)charHeight);
    fprintf(fo, "};\n");

    fclose(fo);

    return true;
}

int main(int argc, char **argv)
{    
    if (argc != 6)
    {
        std::cerr << "preparefont (c) 15-09-2021 Alemorf" << std::endl
                  << "Syntax: " << argv[0] << " first_char_code char_width char_height output_file.c input_file.png" << std::endl;
        return 2;
    }
 
    char* end = nullptr;
    unsigned long first_char_code = strtoul(argv[1], &end, 0);
    if ((end == nullptr) || (end[0] != '\0') || (first_char_code == 0)) {
        std::cerr << "Incorrect first char code = " << argv[1] << std::endl;
        return 2;     
    }

    end = nullptr;
    unsigned long char_width = strtoul(argv[2], &end, 0);
    if ((end == nullptr) || (end[0] != '\0') || (char_width == 0)) {
        std::cerr << "Incorrect char width = " << argv[2] << std::endl;
        return 2;     
    }

    end = nullptr;
    unsigned long char_height = strtoul(argv[3], &end, 0);
    if ((end == nullptr) || (end[0] != '\0') || (char_height == 0)) {
        std::cerr << "Incorrect char height " << argv[3] << std::endl;
        return 2;     
    }
    
    return PrepareFont(first_char_code, char_width, char_height, argv[4], argv[5]) ? 0 : 1;
}

#include "RLE.hpp"
#include "FileUtils.hpp"

#include <iostream>

std::vector<unsigned char> encodeRLE(
    const std::vector<unsigned char>& data)
{
    std::vector<unsigned char> result;

    size_t i = 0;

    while (i < data.size()) {

        size_t run = 1;
        while (i + run < data.size() &&
               data[i] == data[i + run] &&
               run < 129)
        {
            run++;
        }

        if (run >= 2) {
            unsigned char L = 128 + (run - 2);
            result.push_back(L);
            result.push_back(data[i]);
            i += run;
        }
        else {
            size_t start = i;
            size_t diffCount = 0;

            while (i < data.size() &&
                   diffCount < 128)
            {
                if (i + 1 < data.size() &&
                    data[i] == data[i + 1])
                    break;

                diffCount++;
                i++;
            }

            // L = diffCount - 1 (0xxxxxxx)
            result.push_back(diffCount - 1);

            for (size_t j = 0; j < diffCount; ++j)
                result.push_back(data[start + j]);
        }
    }

    return result;
}


std::vector<unsigned char> decodeRLE(
    const std::vector<unsigned char>& data)
{
    std::vector<unsigned char> result;

    size_t i = 0;

    while (i < data.size()) {

        unsigned char L = data[i++];
        
        if (L & 0x80) {
            size_t count = (L & 0x7F) + 2;

            if (i >= data.size())
                break;

            unsigned char value = data[i++];

            for (size_t k = 0; k < count; ++k)
                result.push_back(value);
        }
        else {
            size_t count = (L & 0x7F) + 1;

            for (size_t k = 0; k < count && i < data.size(); ++k)
                result.push_back(data[i++]);
        }
    }

    return result;
}

int RLEEncodeFile(const std::string& inputFile,
                  const std::string& outputFileUser)
{
    std::vector<unsigned char> buffer;

    if (!readBinaryFile(inputFile, buffer))
        return 1;

    std::string outputFile =
        outputFileUser.empty()
        ? makeDefaultOutputName(inputFile, ".rle")
        : outputFileUser;

    std::vector<unsigned char> compressed =
        encodeRLE(buffer);

    if (!writeBinaryFile(outputFile, compressed))
        return 1;

    std::cout << "RLE encoding finished. Output: "
              << outputFile << "\n";

    return 0;
}

int RLEDecodeFile(const std::string& inputFile,
                  const std::string& outputFileUser)
{
    std::vector<unsigned char> buffer;

    if (!readBinaryFile(inputFile, buffer))
        return 1;

    std::string outputFile =
        outputFileUser.empty()
        ? makeDefaultOutputName(inputFile, ".decoded")
        : outputFileUser;

    std::vector<unsigned char> decompressed =
        decodeRLE(buffer);

    if (!writeBinaryFile(outputFile, decompressed))
        return 1;

    std::cout << "RLE decoding finished. Output: "
              << outputFile << "\n";

    return 0;
}

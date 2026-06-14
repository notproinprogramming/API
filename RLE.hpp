#ifndef RLE_HPP
#define RLE_HPP

#include <vector>
#include <string>

std::vector<unsigned char> encodeRLE(
    const std::vector<unsigned char>& data
);

std::vector<unsigned char> decodeRLE(
    const std::vector<unsigned char>& data
);

int RLEEncodeFile(const std::string& inputFile,
                  const std::string& outputFileUser);

int RLEDecodeFile(const std::string& inputFile,
                  const std::string& outputFileUser);

#endif

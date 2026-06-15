#pragma once
#include <cstdint>
#include <string>


// 0 - freeze dictionary
// 1 - reset dictionary when full
void LZWEncodeFile(const std::string& inputFile, const std::string& outputFile, uint16_t maxBits = 12, uint8_t resetMode = 1);

void LZWDecodeFile(const std::string& inputFile, const std::string& outputFile);

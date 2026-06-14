#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include <cstdint>
#include <string>

// вузол дерева Хафмана
struct HuffNode {
  uint64_t freq;
  int symbol;
  int left;
  int right;
};

// таблиця кодів: код + довжина в бітах
struct HuffCode {
  uint32_t bits;
  int len;
};

// розмір таблиці частот у байтах: 256 * 4
static constexpr size_t FREQ_TABLE_SIZE = 256 * sizeof(uint32_t);

int HuffmanEncodeFile(const std::string& inputFile, const std::string& outputFileUser);

int HuffmanDecodeFile(const std::string& inputFile, const std::string& outputFileUser);

#endif

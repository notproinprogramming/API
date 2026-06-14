#ifndef BITSEQ_HPP
#define BITSEQ_HPP

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

class BitWriter {
 public:
  BitWriter(const std::string& filename, bool append = false);
  ~BitWriter();

  void WriteBit(unsigned char bit);

  void WriteBitSequence(const std::vector<unsigned char>& data, size_t bitCount);

  void Close();

 private:
  std::ofstream out;
  unsigned char current_byte;
  int bit_pos;
};

class BitReader {
 public:
  BitReader(const std::string& filename, size_t skipBytes = 0);
  ~BitReader();

  // читання одного біта; повертає 0/1 або -1 при кінці файлу
  int ReadBit();

  std::vector<unsigned char> ReadBitSequence(size_t bitCount);

  void Close();

 private:
  std::ifstream in;
  unsigned char current_byte;
  int bit_pos;
};

void RunBitSeqDemo();

#endif

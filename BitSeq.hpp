#ifndef BITSEQ_HPP
#define BITSEQ_HPP

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

class BitWriter {
    public:
        BitWriter(const std::string &filename);
        ~BitWriter();

        void WriteBitSequence(const std::vector<unsigned char> &data,
                              size_t bitCount);

        void Close();

    private:
        std::ofstream out;
        unsigned char current_byte;
        int bit_pos;
};

class BitReader {
    public:
        BitReader(const std::string &filename);
        ~BitReader();

        std::vector<unsigned char> ReadBitSequence(size_t bitCount);

        void Close();

    private:
        std::ifstream in;
        unsigned char current_byte;
        int bit_pos;
};

void RunBitSeqDemo();

#endif

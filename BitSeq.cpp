#include "BitSeq.hpp"
#include <iomanip>
#include <iostream>

// ================= BitWriter =================

BitWriter::BitWriter(const std::string &filename)
    : current_byte(0),
      bit_pos(0) {
    out.open(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Помилка відкриття файлу для запису: " << filename << "\n";
    }
}

BitWriter::~BitWriter() { Close(); }

void BitWriter::WriteBitSequence(const std::vector<unsigned char> &data,
                                 size_t bitCount) {
    size_t bits_written = 0;
    size_t byte_idx = 0;
    size_t bit_idx_in_byte = 0;

    while (bits_written < bitCount) {
        unsigned char input_byte =
            (byte_idx < data.size()) ? data[byte_idx] : 0;
        unsigned char bit = (input_byte >> bit_idx_in_byte) & 1;

        current_byte |= (bit << bit_pos);
        bit_pos++;
        bits_written++;

        if (bit_pos == 8) {
            out.put(current_byte);
            current_byte = 0;
            bit_pos = 0;
        }

        bit_idx_in_byte++;
        if (bit_idx_in_byte == 8) {
            bit_idx_in_byte = 0;
            byte_idx++;
        }
    }
}

void BitWriter::Close() {
    if (out.is_open()) {
        if (bit_pos > 0) {
            out.put(current_byte);
            bit_pos = 0;
        }
        out.close();
    }
}

// ================= BitReader =================

BitReader::BitReader(const std::string &filename)
    : current_byte(0),
      bit_pos(8) {
    in.open(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Помилка відкриття файлу для читання: " << filename
                  << "\n";
    }
}

BitReader::~BitReader() { Close(); }

std::vector<unsigned char> BitReader::ReadBitSequence(size_t bitCount) {
    std::vector<unsigned char> result;
    size_t bits_read = 0;

    unsigned char out_byte = 0;
    size_t out_bit_pos = 0;

    while (bits_read < bitCount) {
        if (bit_pos == 8) {
            int c = in.get();
            if (c == EOF) {
                break;
            }
            current_byte = static_cast<unsigned char>(c);
            bit_pos = 0;
        }

        unsigned char bit = (current_byte >> bit_pos) & 1;

        out_byte |= (bit << out_bit_pos);

        bit_pos++;
        out_bit_pos++;
        bits_read++;

        if (out_bit_pos == 8) {
            result.push_back(out_byte);
            out_byte = 0;
            out_bit_pos = 0;
        }
    }

    if (out_bit_pos > 0) {
        result.push_back(out_byte);
    }

    return result;
}

void BitReader::Close() {
    if (in.is_open()) {
        in.close();
    }
}

// ================= Demonstration =================

void printHex(const std::vector<unsigned char> &vec, const std::string &name) {
    std::cout << name << " = [";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << (int)vec[i];
        if (i + 1 < vec.size())
            std::cout << ", ";
    }
    std::cout << std::dec << "]\n";
}

void RunBitSeqDemo() {
    std::string filename = "bitstream_test.bin";

    std::vector<unsigned char> a1 = {0xE1, 0x01};
    std::vector<unsigned char> a2 = {0xEE, 0x00};

    std::cout << "\n--- Запис у потік ---\n";
    {
        BitWriter writer(filename);
        writer.WriteBitSequence(a1, 9);
        writer.WriteBitSequence(a2, 9);
        std::cout << "Записано a1 (9 бітів) та a2 (9 бітів) у файл " << filename
                  << "\n";
    }

    std::cout << "\n--- Читання з потоку ---\n";
    {
        BitReader reader(filename);
        std::vector<unsigned char> b1 = reader.ReadBitSequence(11);
        std::vector<unsigned char> b2 = reader.ReadBitSequence(7);

        std::cout << "Очікувано:\nb1 = [E1, 05]\nb2 = [3B]\n\nОтримано:\n";
        printHex(b1, "b1");
        printHex(b2, "b2");
    }
}

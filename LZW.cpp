#include "LZW.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "BitSeq.hpp"
#include "FileUtils.hpp"

struct LZWHeader {
  uint16_t maxBits;
  uint8_t resetMode;
};

static void writeHeader(const std::string& file, const LZWHeader& h) {
  std::vector<unsigned char> buf;

  buf.push_back(h.maxBits & 0xFF);
  buf.push_back((h.maxBits >> 8) & 0xFF);
  buf.push_back(h.resetMode);

  writeBinaryFile(file, buf);
}

static bool readHeader(const std::string& file, LZWHeader& h) {
  std::vector<unsigned char> buf;
  if (!readBinaryFile(file, buf) || buf.size() < 3) return false;

  h.maxBits = buf[0] | (buf[1] << 8);
  h.resetMode = buf[2];
  return true;
}

// ================= ENCODE =================

void LZWEncodeFile(const std::string& inputFile, const std::string& outputFile, uint16_t maxBits, uint8_t resetMode) {
  std::vector<unsigned char> data;
  if (!readBinaryFile(inputFile, data)) return;

  writeHeader(outputFile, {maxBits, resetMode});

  BitWriter writer(outputFile, true);

  const uint32_t MAX_DICT_SIZE = 1u << maxBits;

  std::unordered_map<std::string, uint32_t> dict;

  // ініціалізація словника
  for (uint32_t i = 0; i < 256; i++) {
    dict[std::string(1, (char)i)] = i;
  }

  uint32_t dictSize = 256;

  std::string w;

  for (unsigned char c : data) {
    std::string wc = w + (char)c;

    if (dict.count(wc)) {
      w = wc;
    } else {
      uint32_t code = dict[w];

      // запис коду
      for (int i = maxBits - 1; i >= 0; --i) {
        writer.WriteBit((code >> i) & 1);
      }

      // додавання у словник
      if (dictSize < MAX_DICT_SIZE) {
        dict[wc] = dictSize++;
      } else if (resetMode == 1) {
        dict.clear();
        for (uint32_t i = 0; i < 256; i++) dict[std::string(1, (char)i)] = i;
        dictSize = 256;
      }

      w = std::string(1, (char)c);
    }
  }

  if (!w.empty()) {
    uint32_t code = dict[w];
    for (int i = maxBits - 1; i >= 0; --i) {
      writer.WriteBit((code >> i) & 1);
    }
  }

  writer.Close();
  std::ifstream check(outputFile, std::ios::binary | std::ios::ate);
  size_t compressedSize = 0;
  if (check) {
    compressedSize = static_cast<size_t>(check.tellg());
  }

  std::cout << "LZW: " << inputFile << "\n"
            << "  оригінал:    " << data.size() << " байт\n"
            << "  стиснутий:   " << compressedSize << " байт\n"
            << "  результат:   " << outputFile << "\n";
}

// ================= DECODE =================

void LZWDecodeFile(const std::string& inputFile, const std::string& outputFile) {
  LZWHeader h;
  if (!readHeader(inputFile, h)) {
    std::cerr << "Invalid LZW file\n";
    return;
  }

  BitReader reader(inputFile, 3);

  const uint32_t MAX_DICT_SIZE = 1u << h.maxBits;

  std::vector<std::string> dict;

  // init
  dict.resize(256);
  for (uint32_t i = 0; i < 256; i++) {
    dict[i] = std::string(1, (char)i);
  }

  uint32_t dictSize = 256;

  std::vector<unsigned char> output;

  auto readCode = [&](void) -> int {
    int code = 0;
    for (int i = 0; i < h.maxBits; i++) {
      int bit = reader.ReadBit();
      if (bit == -1) return -1;
      code = (code << 1) | bit;
    }
    return code;
  };

  int prevCodeRaw = readCode();
  if (prevCodeRaw == -1) return;

  uint32_t prevCode = static_cast<uint32_t>(prevCodeRaw);
  std::string prevStr = dict[prevCode];
  output.insert(output.end(), prevStr.begin(), prevStr.end());

  while (true) {
    int currCode = readCode();
    if (currCode == -1) break;

    uint32_t code = static_cast<uint32_t>(currCode);
    std::string entry;

    if (code < dictSize) {
      entry = dict[code];
    } else if (code == dictSize) {
      entry = prevStr + prevStr[0];
    } else {
      break;
    }

    output.insert(output.end(), entry.begin(), entry.end());

    // додаємо у словник
    if (dictSize < MAX_DICT_SIZE) {
      dict.push_back(prevStr + entry[0]);
      dictSize++;
    } else if (h.resetMode == 1) {
      dict.clear();
      dict.resize(256);
      for (uint32_t i = 0; i < 256; i++) dict[i] = std::string(1, (char)i);
      dictSize = 256;
    }

    prevStr = entry;
  }

  writeBinaryFile(outputFile, output);
}

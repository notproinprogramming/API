#include "LZW.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "BWT.hpp"
#include "BitSeq.hpp"
#include "FileUtils.hpp"
#include "MTF.hpp"

// прапорці у заголовку файлу (той самий підхід, що і в Huffman)
static constexpr uint8_t LZW_FLAG_BWT = 0x01;
static constexpr uint8_t LZW_FLAG_MTF = 0x02;

// заголовок: 1 байт прапорців + 2 байти maxBits + 1 байт resetMode = 4 байти
struct LZWHeader {
  uint8_t flags;
  uint16_t maxBits;
  uint8_t resetMode;
};

static void writeHeader(const std::string& file, const LZWHeader& h) {
  std::vector<unsigned char> buf(4);
  buf[0] = h.flags;
  buf[1] = h.maxBits & 0xFF;
  buf[2] = (h.maxBits >> 8) & 0xFF;
  buf[3] = h.resetMode;
  writeBinaryFile(file, buf);
}

static bool readHeader(const std::string& file, LZWHeader& h) {
  std::vector<unsigned char> buf;
  if (!readBinaryFile(file, buf) || buf.size() < 4) return false;
  h.flags = buf[0];
  h.maxBits = buf[1] | (buf[2] << 8);
  h.resetMode = buf[3];
  return true;
}

void LZWEncodeFile(const std::string& inputFile, const std::string& outputFile, uint16_t maxBits, uint8_t resetMode, bool useBWT, bool useMTF) {
  std::vector<unsigned char> data;
  if (!readBinaryFile(inputFile, data)) return;

  size_t originalSize = data.size();

  // препроцесинг
  if (useBWT) data = encodeBWT(data);
  if (useMTF) data = encodeMTF(data);

  uint8_t flags = 0;
  if (useBWT) flags |= LZW_FLAG_BWT;
  if (useMTF) flags |= LZW_FLAG_MTF;

  writeHeader(outputFile, {flags, maxBits, resetMode});

  BitWriter writer(outputFile, true);

  const uint32_t MAX_DICT_SIZE = 1u << maxBits;

  std::unordered_map<std::string, uint32_t> dict;
  for (uint32_t i = 0; i < 256; i++) dict[std::string(1, (char)i)] = i;

  uint32_t dictSize = 256;
  std::string w;

  for (unsigned char c : data) {
    std::string wc = w + (char)c;
    if (dict.count(wc)) {
      w = wc;
    } else {
      uint32_t code = dict[w];
      for (int i = maxBits - 1; i >= 0; --i) writer.WriteBit((code >> i) & 1);

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
    for (int i = maxBits - 1; i >= 0; --i) writer.WriteBit((code >> i) & 1);
  }

  writer.Close();

  std::ifstream check(outputFile, std::ios::binary | std::ios::ate);
  size_t compressedSize = check ? static_cast<size_t>(check.tellg()) : 0;

  std::cout << "LZW encode: " << inputFile << "\n"
            << "  BWT: " << (useBWT ? "так" : "ні") << "  MTF: " << (useMTF ? "так" : "ні") << "\n"
            << "  оригінал:  " << originalSize << " байт\n"
            << "  стиснутий: " << compressedSize << " байт\n"
            << "  результат: " << outputFile << "\n";
}

void LZWDecodeFile(const std::string& inputFile, const std::string& outputFile) {
  LZWHeader h;
  if (!readHeader(inputFile, h)) {
    std::cerr << "Невалідний LZW файл\n";
    return;
  }

  bool hasBWT = (h.flags & LZW_FLAG_BWT) != 0;
  bool hasMTF = (h.flags & LZW_FLAG_MTF) != 0;

  // заголовок тепер 4 байти
  BitReader reader(inputFile, 4);

  const uint32_t MAX_DICT_SIZE = 1u << h.maxBits;

  std::vector<std::string> dict(256);
  for (uint32_t i = 0; i < 256; i++) dict[i] = std::string(1, (char)i);

  uint32_t dictSize = 256;
  std::vector<unsigned char> output;

  auto readCode = [&]() -> int {
    int code = 0;
    for (int i = 0; i < h.maxBits; i++) {
      int bit = reader.ReadBit();
      if (bit == -1) return -1;
      code = (code << 1) | bit;
    }
    return code;
  };

  int prevCodeRaw = readCode();
  if (prevCodeRaw == -1) goto done;

  {
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
  }

done:
  // скасовуємо препроцесинг у зворотному порядку
  if (hasMTF) output = decodeMTF(output);
  if (hasBWT) output = decodeBWT(output);

  writeBinaryFile(outputFile, output);
  std::cout << "LZW decode: " << outputFile << " (" << output.size() << " байт)\n";
}

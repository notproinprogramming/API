#include "Huffman.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <queue>
#include <vector>

#include "BWT.hpp"
#include "BitSeq.hpp"
#include "FileUtils.hpp"
#include "MTF.hpp"

// BWT на файлах більше цього ліміту займе дуже багато RAM і часу
static constexpr size_t BWT_SIZE_LIMIT = 256 * 1024 * 1024;  // 256 МБ

struct NodeCmp {
  const std::vector<HuffNode>& pool;
  bool operator()(int a, int b) const { return pool[a].freq > pool[b].freq; }
};

static int buildTree(const std::array<uint32_t, 256>& freq, std::vector<HuffNode>& pool) {
  pool.clear();
  pool.reserve(512);

  std::priority_queue<int, std::vector<int>, NodeCmp> pq{NodeCmp{pool}};

  for (int i = 0; i < 256; i++) {
    if (freq[i] == 0) continue;
    int idx = static_cast<int>(pool.size());
    pool.push_back({freq[i], i, -1, -1});
    pq.push(idx);
  }

  if (pq.size() == 1) {
    int child = pq.top();
    pq.pop();
    int root = static_cast<int>(pool.size());
    pool.push_back({pool[child].freq, -1, child, -1});
    return root;
  }

  while (pq.size() > 1) {
    int a = pq.top();
    pq.pop();
    int b = pq.top();
    pq.pop();

    int parent = static_cast<int>(pool.size());
    pool.push_back({pool[a].freq + pool[b].freq, -1, a, b});
    pq.push(parent);
  }

  return pq.top();
}

static void buildCodes(const std::vector<HuffNode>& pool, int node, uint32_t bits, int depth, std::array<HuffCode, 256>& codes) {
  if (node < 0) return;

  const HuffNode& n = pool[node];

  if (n.symbol >= 0) {
    codes[n.symbol] = {bits, depth == 0 ? 1 : depth};
    return;
  }

  buildCodes(pool, n.left, (bits << 1) | 0, depth + 1, codes);
  buildCodes(pool, n.right, (bits << 1) | 1, depth + 1, codes);
}

int HuffmanEncodeFile(const std::string& inputFile, const std::string& outputFileUser, bool useBWT, bool useMTF) {
  std::vector<unsigned char> buf;
  if (!readBinaryFile(inputFile, buf)) return 1;

  size_t originalSize = buf.size();

  if (useBWT && originalSize > BWT_SIZE_LIMIT) {
    std::cerr << "Попередження: файл " << originalSize / (1024 * 1024) << " МБ,"
              << " BWT потребує ~" << (originalSize * 6) / (1024 * 1024) << " МБ RAM"
              << " i може працювати дуже довго.\n"
              << "Продовжити? (y/n): ";
    char c;
    std::cin >> c;
    std::cin.ignore();
    if (c != 'y' && c != 'Y') return 1;
  }

  if (useBWT) {
    std::cerr << "BWT encode...\n";
    buf = encodeBWT(buf);
  }
  if (useMTF) {
    std::cerr << "MTF encode...\n";
    buf = encodeMTF(buf);
  }

  std::array<uint32_t, 256> freq{};
  for (unsigned char b : buf) freq[b]++;

  std::vector<HuffNode> pool;
  std::array<HuffCode, 256> codes{};

  if (!buf.empty()) {
    int root = buildTree(freq, pool);
    buildCodes(pool, root, 0, 0, codes);
  }

  std::string outputFile = outputFileUser.empty() ? makeDefaultOutputName(inputFile, ".huf") : outputFileUser;

  {
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
      std::cerr << "Не вдалося відкрити: " << outputFile << "\n";
      return 1;
    }

    uint8_t flags = 0;
    if (useBWT) flags |= FLAG_BWT;
    if (useMTF) flags |= FLAG_MTF;
    out.put(static_cast<char>(flags));

    for (int i = 0; i < 256; i++) {
      uint32_t f = freq[i];
      out.write(reinterpret_cast<const char*>(&f), 4);
    }
  }

  {
    BitWriter writer(outputFile, true);
    for (unsigned char b : buf) {
      const HuffCode& c = codes[b];
      for (int i = c.len - 1; i >= 0; i--) {
        unsigned char bit = (c.bits >> i) & 1;
        writer.WriteBit(bit);
      }
    }
  }

  std::ifstream check(outputFile, std::ios::binary | std::ios::ate);
  size_t compressedSize = static_cast<size_t>(check.tellg());

  std::cout << "Huffman: " << inputFile << "\n"
            << "  BWT: " << (useBWT ? "так" : "ні") << "  MTF: " << (useMTF ? "так" : "ні") << "\n"
            << "  оригінал:    " << originalSize << " байт\n"
            << "  стиснутий:   " << compressedSize << " байт\n"
            << "  (таблиця:    " << FREQ_TABLE_SIZE << " байт)\n"
            << "  результат:   " << outputFile << "\n";

  return 0;
}

int HuffmanDecodeFile(const std::string& inputFile, const std::string& outputFileUser) {
  std::ifstream in(inputFile, std::ios::binary);
  if (!in) {
    std::cerr << "Не вдалося відкрити: " << inputFile << "\n";
    return 1;
  }

  uint8_t flags = static_cast<uint8_t>(in.get());
  bool hasBWT = (flags & FLAG_BWT) != 0;
  bool hasMTF = (flags & FLAG_MTF) != 0;

  std::array<uint32_t, 256> freq{};
  for (int i = 0; i < 256; i++) {
    uint32_t f = 0;
    in.read(reinterpret_cast<char*>(&f), 4);
    freq[i] = f;
  }
  in.close();

  uint64_t totalSymbols = 0;
  for (uint32_t f : freq) totalSymbols += f;

  std::vector<HuffNode> pool;
  std::array<HuffCode, 256> codes{};

  int root = -1;
  if (totalSymbols > 0) {
    root = buildTree(freq, pool);
    buildCodes(pool, root, 0, 0, codes);
  }

  std::string outputFile = outputFileUser.empty() ? makeDefaultOutputName(inputFile, ".decoded") : outputFileUser;

  std::vector<unsigned char> result;
  result.reserve(static_cast<size_t>(totalSymbols));

  // зсув: 1 байт прапорців + 256*4 байт таблиці частот
  static constexpr size_t DATA_OFFSET = 1 + FREQ_TABLE_SIZE;

  {
    BitReader reader(inputFile, DATA_OFFSET);

    int cur = root;
    uint64_t decoded = 0;

    while (decoded < totalSymbols) {
      int bit = reader.ReadBit();
      if (bit < 0) break;

      if (pool[cur].symbol >= 0) {
        result.push_back(static_cast<unsigned char>(pool[cur].symbol));
        decoded++;
        cur = root;
        continue;
      }

      if (bit == 0)
        cur = pool[cur].left;
      else
        cur = pool[cur].right;

      if (cur < 0) break;

      if (pool[cur].symbol >= 0) {
        result.push_back(static_cast<unsigned char>(pool[cur].symbol));
        decoded++;
        cur = root;
      }
    }
  }

  if (hasMTF) {
    std::cerr << "MTF decode...\n";
    result = decodeMTF(result);
  }
  if (hasBWT) {
    std::cerr << "BWT decode...\n";
    result = decodeBWT(result);
  }

  if (!writeBinaryFile(outputFile, result)) return 1;

  std::cout << "Huffman decode: " << outputFile << " (" << result.size() << " байт)\n";

  return 0;
}

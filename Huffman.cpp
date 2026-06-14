#include "Huffman.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <queue>
#include <vector>

#include "BitSeq.hpp"
#include "FileUtils.hpp"

// побудова дерева
//
// порівнювач для черги з пріоритетом - менша частота = вищий пріоритет
struct NodeCmp {
  const std::vector<HuffNode>& pool;
  bool operator()(int a, int b) const { return pool[a].freq > pool[b].freq; }
};

// будуємо дерево Гафмана з таблиці частот, повертає індекс кореня в pool
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

  // крайній випадок - файл з одним унікальним байтом
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

int HuffmanEncodeFile(const std::string& inputFile, const std::string& outputFileUser) {
  std::vector<unsigned char> buf;
  if (!readBinaryFile(inputFile, buf)) return 1;

  // рахуємо частоти
  std::array<uint32_t, 256> freq{};
  for (unsigned char b : buf) freq[b]++;

  // будуємо дерево і таблицю кодів
  std::vector<HuffNode> pool;
  std::array<HuffCode, 256> codes{};

  if (!buf.empty()) {
    int root = buildTree(freq, pool);
    buildCodes(pool, root, 0, 0, codes);
  }

  std::string outputFile = outputFileUser.empty() ? makeDefaultOutputName(inputFile, ".huf") : outputFileUser;

  // відкриваємо вихідний файл і пишемо таблицю частот (256 * 4 байти)
  {
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
      std::cerr << "Не вдалося відкрити: " << outputFile << "\n";
      return 1;
    }
    for (int i = 0; i < 256; i++) {
      uint32_t f = freq[i];
      out.write(reinterpret_cast<const char*>(&f), 4);
    }
  }

  // дописуємо стиснені дані бітовим потоком
  {
    BitWriter writer(outputFile, true /* append */);
    for (unsigned char b : buf) {
      const HuffCode& c = codes[b];
      // пишемо біти від старшого до молодшого
      for (int i = c.len - 1; i >= 0; i--) {
        unsigned char bit = (c.bits >> i) & 1;
        writer.WriteBit(bit);
      }
    }
  }

  // рахуємо розмір стиснутого файлу
  std::ifstream check(outputFile, std::ios::binary | std::ios::ate);
  size_t compressedSize = static_cast<size_t>(check.tellg());

  std::cout << "Huffman: " << inputFile << "\n"
            << "  оригінал:    " << buf.size() << " байт\n"
            << "  стиснутий:   " << compressedSize << " байт\n"
            << "  (таблиця:    " << FREQ_TABLE_SIZE << " байт)\n"
            << "  результат:   " << outputFile << "\n";

  return 0;
}

// ---------------------------------------------------------------------------
// декодування
// ---------------------------------------------------------------------------

int HuffmanDecodeFile(const std::string& inputFile, const std::string& outputFileUser) {
  std::ifstream in(inputFile, std::ios::binary);
  if (!in) {
    std::cerr << "Не вдалося відкрити: " << inputFile << "\n";
    return 1;
  }

  // читаємо таблицю частот
  std::array<uint32_t, 256> freq{};
  for (int i = 0; i < 256; i++) {
    uint32_t f = 0;
    in.read(reinterpret_cast<char*>(&f), 4);
    freq[i] = f;
  }
  in.close();

  // відновлюємо загальну кількість символів
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

  // читаємо бітовий потік після таблиці
  {
    BitReader reader(inputFile, FREQ_TABLE_SIZE);

    int cur = root;
    uint64_t decoded = 0;

    while (decoded < totalSymbols) {
      int bit = reader.ReadBit();
      if (bit < 0) break;

      // якщо дерево з одного вузла - він і є листком
      if (pool[cur].symbol >= 0) {
        result.push_back(static_cast<unsigned char>(pool[cur].symbol));
        decoded++;
        cur = root;
        // цей біт треба ще опрацювати - але при одному унікальному
        // символі код завжди 0-довжини, тому просто ігноруємо біт
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

  if (!writeBinaryFile(outputFile, result)) return 1;

  std::cout << "Huffman decode: " << outputFile << " (" << result.size() << " байт)\n";

  return 0;
}

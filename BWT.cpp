#include "BWT.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>

// розмір блоку - 900 Кб, як у bzip2
// на великих файлах BWT наївним сортуванням просто не завершиться
static constexpr size_t BWT_BLOCK_SIZE = 900 * 1024;

// порівнювач циклічних зсувів без операції %
// замість (a+k)%n використовуємо два діапазони - дає суттєвий приріст швидкості
static bool cyclicLess(const unsigned char* data, size_t n, uint32_t a, uint32_t b) {
  if (a == b) return false;

  const unsigned char* pa = data + a;
  const unsigned char* pb = data + b;

  size_t leftA = n - a;
  size_t leftB = n - b;
  size_t minLeft = std::min(leftA, leftB);

  // перший відрізок до кінця масиву
  int r = memcmp(pa, pb, minLeft);
  if (r != 0) return r < 0;

  // один з вказівників досяг кінця, порівнюємо залишок
  if (leftA < leftB) {
    // a обернувся, pb ще не
    r = memcmp(data, pb + leftA, n - leftB);
    if (r != 0) return r < 0;
    return memcmp(data + (n - leftB), data, leftB) < 0;
  } else if (leftB < leftA) {
    r = memcmp(pa + leftB, data, n - leftA);
    if (r != 0) return r < 0;
    return memcmp(data, data + (n - leftA), leftA) < 0;
  }

  // обидва обернулися одночасно - рядки рівні
  return false;
}

// BWT одного блоку, повертає трансформований блок + origRow в кінці (4 байти)
static std::vector<unsigned char> bwtEncodeBlock(const unsigned char* data, size_t n) {
  std::vector<uint32_t> idx(n);
  std::iota(idx.begin(), idx.end(), 0);

  std::sort(idx.begin(), idx.end(), [&](uint32_t a, uint32_t b) { return cyclicLess(data, n, a, b); });

  std::vector<unsigned char> result(n + 4);
  uint32_t origRow = 0;

  for (size_t i = 0; i < n; i++) {
    result[i] = data[(idx[i] + n - 1) % n];
    if (idx[i] == 0) origRow = static_cast<uint32_t>(i);
  }

  result[n + 0] = (origRow >> 0) & 0xFF;
  result[n + 1] = (origRow >> 8) & 0xFF;
  result[n + 2] = (origRow >> 16) & 0xFF;
  result[n + 3] = (origRow >> 24) & 0xFF;

  return result;
}

// зворотне BWT одного блоку
static std::vector<unsigned char> bwtDecodeBlock(const unsigned char* data, size_t n) {
  uint32_t origRow = (uint32_t)data[n + 0] | ((uint32_t)data[n + 1] << 8) | ((uint32_t)data[n + 2] << 16) | ((uint32_t)data[n + 3] << 24);

  const unsigned char* L = data;

  uint32_t freq[256] = {};
  for (size_t i = 0; i < n; i++) freq[L[i]]++;

  uint32_t cumul[256] = {};
  for (int i = 1; i < 256; i++) cumul[i] = cumul[i - 1] + freq[i - 1];

  std::vector<uint32_t> T(n);
  uint32_t cnt[256] = {};
  for (size_t i = 0; i < n; i++) {
    unsigned char c = L[i];
    T[i] = cumul[c] + cnt[c];
    cnt[c]++;
  }

  std::vector<unsigned char> result(n);
  uint32_t pos = origRow;
  for (size_t i = n; i-- > 0;) {
    result[i] = L[pos];
    pos = T[pos];
  }

  return result;
}

// формат файлу з кількома блоками:
// [4 байти - кількість блоків]
// для кожного блоку: [4 байти - розмір блоку без origRow][блок даних + 4 байти origRow]
std::vector<unsigned char> encodeBWT(const std::vector<unsigned char>& data) {
  if (data.empty()) return {};

  size_t n = data.size();
  size_t numBlocks = (n + BWT_BLOCK_SIZE - 1) / BWT_BLOCK_SIZE;

  std::vector<unsigned char> result;
  // резервуємо приблизно стільки ж місця + заголовки
  result.reserve(n + numBlocks * 8 + 4);

  // кількість блоків
  uint32_t nb = static_cast<uint32_t>(numBlocks);
  result.push_back((nb >> 0) & 0xFF);
  result.push_back((nb >> 8) & 0xFF);
  result.push_back((nb >> 16) & 0xFF);
  result.push_back((nb >> 24) & 0xFF);

  for (size_t b = 0; b < numBlocks; b++) {
    size_t offset = b * BWT_BLOCK_SIZE;
    size_t blockLen = std::min(BWT_BLOCK_SIZE, n - offset);

    std::cerr << "BWT encode: блок " << (b + 1) << "/" << numBlocks << " (" << blockLen << " байт)\n";

    auto encoded = bwtEncodeBlock(data.data() + offset, blockLen);

    // розмір блоку (без 4 байтів origRow)
    uint32_t sz = static_cast<uint32_t>(blockLen);
    result.push_back((sz >> 0) & 0xFF);
    result.push_back((sz >> 8) & 0xFF);
    result.push_back((sz >> 16) & 0xFF);
    result.push_back((sz >> 24) & 0xFF);

    result.insert(result.end(), encoded.begin(), encoded.end());
  }

  return result;
}

std::vector<unsigned char> decodeBWT(const std::vector<unsigned char>& data) {
  if (data.size() < 4) return {};

  size_t pos = 0;
  uint32_t numBlocks = (uint32_t)data[pos] | ((uint32_t)data[pos + 1] << 8) | ((uint32_t)data[pos + 2] << 16) | ((uint32_t)data[pos + 3] << 24);
  pos += 4;

  std::vector<unsigned char> result;

  for (uint32_t b = 0; b < numBlocks; b++) {
    if (pos + 4 > data.size()) break;

    uint32_t blockLen = (uint32_t)data[pos] | ((uint32_t)data[pos + 1] << 8) | ((uint32_t)data[pos + 2] << 16) | ((uint32_t)data[pos + 3] << 24);
    pos += 4;

    if (pos + blockLen + 4 > data.size()) break;

    auto decoded = bwtDecodeBlock(data.data() + pos, blockLen);
    result.insert(result.end(), decoded.begin(), decoded.end());

    pos += blockLen + 4;
  }

  return result;
}

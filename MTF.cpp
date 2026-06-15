#include "MTF.hpp"

#include <list>
#include <vector>

std::vector<unsigned char> encodeMTF(const std::vector<unsigned char>& data) {
  // початковий алфавіт: 0..255 у порядку зростання
  std::list<unsigned char> alphabet;
  for (int i = 0; i < 256; i++) alphabet.push_back(static_cast<unsigned char>(i));

  std::vector<unsigned char> result;
  result.reserve(data.size());

  for (unsigned char c : data) {
    // шукаємо символ і запам'ятовуємо його індекс
    unsigned char idx = 0;
    auto it = alphabet.begin();
    while (*it != c) {
      ++it;
      ++idx;
    }

    result.push_back(idx);

    // переносимо символ на початок
    alphabet.erase(it);
    alphabet.push_front(c);
  }

  return result;
}

std::vector<unsigned char> decodeMTF(const std::vector<unsigned char>& data) {
  std::list<unsigned char> alphabet;
  for (int i = 0; i < 256; i++) alphabet.push_back(static_cast<unsigned char>(i));

  std::vector<unsigned char> result;
  result.reserve(data.size());

  for (unsigned char idx : data) {
    // беремо символ з позиції idx
    auto it = alphabet.begin();
    std::advance(it, idx);
    unsigned char c = *it;

    result.push_back(c);

    // переносимо на початок
    alphabet.erase(it);
    alphabet.push_front(c);
  }

  return result;
}

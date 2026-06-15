#ifndef BWT_HPP
#define BWT_HPP

#include <cstdint>
#include <vector>

// BWT - перетворення Барроуза-Вілера
// повертає трансформований буфер, в кінці зберігає 4 байти - індекс оригінального рядка
std::vector<unsigned char> encodeBWT(const std::vector<unsigned char>& data);

// зворотне BWT, очікує формат з encodeBWT (4 байти індексу в кінці)
std::vector<unsigned char> decodeBWT(const std::vector<unsigned char>& data);

#endif

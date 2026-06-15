#ifndef MTF_HPP
#define MTF_HPP

#include <vector>

// MTF - Move-to-Front ("стопка книг")
// перетворює буфер, замінюючи кожен байт його індексом у поточному алфавіті
std::vector<unsigned char> encodeMTF(const std::vector<unsigned char>& data);

std::vector<unsigned char> decodeMTF(const std::vector<unsigned char>& data);

#endif

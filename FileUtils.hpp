#ifndef FILEUTILS_HPP
#define FILEUTILS_HPP

#include <vector>
#include <string>

bool readBinaryFile(const std::string& filename,
                    std::vector<unsigned char>& buffer);

bool writeBinaryFile(const std::string& filename,
                     const std::vector<unsigned char>& buffer);

bool writeTextFile(const std::string& filename,
                   const std::string& content);

std::string makeDefaultOutputName(const std::string& input,
                                  const std::string& extension);

#endif

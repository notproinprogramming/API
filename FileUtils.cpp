#include "FileUtils.hpp"
#include <fstream>
#include <iostream>

bool readBinaryFile(const std::string& filename,
                    std::vector<unsigned char>& buffer) {

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open input file: " << filename << "\n";
        return false;
    }

    buffer.assign(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );

    return true;
}

bool writeBinaryFile(const std::string& filename,
                     const std::vector<unsigned char>& buffer) {

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot open output file: " << filename << "\n";
        return false;
    }

    out.write(reinterpret_cast<const char*>(buffer.data()),
              buffer.size());

    return true;
}

bool writeTextFile(const std::string& filename,
                   const std::string& content) {

    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Cannot open output file: " << filename << "\n";
        return false;
    }

    out << content;
    return true;
}

std::string makeDefaultOutputName(const std::string& input,
                                  const std::string& extension) {

    return input + extension;
}

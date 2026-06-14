#include "Base64.hpp"
#include "FileUtils.hpp"

#include <iostream>
#include <fstream>

static const std::string BASE64_ALPHABET =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static int DECODE_TABLE[256];


void initDecodeTable() {
    for (int i = 0; i < 256; ++i)
        DECODE_TABLE[i] = -1;

    for (int i = 0; i < 64; ++i)
        DECODE_TABLE[(unsigned char)BASE64_ALPHABET[i]] = i;
}



std::string encodeBase64(const std::vector<unsigned char>& data) {
    std::string result;
    int val = 0, valb = -6;

    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;

        while (valb >= 0) {
            result.push_back(
                BASE64_ALPHABET[(val >> valb) & 0x3F]
            );
            valb -= 6;
        }
    }

    if (valb > -6) {
        result.push_back(
            BASE64_ALPHABET[((val << 8) >> (valb + 8)) & 0x3F]
        );
    }

    while (result.size() % 4)
        result.push_back('=');

    return result;
}

bool decodeBase64Line(const std::string& line,
                      std::vector<unsigned char>& output,
                      int lineNumber,
                      bool& paddingEncountered) {

    int val = 0, valb = -8;
    bool localPadding = false;

    for (size_t i = 0; i < line.size(); ++i) {
        unsigned char c = line[i];

        if (c == '=') {
            localPadding = true;
            paddingEncountered = true;
            continue;
        }

        if (localPadding) {
            std::cerr << "Row " << lineNumber
                      << ", position " << i + 1
                      << ": Incorrect padding usage\n";
            return false;
        }

        if (DECODE_TABLE[c] == -1) {
            std::cerr << "Row " << lineNumber
                      << ", position " << i + 1
                      << ": Invalid character ('" << c << "')\n";
            return false;
        }

        val = (val << 6) + DECODE_TABLE[c];
        valb += 6;

        if (valb >= 0) {
            output.push_back(
                static_cast<unsigned char>((val >> valb) & 0xFF)
            );
            valb -= 8;
        }
    }

    return true;
}

int Base64EncodeFile(const std::string& inputFile,
                     const std::string& outputFileUser) {

    std::vector<unsigned char> buffer;

    if (!readBinaryFile(inputFile, buffer))
        return 1;

    std::string outputFile =
        outputFileUser.empty()
        ? makeDefaultOutputName(inputFile, ".base64")
        : outputFileUser;

    std::string encoded = encodeBase64(buffer);

    std::string finalOutput = "- Base64 encoded file\n";

    for (size_t i = 0; i < encoded.size(); i += 76)
        finalOutput += encoded.substr(i, 76) + "\n";

    if (!writeTextFile(outputFile, finalOutput))
        return 1;

    std::cout << "Encoding finished. Output: "
              << outputFile << "\n";

    return 0;
}
int Base64DecodeFile(const std::string& inputFile,
                     const std::string& outputFileUser) {

    std::ifstream in(inputFile);
    if (!in) {
        std::cerr << "Cannot open input file\n";
        return 1;
    }

    std::string outputFile =
        outputFileUser.empty()
        ? makeDefaultOutputName(inputFile, ".decoded")
        : outputFileUser;

    std::vector<unsigned char> decodedData;

    std::string line;
    int lineNumber = 0;
    bool paddingEncountered = false;
    bool finished = false;

    while (std::getline(in, line)) {
        lineNumber++;

        if (line.empty() || line[0] == '-')
            continue;

        if (finished) {
            std::cerr << "Data found after padding\n";
            break;
        }

        if (line.length() % 4 != 0) {
            std::cerr << "Row " << lineNumber
                      << ": Length not multiple of 4\n";
            return 1;
        }

        if (!decodeBase64Line(line, decodedData,
                              lineNumber,
                              paddingEncountered))
            return 1;

        if (paddingEncountered)
            finished = true;
    }

    if (!writeBinaryFile(outputFile, decodedData))
        return 1;

    std::cout << "Decoding finished. Output: "
              << outputFile << "\n";

    return 0;
}

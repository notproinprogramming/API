#include "Base64.hpp"
#include "BitSeq.hpp"
#include "RLE.hpp"
#include <cstdlib>
#include <iostream>

int main() {

    initDecodeTable();

    while (true) {
        std::cout << "\n=== MENU ===\n";
        std::cout << "0) Exit\n";
        std::cout << "1) Base64 encode file\n";
        std::cout << "2) Base64 decode file\n";
        std::cout << "3) RLE encode file\n";
        std::cout << "4) RLE decode file\n";
        std::cout << "5) Тест бінарних потоків (Приклад з лабораторної)\n";
        std::cout << "9) Open .base64 file in nvim\n";

        int choose;
        std::cin >> choose;

        if (choose == 0)
            break;

        if (choose == 1) {
            std::string in, out;
            std::cout << "Input file: ";
            std::cin >> in;
            std::cin.ignore();
            std::cout << "Output file (empty = default): ";
            std::getline(std::cin, out);

            Base64EncodeFile(in, out);
        } else if (choose == 2) {
            std::string in, out;
            std::cout << "Encoded file: ";
            std::cin >> in;
            std::cin.ignore();
            std::cout << "Output file (empty = default): ";
            std::getline(std::cin, out);

            Base64DecodeFile(in, out);
        } else if (choose == 3) {
            std::string in, out;
            std::cout << "Input file: ";
            std::cin >> in;
            std::cin.ignore();
            std::cout << "Output file (empty = default): ";
            std::getline(std::cin, out);

            RLEEncodeFile(in, out);
        } else if (choose == 4) {
            std::string in, out;
            std::cout << "Encoded file: ";
            std::cin >> in;
            std::cin.ignore();
            std::cout << "Output file (empty = default): ";
            std::getline(std::cin, out);

            RLEDecodeFile(in, out);
        } else if (choose == 5) {
            RunBitSeqDemo();
        } else if (choose == 9) {
            std::string file;
            std::cout << "File to open: ";
            std::cin >> file;

            std::string command = "nvim " + file;
            system(command.c_str());
        }
    }

    return 0;
}

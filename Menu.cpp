#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "Base64.hpp"
#include "BitSeq.hpp"
#include "Huffman.hpp"
#include "RLE.hpp"

namespace fs = std::filesystem;

static const std::string DIR_MY = "my files";
static const std::string DIR_ENCODED = "encoded files";
static const std::string DIR_DECODED = "decoded files";

// створюємо папки при першому запуску якщо їх нема
static void ensureDirs() {
  for (const auto& d : {DIR_MY, DIR_ENCODED, DIR_DECODED}) fs::create_directories(d);
}

// виводимо вміст папки у вигляді нумерованого списку
// повертаємо вектор шляхів для вибору по номеру
static std::vector<fs::path> listDir(const std::string& dir) {
  std::vector<fs::path> entries;

  for (const auto& e : fs::directory_iterator(dir))
    if (e.is_regular_file()) entries.push_back(e.path());

  std::sort(entries.begin(), entries.end());

  if (entries.empty()) {
    std::cout << "  (порожньо)\n";
    return entries;
  }

  for (size_t i = 0; i < entries.size(); i++) std::cout << "  " << (i + 1) << ") " << entries[i].filename().string() << "\n";

  return entries;
}

// вибір файлу з папки за номером або ручне введення імені
static std::string pickFile(const std::string& dir) {
  std::cout << "\nФайли у \"" << dir << "\":\n";
  auto entries = listDir(dir);

  std::cout << "Номер або ім'я файлу: ";
  std::string input;
  std::cin >> input;
  std::cin.ignore();

  // якщо ввели число - беремо з списку
  try {
    int idx = std::stoi(input);
    if (idx >= 1 && idx <= static_cast<int>(entries.size())) return entries[idx - 1].string();
  } catch (...) {
  }

  // інакше вважаємо що ввели ім'я файлу в цій папці
  return dir + "/" + input;
}

// будуємо вихідний шлях: кладемо файл у потрібну папку зі своїм розширенням
static std::string makeOutPath(const std::string& dir, const std::string& inputPath, const std::string& ext) {
  fs::path p(inputPath);
  return dir + "/" + p.filename().string() + ext;
}

// --- підменю Base64 ---
static void menuBase64() {
  std::cout << "\n  1) Encode\n  2) Decode\n> ";
  int sub;
  std::cin >> sub;
  std::cin.ignore();

  if (sub == 1) {
    std::string in = pickFile(DIR_MY);
    std::string out = makeOutPath(DIR_ENCODED, in, ".base64");
    Base64EncodeFile(in, out);

  } else if (sub == 2) {
    std::string in = pickFile(DIR_ENCODED);
    std::string out = makeOutPath(DIR_DECODED, in, ".decoded");
    Base64DecodeFile(in, out);
  }
}

// --- підменю RLE ---
static void menuRLE() {
  std::cout << "\n  1) Encode\n  2) Decode\n  > ";
  int sub;
  std::cin >> sub;
  std::cin.ignore();

  if (sub == 1) {
    std::string in = pickFile(DIR_MY);
    std::string out = makeOutPath(DIR_ENCODED, in, ".rle");
    RLEEncodeFile(in, out);

  } else if (sub == 2) {
    std::string in = pickFile(DIR_ENCODED);
    std::string out = makeOutPath(DIR_DECODED, in, ".decoded");
    RLEDecodeFile(in, out);
  }
}

static void menuHuffman() {
  std::cout << "\n  1) Encode\n  2) Decode\n> ";
  int sub;
  std::cin >> sub;
  std::cin.ignore();

  if (sub == 1) {
    std::string in = pickFile(DIR_MY);
    std::string out = makeOutPath(DIR_ENCODED, in, ".huf");
    HuffmanEncodeFile(in, out);

  } else if (sub == 2) {
    std::string in = pickFile(DIR_ENCODED);
    std::string out = makeOutPath(DIR_DECODED, in, ".decoded");
    HuffmanDecodeFile(in, out);
  }
}

static void menuNvim() {
  std::cout << "\n  1) my files\n  2) encoded files\n  3) decoded files\n> ";
  int sub;
  std::cin >> sub;
  std::cin.ignore();

  std::string dir;
  if (sub == 1)
    dir = DIR_MY;
  else if (sub == 2)
    dir = DIR_ENCODED;
  else if (sub == 3)
    dir = DIR_DECODED;
  else
    return;

  std::string file = pickFile(dir);
  system(("nvim " + file).c_str());
}

int main() {
  ensureDirs();
  initDecodeTable();

  while (true) {
    std::cout << "\n=== MENU ===\n";
    std::cout << "0 - Exit\n";
    std::cout << "1 - Base64    (1 encode, 2 decode)\n";
    std::cout << "2 - RLE       (1 encode, 2 decode)\n";
    std::cout << "3 - Huffman   (1 encode, 2 decode)\n";
    std::cout << "9 - nvim\n";
    std::cout << "10 - Test (BitSeq demo)\n";
    std::cout << "> ";

    int choose;
    if (!(std::cin >> choose)) break;
    std::cin.ignore();

    if (choose == 0)
      break;
    else if (choose == 1)
      menuBase64();
    else if (choose == 2)
      menuRLE();
    else if (choose == 3)
      menuHuffman();
    else if (choose == 9)
      menuNvim();
    else if (choose == 10)
      RunBitSeqDemo();
  }

  return 0;
}

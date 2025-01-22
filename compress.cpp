#include <fstream>
#include <iostream>

#include "arithmetic_coding.cpp"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <input> <output>\n";
    return 1;
  }

  auto in_filename = argv[1];
  auto out_filename = argv[2];

  std::ifstream ifs(in_filename, std::ios::binary);
  std::ofstream ofs(out_filename, std::ios::binary);

  if (!ifs || !ofs) {
    std::cerr << "failed to open files\n";
    return 1;
  }

  arithmetic_coder<32, 16> coder;
  coder.encode(ifs, ofs);

  return 0;
}
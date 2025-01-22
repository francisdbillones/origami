#pragma once

#include <cstdint>
#include <fstream>

class bit_writer {
private:
  std::ostream &output;
  uint8_t buffer;
  int bits_used;

public:
  bit_writer(std::ostream &out) : output(out), buffer(0), bits_used(0) {}

  void write_bit(uint8_t bit) {
    buffer = (buffer << 1) | (bit & 1);
    bits_used++;

    if (bits_used == 8) {
      output.put(static_cast<char>(buffer));
      buffer = 0;
      bits_used = 0;
    }
  }

  void write_n_bits(uint64_t bits, int n) {
    for (int i = n - 1; i >= 0; i--) {
      write_bit((bits >> i) & 1);
    }
  }

  void clean() {
    if (bits_used > 0) {
      buffer <<= (8 - bits_used);
      output.put(static_cast<char>(buffer));
    }
  }
};
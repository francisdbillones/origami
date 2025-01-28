#pragma once

#include <cstdint>
#include <fstream>

class bit_reader {
private:
  std::istream &input;
  uint8_t buffer;
  int bits_remaining;

public:
  bit_reader(std::istream &in) : input(in), buffer(0), bits_remaining(0) {}

  uint8_t read_bit() {
    if (bits_remaining == 0) {
      char c;
      input.get(c);
      buffer = static_cast<uint8_t>(c);
      bits_remaining = 8;
    }

    uint8_t bit = (buffer >> (bits_remaining - 1)) & 1;
    bits_remaining--;
    return bit;
  }

  uint64_t read_n_bits(int n) {
    uint64_t result = 0;
    for (int i = 0; i < n; i++) {
      result = (result << 1) | read_bit();
    }
    return result;
  }

  uint64_t peek_bits(int n) {
    if (n > bits_remaining) {
      char c;
      input.get(c);
      buffer = static_cast<uint8_t>(c);
      bits_remaining = 8;
    }
    if (n > bits_remaining)
      return -1;
    return buffer >> (bits_remaining - n);
  }

  void consume_bits(int n) {
    if (n > bits_remaining) {
      bits_remaining = 0;
      buffer = 0;
    } else {
      bits_remaining -= n;
      buffer &= (1ULL << bits_remaining) - 1;
    }
  }

  bool at_eof() const { return input.eof() && bits_remaining == 0; }
};
#pragma once

#include <cstdint>
#include <fstream>

class bit_reader {
private:
  std::ifstream &input;
  uint64_t buffer = 0;
  int bits_in_buffer = 0;
  bool eof = false;

  void fill_buffer() {
    while (bits_in_buffer < 64 && !eof) {
      uint8_t byte;
      if (input.get((char &)byte)) {
        buffer = (buffer << 8) | byte;
        bits_in_buffer += 8;
      } else
        eof = true;
    }
  }

public:
  explicit bit_reader(std::ifstream &in) : input(in) { fill_buffer(); }

  int read_bit() {
    if (bits_in_buffer == 0) {
      fill_buffer();
      if (bits_in_buffer == 0)
        return -1;
    }

    bool bit = (buffer >> (bits_in_buffer - 1)) & 1;
    bits_in_buffer--;
    buffer &= (1ULL << bits_in_buffer) - 1;
    return bit;
  }

  int read_n_bits(int n) {
    int result = 0;
    if (n > bits_in_buffer)
      fill_buffer();

    if (n > bits_in_buffer)
      return -1;

    result = (buffer >> (bits_in_buffer - n)) & ((1ULL << n) - 1);
    bits_in_buffer -= n;
    buffer &= (1ULL << bits_in_buffer) - 1;
    return result;
  }

  int peek_bits(int n) {
    if (n > bits_in_buffer)
      fill_buffer();
    if (n > bits_in_buffer)
      return -1;
    return buffer >> (bits_in_buffer - n);
  }

  void consume_bits(int n) {
    if (n > bits_in_buffer) {
      bits_in_buffer = 0;
      buffer = 0;
    } else {
      bits_in_buffer -= n;
      buffer &= (1ULL << bits_in_buffer) - 1;
    }
  }

  bool at_eof() const { return eof && bits_in_buffer == 0; }
};
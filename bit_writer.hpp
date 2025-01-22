#pragma once

#include <cstdint>
#include <fstream>

class bit_writer {
private:
  std::ofstream &output;
  uint64_t buffer = 0;
  int bits_in_buffer = 0;

  // assumes we're flushing with a multiple of 8 bits
  void flush_buffer() {
    for (int i = 0; i < bits_in_buffer / 8; i++) {
      uint8_t byte = buffer >> (bits_in_buffer - 8 * (i + 1));
      output.put(byte);
      buffer &= (1ULL << bits_in_buffer) - 1;
    }
    // this will be wrong if we're not flushing with a multiple of 8 bits
    bits_in_buffer = 0;
  }

  void flush_remaining() {
    // pad remaining bits with zeros and flush
    if (bits_in_buffer > 0) {
      buffer <<= 8 - (bits_in_buffer % 8);
      bits_in_buffer += 8 - (bits_in_buffer % 8);
      flush_buffer();
    }
  }

public:
  explicit bit_writer(std::ofstream &out) : output(out) {}

  void write_bit(bool bit) {
    buffer = (buffer << 1) | bit;
    bits_in_buffer++;

    if (bits_in_buffer == 64) {
      flush_buffer();
    }
  }

  //   void write_n_bits(uint64_t bits, int n) {
  //     if (n > 64)
  //       throw std::runtime_error("Cannot write more than 64 bits");
  //     int bits_to_write = std::min(n, 64 - bits_in_buffer);
  //     buffer = (buffer << bits_to_write) | (bits >> (n - bits_to_write));
  //     bits_in_buffer += bits_to_write;
  //     if (bits_in_buffer == 64)
  //       flush_buffer();
  //     write_n_bits(bits & ((1ULL << (n - bits_to_write)) - 1), n -
  //     bits_to_write);
  //   }

  void clean() { flush_remaining(); }

  ~bit_writer() { clean(); }
};
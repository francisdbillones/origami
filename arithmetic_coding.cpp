#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "bit_reader.hpp"
#include "bit_writer.hpp"
#include "byte_static_model.cpp"

#define SMOOTHING_CONSTANT 0.1

template <int CODE_BITS, int FREQ_BITS> class arithmetic_coder {
private:
  static constexpr uint64_t HALF = 1ULL << (CODE_BITS - 1);
  static constexpr uint64_t ONE_FOURTH = 1ULL << (CODE_BITS - 2);
  static constexpr uint64_t THREE_FOURTHS = 3ULL << (CODE_BITS - 2);
  static constexpr uint64_t MAX_CODE = (1ULL << CODE_BITS) - 1;
  static constexpr uint64_t MAX_FREQ = (1ULL << FREQ_BITS) - 1;

  byte_static_model<FREQ_BITS> model;

public:
  arithmetic_coder() : model() {}

  void encode(std::ifstream &input, std::ofstream &output) {
    bit_writer writer(output);
    uint64_t low = 0;
    uint64_t high = MAX_CODE;
    int pending_bits = 0;

    uint16_t byte;
    while (1) {
      input.get((char &)byte);
      if (input.eof())
        byte = model.EOF_BYTE;

      auto value = model.get_probability(byte);
      model.update(byte);

      uint64_t range = high - low + 1;
      high = low + ((range * value.high) / value.count) - 1;
      low = low + ((range * value.low) / value.count);

      while (1) {
        if (high < HALF) {
          writer.write_bit(0);
          writer.write_n_bits((1 << pending_bits) - 1, pending_bits);
          pending_bits = 0;
        } else if (low >= HALF) {
          writer.write_bit(1);
          writer.write_n_bits(0, pending_bits);
          pending_bits = 0;
        } else if (low >= ONE_FOURTH && high < THREE_FOURTHS) {
          pending_bits++;
          low -= ONE_FOURTH;
          high -= ONE_FOURTH;
        } else
          break;

        high = (high << 1) | 1;
        low <<= 1;
        high &= MAX_CODE;
        low &= MAX_CODE;
      }

      if (byte == model.EOF_BYTE)
        break;
    }

    pending_bits++;
    if (low < ONE_FOURTH) {
      writer.write_bit(0);
      writer.write_n_bits((1 << pending_bits) - 1, pending_bits);
    } else {
      writer.write_bit(1);
      writer.write_n_bits(0, pending_bits);
    }

    writer.clean();
  }

  void decode(std::ifstream &input, std::ofstream &output) {
    bit_reader reader(input);

    uint64_t high = MAX_CODE;
    uint64_t low = 0;
    uint64_t value = 0;
    value = reader.read_n_bits(CODE_BITS);

    while (1) {
      uint64_t range = high - low + 1;
      uint64_t scaled_value =
          ((value - low + 1) * model.get_count() - 1) / range;

      auto [byte, prob] = model.get_byte_and_range(scaled_value);
      if (byte == model.EOF_BYTE)
        break;

      model.update(byte);
      output.put(byte);

      high = low + (range * prob.high) / prob.count - 1;
      low = low + (range * prob.low) / prob.count;

      while (1) {
        if (high < HALF) {
        } else if (low >= HALF) {
          value -= HALF;
          low -= HALF;
          high -= HALF;
        } else if (low >= ONE_FOURTH && high < THREE_FOURTHS) {
          value -= ONE_FOURTH;
          low -= ONE_FOURTH;
          high -= ONE_FOURTH;
        } else
          break;

        low <<= 1;
        high = (high << 1) | 1;
        value = (value << 1) | (reader.read_bit() == 1 ? 1 : 0);
      }
    }
  }

  void reset() { model.reset(); }
};

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "ByteStaticModel.cpp"

#define SMOOTHING_CONSTANT 0.1
#define CODE_BITS 32
#define FREQ_BITS 14
#define MAX_CODE (1ULL << CODE_BITS) - 1
#define MAX_FREQ (1ULL << FREQ_BITS) - 1
#define HALF 1ULL << (CODE_BITS - 1)
#define ONE_FOURTH 1ULL << (CODE_BITS - 2)
#define THREE_FOURTHS 3ULL << (CODE_BITS - 2)

void put_bit_plus_pending(bool bit, int &pending_bits,
                          std::vector<bool> &bits) {
  bits.push_back(bit);

  for (int i = 0; i < pending_bits; ++i)
    bits.push_back(!bit);
  pending_bits = 0;
}

std::vector<bool> arithmetic_encode(std::vector<int> bytes) {
  ByteStaticModel model(FREQ_BITS);

  uint64_t low = 0;
  uint64_t high = MAX_CODE;

  uint64_t range;

  std::vector<bool> output_bits;
  int pending_bits = 0;

  for (int byte : bytes) {
    auto prob = model.get_probability(byte);
    model.update(byte);
    range = high - low + 1;
    high = low + ((range * prob.high) / prob.count) - 1;
    low = low + ((range * prob.low) / prob.count);

    while (1) {
      if (high < HALF) {
        put_bit_plus_pending(0, pending_bits, output_bits);
      } else if (low >= HALF) {
        put_bit_plus_pending(1, pending_bits, output_bits);
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
  }
  pending_bits++;
  if (low < ONE_FOURTH)
    put_bit_plus_pending(0, pending_bits, output_bits);
  else
    put_bit_plus_pending(1, pending_bits, output_bits);
  return output_bits;
}

std::vector<int> arithmetic_decode(std::vector<bool> bits, unsigned int size) {
  std::vector<int> result;
  ByteStaticModel model(FREQ_BITS);

  uint64_t high = MAX_CODE;
  uint64_t low = 0;
  uint64_t value = 0;

  for (int i = 0; i < CODE_BITS; ++i) {
    value = (value << 1) | bits[i];
  }
  int bits_read = CODE_BITS;
  for (int i = 0; i < size; ++i) {
    uint64_t range = high - low + 1;
    uint64_t scaled_value = ((value - low + 1) * model.get_count() - 1) / range;
    auto [byte, prob] = model.get_byte_and_range(scaled_value);
    model.update(byte);
    result.push_back(byte);
    high = low + (range * prob.high) / prob.count - 1;
    low = low + (range * prob.low) / prob.count;
    while (1) {
      if (high < HALF) {
        // do nothing
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
      value = (value << 1) | bits[bits_read++];
    }
  }
  return result;
}

int main(int argc, char *argv[]) {
  std::ifstream ifs;
  ifs.open(argv[1], std::ifstream::in);

  std::vector<int> bytes;
  while (ifs.good()) {
    bytes.push_back(ifs.get());
  }

  auto bits = arithmetic_encode(bytes);

  auto decoded_bytes = arithmetic_decode(bits, bytes.size());

  std::cout << std::string(decoded_bytes.begin(), decoded_bytes.end())
            << std::endl;
  std::cout << "original: " << bytes.size() * 8 << " bits" << std::endl;
  std::cout << "encoded: " << bits.size() << std::endl;

  std::cout << "compression ratio: " << bits.size() / (bytes.size() * 8.)
            << std::endl;
  return 0;
}
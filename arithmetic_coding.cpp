#include <algorithm>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "ByteStaticModel.cpp"

#define SMOOTHING_CONSTANT 0.1
#define MAX_CODE 0xFFFFFFFFFFFFU
#define HALF 0x800000000000U
#define ONE_FOURTH 0x400000000000U
#define THREE_FOURTHS 0xC00000000000U

void put_bit_plus_pending(bool bit, int &pending_bits,
                          std::vector<bool> &bits) {
  bits.push_back(bit);

  for (int i = 0; i < pending_bits; ++i)
    bits.push_back(!bit);
  pending_bits = 0;
}

std::vector<bool> arithmetic_encode(std::vector<uint8_t> bytes,
                                    ByteStaticModel &model) {

  uint64_t low = 0;
  uint64_t high = MAX_CODE;

  uint64_t range;
  frac byte_low, byte_high;

  std::vector<bool> output_bits;
  int pending_bits = 0;

  for (uint8_t byte : bytes) {
    if (byte == 0)
      byte_low = {0, 1};
    else
      byte_low = model.get_probability(byte - 1);
    byte_high = model.get_probability(byte);
    range = high - low + 1;
    high = low + (range * byte_high.num) / byte_high.dem - 1;
    low = low + (range * byte_low.num) / byte_low.dem;

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

std::vector<uint8_t> arithmetic_decode(std::vector<bool> bits,
                                       unsigned int size,
                                       ByteStaticModel &model) {
  std::vector<uint8_t> result;

  uint64_t high = MAX_CODE;
  uint64_t low = 0;
  uint64_t value = 0;

  for (int i = 0; i < 48; ++i) {
    value = (value << 1) | bits[i];
  }
  int bits_read = 48;
  for (int i = 0; i < size; ++i) {
    uint64_t range = high - low + 1;
    uint64_t scaled_value = ((value - low + 1) * model.count - 1) / range;
    auto [byte, prob] = model.get_byte_and_range(scaled_value);
    result.push_back(byte);
    high = low + (range * prob.second.num) / prob.second.dem - 1;
    low = low + (range * prob.first.num) / prob.first.dem;
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
      high = (high << 1) + 1;
      value = (value << 1) | bits[bits_read++];
    }
  }
  return result;
}

int main() {

  std::string message =
      " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Ut eleifend "
      "ex arcu, vel ornare turpis cursus id. Integer luctus vulputate ligula, "
      "et convallis ligula eleifend nec. Phasellus et pellentesque enim, in "
      "luctus urna. Aliquam fringilla vitae tortor nec fermentum. Morbi "
      "convallis dolor vel lectus dignissim luctus. Proin commodo, tellus ac "
      "scelerisque mattis, tellus ante elementum ex, eget lacinia lectus "
      "tortor id sem. In hac habitasse platea dictumst. Ut molestie sit amet "
      "libero viverra feugiat. Aenean sodales, mauris nec fermentum mollis, "
      "leo elit lacinia magna, et euismod urna diam sed nibh. Proin id erat a "
      "mi posuere dignissim. Pellentesque a sodales dui, in vestibulum ante. "
      "Proin viverra dignissim felis. Lorem ipsum dolor sit amet, consectetur "
      "adipiscing elit. ";

  std::vector<uint8_t> bytes(message.begin(), message.end());
  ByteStaticModel model(bytes, {1, 10});

  auto bits = arithmetic_encode(bytes, model);

  std::cout << std::endl;

  auto decoded_bytes = arithmetic_decode(bits, message.length(), model);

  std::cout << std::string(decoded_bytes.begin(), decoded_bytes.end())
            << std::endl;
  std::cout << "original: " << bytes.size() * 8 << " bits" << std::endl;
  std::cout << "encoded: " << bits.size() << " bits" << std::endl;
  ;
}
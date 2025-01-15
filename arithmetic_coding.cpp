#include <algorithm>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "ByteStaticModel.cpp"

#define SMOOTHING_CONSTANT 0.1

std::pair<double, double> arithmetic_encode(std::vector<uint8_t> bytes,
                                            ByteStaticModel &model) {

  double low = 0.0;
  double high = 1.0;

  double byte_low, byte_high, range;
  for (uint8_t byte : bytes) {
    if (byte == 0)
      byte_low = 0.0;
    else
      byte_low = model.get_probability(byte - 1);
    byte_high = model.get_probability(byte);
    range = high - low;
    high = low + range * byte_high;
    low = low + range * byte_low;
    std::cout << low << " " << high << std::endl;
  }
  return {low, high};
}

std::vector<uint8_t> arithmetic_decode(double value, unsigned int size,
                                       ByteStaticModel &model) {
  std::vector<uint8_t> result;

  for (unsigned int i = 0; i < size; i++) {
    auto [byte, range] = model.get_byte_and_range(value);
    result.push_back(byte);
    value = (value - range.first) / (range.second - range.first);
  }

  return result;
}

int main() {
  std::string message = "i am francis";

  std::vector<uint8_t> bytes(message.begin(), message.end());
  ByteStaticModel model(bytes, SMOOTHING_CONSTANT);

  auto [low, high] = arithmetic_encode(bytes, model);

  std::cout << low << " " << high << std::endl;

  for (auto byte : bytes) {
    std::cout << (char)byte << " " << model.get_probability(byte) << std::endl;
  }

  auto decoded_bytes =
      arithmetic_decode((high + low) / 2, message.length(), model);

  std::cout << std::string(decoded_bytes.begin(), decoded_bytes.end())
            << std::endl;
  ;
}
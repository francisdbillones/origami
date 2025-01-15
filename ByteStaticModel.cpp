#include <cstdint>
#include <iostream>
#include <vector>

class ByteStaticModel {
private:
  double probabilities[0xFF];
  double cumulative_probabilities[0xFF];

public:
  ByteStaticModel(const std::vector<uint8_t> &bytes,
                  double smoothing_constant) {
    uint64_t freqs[0xFF] = {0};
    for (uint8_t byte : bytes) {
      freqs[byte]++;
    }

    for (int i = 0; i < 0xFF; ++i) {
      probabilities[i] = (freqs[i] + smoothing_constant) /
                         (bytes.size() + 0xFF * smoothing_constant);
    }
    cumulative_probabilities[0] = probabilities[0];
    for (int i = 1; i < 0xFF; ++i) {
      cumulative_probabilities[i] =
          cumulative_probabilities[i - 1] + probabilities[i];
    }
  }

  double get_probability(uint8_t byte) const {
    return cumulative_probabilities[byte];
  }
  std::pair<uint8_t, std::pair<double, double>>
  get_byte_and_range(double prob) const {
    if (prob < probabilities[0])
      return {0, {0.0, probabilities[0]}};

    for (int i = 1; i < 0xFF; ++i) {
      if (prob < cumulative_probabilities[i]) {
        return {(uint8_t)i,
                {cumulative_probabilities[i - 1], cumulative_probabilities[i]}};
      }
    }
  }
};

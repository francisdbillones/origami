#include <cstdint>
#include <iostream>
#include <vector>

typedef struct frac {
  uint64_t num;
  uint64_t dem;
} frac;

class ByteStaticModel {
private:
  frac probabilities[256];
  frac cumulative_probabilities[256];

public:
  uint64_t count;
  ByteStaticModel(const std::vector<uint8_t> &bytes, frac smoothing_constant) {
    uint64_t freqs[256] = {0};
    for (uint8_t byte : bytes) {
      freqs[byte]++;
    }

    for (int i = 0; i < 256; ++i) {
      probabilities[i] = {freqs[i], bytes.size()};
    }
    cumulative_probabilities[0] = probabilities[0];
    for (int i = 1; i < 256; i++) {
      cumulative_probabilities[i] = {cumulative_probabilities[i - 1].num +
                                         probabilities[i].num,
                                     bytes.size()};
    }

    count = bytes.size();
  }

  frac get_probability(uint8_t byte) const {
    return cumulative_probabilities[byte];
  }
  std::pair<uint8_t, std::pair<frac, frac>>
  get_byte_and_range(uint64_t prob) const {
    if (prob < cumulative_probabilities[0].num) {
      return {0, {{0, count}, cumulative_probabilities[0]}};
    }
    for (int i = 1; i < 256; ++i) {
      if (prob < cumulative_probabilities[i].num)
        return {i,
                {cumulative_probabilities[i - 1], cumulative_probabilities[i]}};
    };
  }
};

#include <cstdint>
#include <iostream>
#include <vector>

typedef struct prob {
  uint64_t low;
  uint64_t high;
  uint64_t count;
} frac;
class ByteStaticModel {
private:
  uint64_t cumulative_probabilities[258];
  bool frozen;
  int freq_bits;

public:
  ByteStaticModel(const int freq_bits) {
    this->freq_bits = freq_bits;
    this->frozen = false;

    for (int i = 0; i < 258; ++i) {
      this->cumulative_probabilities[i] = i;
    }
  }

  prob get_probability(const int byte) const {
    return {this->cumulative_probabilities[byte],
            this->cumulative_probabilities[byte + 1],
            this->cumulative_probabilities[257]};
  }
  std::pair<int, prob> get_byte_and_range(uint64_t value) const {
    for (int i = 0; i < 257; i++) {
      if (value < this->cumulative_probabilities[i + 1]) {
        return {i,
                {this->cumulative_probabilities[i],
                 this->cumulative_probabilities[i + 1], get_count()}};
      }
    }
  }

  bool update(int byte) {
    if (this->frozen)
      return false;
    for (int i = byte + 1; i < 258; ++i)
      this->cumulative_probabilities[i]++;

    if (this->cumulative_probabilities[257] >= ((1ULL << this->freq_bits) - 1))
      frozen = true;
    return true;
  }

  uint64_t get_count() const { return this->cumulative_probabilities[257]; }
};

#include <cstdint>
#include <iostream>
#include <vector>

typedef struct prob {
  uint64_t low;
  uint64_t high;
  uint64_t count;
} prob;

template <int FREQ_BITS> class byte_static_model {
private:
  uint64_t cumulative_probabilities[258];
  bool frozen;

  static constexpr uint64_t MAX_FREQ = (1ULL << FREQ_BITS) - 1;

public:
  static constexpr uint16_t EOF_BYTE = 0xFF;
  byte_static_model() { reset(); }

  prob get_probability(const uint8_t byte) const {
    return {cumulative_probabilities[byte], cumulative_probabilities[byte + 1],
            cumulative_probabilities[257]};
  }
  std::pair<uint64_t, prob> get_byte_and_range(const uint64_t value) const {
    int i;
    for (i = 0; i < 257; i++) {
      if (value < cumulative_probabilities[i + 1])
        break;
    }

    return {i,
            {cumulative_probabilities[i], cumulative_probabilities[i + 1],
             get_count()}};
  }

  void reset() {
    frozen = false;

    for (int i = 0; i < 258; ++i)
      cumulative_probabilities[i] = i;
  }

  bool update(const uint8_t byte) {
    if (this->frozen)
      return false;
    for (int i = byte + 1; i < 258; ++i)
      cumulative_probabilities[i]++;

    if (cumulative_probabilities[257] >= MAX_FREQ)
      frozen = true;
    return true;
  }

  inline uint64_t get_count() const { return cumulative_probabilities[257]; }
};

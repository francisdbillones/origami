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

  static constexpr uint64_t MAX_FREQ = (1ULL << FREQ_BITS) - 1;

public:
  bool normalized;
  static constexpr uint16_t EOF_BYTE = 0xFF;
  byte_static_model() { reset(); }

  inline prob get_probability(const uint8_t byte) const {
    return {cumulative_probabilities[byte], cumulative_probabilities[byte + 1],
            get_count()};
  }
  std::pair<uint64_t, prob> get_byte_and_range(const uint64_t value) const {
    int left = 0;
    int right = 257;
    while (left + 1 < right) {
      int mid = (left + right) / 2;
      if (cumulative_probabilities[mid] <= value)
        left = mid;
      else
        right = mid;
    }

    return {left, get_probability(left)};
  }

  void reset() {
    normalized = false;
    for (int i = 0; i < 258; ++i)
      cumulative_probabilities[i] = i;
  }

  bool update(const uint8_t byte) {
    if (this->normalized)
      return false;
    for (int i = byte + 1; i < 258; ++i)
      cumulative_probabilities[i]++;

    if (get_count() == MAX_FREQ) {
      normalize_frequencies();
    }
    return true;
  }

  inline uint64_t get_count() const { return cumulative_probabilities[257]; }

  void normalize_frequencies() {
    uint64_t frequencies[257];
    uint64_t total = 0;

    // extract frequencies from cumulative
    for (int i = 0; i < 257; i++) {
      frequencies[i] =
          cumulative_probabilities[i + 1] - cumulative_probabilities[i];
      total += frequencies[i];
    }

    // scale everything up to use FREQ_BITS precision
    uint64_t scale = (1ULL << FREQ_BITS) / total;
    uint64_t remaining = (1ULL << FREQ_BITS);

    for (int i = 0; i < 257; i++) {
      frequencies[i] *= scale;
      remaining -= frequencies[i];
    }

    // distribute any leftover probability
    while (remaining > 0) {
      for (int i = 0; i < 257 && remaining > 0; i++) {
        if (frequencies[i] > 0) {
          frequencies[i]++;
          remaining--;
        }
      }
    }

    // recompute cumulative frequencies
    cumulative_probabilities[0] = 0;
    for (int i = 0; i < 257; i++) {
      cumulative_probabilities[i + 1] =
          cumulative_probabilities[i] + frequencies[i];
    }
    normalized = true;
  }
};

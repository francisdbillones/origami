#include <cstdint>
#include <iostream>
#include <vector>

typedef struct prob {
  uint64_t low;
  uint64_t high;
  uint64_t count;
} prob;

typedef uint16_t SYMBOL;

template <int FREQ_BITS> class byte_static_model {
private:
  uint64_t cumulative_probabilities[258];

  static constexpr uint64_t MAX_FREQ = (1ULL << FREQ_BITS) - 1;
  bool freeze_next_update;

public:
  bool frozen;

  static constexpr SYMBOL EOF_SYMBOL = 256;
  byte_static_model() { reset(); }

  prob get_probability(const SYMBOL byte) const {
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
    frozen = false;
    freeze_next_update = false;

    for (int i = 0; i < 258; ++i)
      cumulative_probabilities[i] = i;
  }

  bool update(const SYMBOL byte) {
    if (frozen)
      return false;
    for (int i = byte + 1; i < 258; ++i)
      cumulative_probabilities[i]++;

    if (freeze_next_update) {
      frozen = true;
      freeze_next_update = false;
    }
    if (get_count() == MAX_FREQ)
      freeze_next_update = true;
    return true;
  }

  inline uint64_t get_count() const {
    if (frozen)
      return 1ULL << FREQ_BITS;
    else
      return cumulative_probabilities[257];
  }
};

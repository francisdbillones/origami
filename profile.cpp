#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

#include "arithmetic_coding.cpp"

struct profile_result {
  size_t input_size;
  size_t output_size;
  std::chrono::microseconds duration;
  bool success;
};

void print_stats(const std::string &operation,
                 const std::vector<profile_result> &results) {
  // calculate averages
  double avg_ratio = 0;
  double avg_speed = 0;
  double avg_time_ms = 0;
  int success_count = 0;

  for (const auto &r : results) {
    if (!r.success)
      continue;
    success_count++;
    avg_ratio += static_cast<double>(r.output_size) / r.input_size;
    avg_speed +=
        (r.input_size * 1000000.0) / (r.duration.count() * 1024 * 1024);
    avg_time_ms += r.duration.count() / 1000.0;
  }

  if (success_count > 0) {
    avg_ratio /= success_count;
    avg_speed /= success_count;
    avg_time_ms /= success_count;
  }

  std::cout << operation << " Statistics:\n"
            << "  Success rate: " << success_count << "/" << results.size()
            << "\n"
            << "  Average ratio: " << std::fixed << std::setprecision(4)
            << avg_ratio << "\n"
            << "  Average speed: " << std::fixed << std::setprecision(2)
            << avg_speed << " MB/s\n"
            << "  Average time: " << std::fixed << std::setprecision(2)
            << avg_time_ms << " ms\n\n";
}

size_t get_file_size(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file)
    return 0;
  auto size = file.tellg();
  return size >= 0 ? static_cast<size_t>(size) : 0;
}

bool verify_files(const std::string &original, const std::string &decoded) {
  std::ifstream f1(original, std::ios::binary);
  std::ifstream f2(decoded, std::ios::binary);

  if (!f1 || !f2) {
    std::cerr << "Failed to open files for verification\n";
    return false;
  }

  std::vector<char> buf1(8192), buf2(8192);
  size_t total_bytes = 0;

  while (f1 && f2) {
    f1.read(buf1.data(), buf1.size());
    f2.read(buf2.data(), buf2.size());

    auto count1 = f1.gcount();
    auto count2 = f2.gcount();

    if (count1 != count2) {
      std::cerr << "Size mismatch: original read " << count1
                << " bytes, decoded read " << count2 << " bytes\n";
      return false;
    }

    if (std::memcmp(buf1.data(), buf2.data(), count1) != 0) {
      std::cerr << "Content mismatch at offset " << total_bytes << "\n";
      // print first mismatch
      for (size_t i = 0; i < count1; i++) {
        if (buf1[i] != buf2[i]) {
          std::cerr << "First difference at byte " << i << ": " << std::hex
                    << (int)(unsigned char)buf1[i] << " vs "
                    << (int)(unsigned char)buf2[i] << std::dec << "\n";
          break;
        }
      }
      return false;
    }

    total_bytes += count1;
    if (f1.eof() != f2.eof()) {
      std::cerr << "EOF mismatch at " << total_bytes << " bytes\n";
      return false;
    }
  }

  bool success = f1.eof() && f2.eof();
  if (!success) {
    std::cerr << "Final EOF state mismatch\n";
  }
  return success;
}

profile_result run_compression(arithmetic_coder<32, 16> &coder,
                               const std::string &input_file,
                               const std::string &output_file) {
  profile_result result{};
  result.input_size = get_file_size(input_file);

  coder.reset();

  std::ifstream input(input_file, std::ios::binary);
  std::ofstream output(output_file, std::ios::binary);

  if (!input || !output || result.input_size == 0) {
    result.success = false;
    return result;
  }

  auto start = std::chrono::high_resolution_clock::now();
  coder.encode(input, output);
  output.flush();
  output.close();
  auto end = std::chrono::high_resolution_clock::now();

  result.output_size = get_file_size(output_file);
  result.duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  result.success = true;

  return result;
}

profile_result run_decompression(arithmetic_coder<32, 16> &coder,
                                 const std::string &input_file,
                                 const std::string &output_file) {
  profile_result result{};
  result.input_size = get_file_size(input_file);

  coder.reset();

  std::ifstream input(input_file, std::ios::binary);
  std::ofstream output(output_file, std::ios::binary);

  if (!input || !output || result.input_size == 0) {
    result.success = false;
    return result;
  }

  auto start = std::chrono::high_resolution_clock::now();
  coder.decode(input, output);
  output.flush();
  output.close();
  auto end = std::chrono::high_resolution_clock::now();

  result.output_size = get_file_size(output_file);
  result.duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  result.success = true;

  return result;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " <input_file>\n";
    return 1;
  }

  const int NUM_RUNS = 5; // number of times to run each operation
  const std::string input_file = argv[1];
  std::vector<profile_result> compression_results;
  std::vector<profile_result> decompression_results;
  std::vector<bool> verification_results;

  arithmetic_coder<32, 16> coder;

  for (int i = 0; i < NUM_RUNS; i++) {
    std::string compressed_file =
        input_file + ".compressed." + std::to_string(i);
    std::string decompressed_file =
        input_file + ".decompressed." + std::to_string(i);

    auto comp_result = run_compression(coder, input_file, compressed_file);
    compression_results.push_back(comp_result);

    if (comp_result.success) {
      auto decomp_result =
          run_decompression(coder, compressed_file, decompressed_file);
      decompression_results.push_back(decomp_result);

      if (decomp_result.success) {
        verification_results.push_back(
            verify_files(input_file, decompressed_file));
      }
    }

    // cleanup
    std::remove(compressed_file.c_str());
    std::remove(decompressed_file.c_str());
  }

  print_stats("Compression", compression_results);
  print_stats("Decompression", decompression_results);

  int successful_verifications = std::count(verification_results.begin(),
                                            verification_results.end(), true);
  std::cout << "Verification success rate: " << successful_verifications << "/"
            << verification_results.size() << "\n";

  return 0;
}

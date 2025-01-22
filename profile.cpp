#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
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
  double avg_ratio = 0;
  double avg_speed = 0;
  double avg_time_ms = 0;
  int success_count = 0;

  std::vector<double> ratios;
  std::vector<double> speeds;
  std::vector<double> times;

  for (const auto &r : results) {
    if (!r.success)
      continue;
    success_count++;
    double ratio = static_cast<double>(r.output_size) / r.input_size;
    double speed =
        (r.input_size * 1000000.0) / (r.duration.count() * 1024 * 1024);
    double time = r.duration.count() / 1000.0;

    ratios.push_back(ratio);
    speeds.push_back(speed);
    times.push_back(time);

    avg_ratio += ratio;
    avg_speed += speed;
    avg_time_ms += time;
  }

  if (success_count > 0) {
    avg_ratio /= success_count;
    avg_speed /= success_count;
    avg_time_ms /= success_count;

    auto calc_stddev = [](const std::vector<double> &vals, double mean) {
      double sum = 0;
      for (double v : vals) {
        sum += (v - mean) * (v - mean);
      }
      return std::sqrt(sum / vals.size());
    };

    double ratio_stddev = calc_stddev(ratios, avg_ratio);
    double speed_stddev = calc_stddev(speeds, avg_speed);
    double time_stddev = calc_stddev(times, avg_time_ms);

    std::cout << operation << " Statistics:\n"
              << "  Success rate: " << success_count << "/" << results.size()
              << "\n"
              << "  Ratio: " << std::fixed << std::setprecision(4) << avg_ratio
              << " ± " << ratio_stddev << "\n"
              << "  Speed: " << std::fixed << std::setprecision(2) << avg_speed
              << " ± " << speed_stddev << " MB/s\n"
              << "  Time: " << std::fixed << std::setprecision(2) << avg_time_ms
              << " ± " << time_stddev << " ms\n\n";
  }
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

    auto count1 = static_cast<size_t>(f1.gcount());
    auto count2 = static_cast<size_t>(f2.gcount());

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
                               const std::vector<char> &input_data,
                               std::vector<char> &output_data) {
  profile_result result{};
  result.input_size = input_data.size();

  coder.reset();

  auto start = std::chrono::high_resolution_clock::now();

  std::stringstream input_stream, output_stream;
  input_stream.write(input_data.data(), input_data.size());
  coder.encode(input_stream, output_stream);

  auto end = std::chrono::high_resolution_clock::now();

  output_data =
      std::vector<char>(std::istreambuf_iterator<char>(output_stream), {});
  result.output_size = output_data.size();
  result.duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  result.success = true;

  return result;
}

profile_result run_decompression(arithmetic_coder<32, 16> &coder,
                                 const std::vector<char> &input_data,
                                 std::vector<char> &output_data) {
  profile_result result{};
  result.input_size = input_data.size();

  coder.reset();

  auto start = std::chrono::high_resolution_clock::now();

  std::stringstream input_stream, output_stream;
  input_stream.write(input_data.data(), input_data.size());
  coder.decode(input_stream, output_stream);

  auto end = std::chrono::high_resolution_clock::now();

  output_data =
      std::vector<char>(std::istreambuf_iterator<char>(output_stream), {});
  result.output_size = output_data.size();
  result.duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  result.success = true;

  return result;
}

bool verify_data(const std::vector<char> &original,
                 const std::vector<char> &decoded) {
  if (original.size() != decoded.size()) {
    std::cerr << "Size mismatch: original " << original.size()
              << " bytes, decoded " << decoded.size() << " bytes\n";
    return false;
  }

  if (std::memcmp(original.data(), decoded.data(), original.size()) != 0) {
    for (size_t i = 0; i < original.size(); i++) {
      if (original[i] != decoded[i]) {
        std::cerr << "First difference at byte " << i << ": " << std::hex
                  << (int)(unsigned char)original[i] << " vs "
                  << (int)(unsigned char)decoded[i] << std::dec << "\n";
        break;
      }
    }
    return false;
  }

  return true;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <input_file> <num_runs>\n";
    return 1;
  }

  const int NUM_RUNS = std::stoi(argv[2]);
  const std::string input_file = argv[1];
  std::vector<char> input_data;

  // load file into memory
  std::ifstream input(input_file, std::ios::binary);
  if (!input) {
    std::cerr << "failed to open input file\n";
    return 1;
  }
  input_data = std::vector<char>(std::istreambuf_iterator<char>(input), {});
  input.close();

  std::vector<profile_result> compression_results(NUM_RUNS);
  std::vector<profile_result> decompression_results(NUM_RUNS);
  std::vector<bool> verification_results(NUM_RUNS);

  arithmetic_coder<32, 16> coder;
  for (int i = 0; i < NUM_RUNS; i++) {
    std::vector<char> compressed_data, decompressed_data;

    compression_results[i] =
        run_compression(coder, input_data, compressed_data);
    if (compression_results[i].success) {
      decompression_results[i] =
          run_decompression(coder, compressed_data, decompressed_data);
      if (decompression_results[i].success) {
        verification_results[i] = verify_data(input_data, decompressed_data);
      }
    }
  }

  print_stats("Compression", compression_results);
  print_stats("Decompression", decompression_results);

  int successful_verifications = std::count(verification_results.begin(),
                                            verification_results.end(), true);
  std::cout << "Verification success rate: " << successful_verifications << "/"
            << verification_results.size() << "\n";

  return 0;
}

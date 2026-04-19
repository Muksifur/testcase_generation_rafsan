#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr long long kMinValue = 100000000000LL;
constexpr long long kMaxValue = 1000000000000LL;

struct Zone {
  int start = 0;
  int end = 0;
  int inner_start = 0;
  int inner_end = 0;
  int p1 = 0;
  int p2 = 0;
  long long base = 0;
};

struct TestCase {
  int N = 0;
  int X = 0;
  int u = 0;
  int v = 0;
  long long c = 0;
  std::vector<long long> a;
  std::vector<int> x;
};

class Generator {
 public:
  Generator()
      : rng_(static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count())) {}

  int RandInt(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng_);
  }

  long long RandLong(long long lo, long long hi) {
    std::uniform_int_distribution<long long> dist(lo, hi);
    return dist(rng_);
  }

  double RandProb() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng_);
  }

  std::vector<int> GeneratePrimes(int limit) {
    std::vector<bool> is_prime(limit + 1, true);
    is_prime[0] = false;
    if (limit >= 1) {
      is_prime[1] = false;
    }
    for (int i = 2; i * i <= limit; ++i) {
      if (is_prime[i]) {
        for (int j = i * i; j <= limit; j += i) {
          is_prime[j] = false;
        }
      }
    }
    std::vector<int> primes;
    for (int i = 2; i <= limit; ++i) {
      if (is_prime[i]) {
        primes.push_back(i);
      }
    }
    return primes;
  }

 private:
  std::mt19937_64 rng_;
};

int ReadIntWithDefault(const std::string& prompt, int default_value) {
  std::cout << prompt;
  std::string line;
  std::getline(std::cin, line);
  if (line.empty()) {
    return default_value;
  }
  return std::stoi(line);
}

std::string ReadStringWithDefault(const std::string& prompt, const std::string& def) {
  std::cout << prompt;
  std::string line;
  std::getline(std::cin, line);
  if (line.empty()) {
    return def;
  }
  return line;
}

std::vector<Zone> BuildZones(int X,
                             int zone_len,
                             int w_opt,
                             const std::vector<int>& primes,
                             Generator& gen) {
  int total_primes = static_cast<int>(primes.size());
  int zone_count = (X + zone_len - 1) / zone_len;
  std::vector<Zone> zones(zone_count);

  int group_size = w_opt >= 100 ? 3 : 2;
  double same_base_prob = w_opt >= 100 ? 0.35 : 0.2;

  for (int i = 0; i < zone_count; ++i) {
    int start = i * zone_len + 1;
    int end = std::min(X, start + zone_len - 1);
    int len = end - start + 1;
    int inner_start = start + len / 4;
    int inner_end = start + (3 * len) / 4;

    int idx1 = total_primes - 200 + (i * 7) % 100;
    int idx2 = total_primes - 150 + (i * 11) % 100;
    int p1 = primes[idx1];
    int p2 = primes[idx2];

    if (i > 0) {
      const Zone& prev = zones[i - 1];
      if (gen.RandProb() < same_base_prob) {
        p1 = prev.p1;
        p2 = prev.p2;
      } else {
        if (i % group_size != 0) {
          p1 = prev.p1;
        }
        if (i % group_size == group_size - 1) {
          p2 = prev.p2;
        }
      }
    }

    Zone zone;
    zone.start = start;
    zone.end = end;
    zone.inner_start = inner_start;
    zone.inner_end = inner_end;
    zone.p1 = p1;
    zone.p2 = p2;
    zone.base = 1LL * p1 * p2;
    zones[i] = zone;
  }
  return zones;
}

std::vector<int> SelectPositions(int N,
                                 int X,
                                 int zone_len,
                                 const std::vector<Zone>& zones,
                                 Generator& gen) {
  std::vector<int> selected;
  selected.reserve(N);
  std::vector<int> remaining;
  remaining.reserve(X);

  constexpr double kInnerProb = 0.9;
  constexpr double kOuterProb = 0.7;

  for (int pos = 1; pos <= X; ++pos) {
    int zone_index = (pos - 1) / zone_len;
    const Zone& zone = zones[zone_index];
    bool inner = pos >= zone.inner_start && pos <= zone.inner_end;
    double prob = inner ? kInnerProb : kOuterProb;
    if (gen.RandProb() < prob) {
      selected.push_back(pos);
    } else {
      remaining.push_back(pos);
    }
  }

  if (static_cast<int>(selected.size()) > N) {
    std::shuffle(selected.begin(), selected.end(), std::mt19937{std::random_device{}()});
    selected.resize(N);
  } else if (static_cast<int>(selected.size()) < N) {
    std::shuffle(remaining.begin(), remaining.end(), std::mt19937{std::random_device{}()});
    int needed = N - static_cast<int>(selected.size());
    if (needed > static_cast<int>(remaining.size())) {
      needed = static_cast<int>(remaining.size());
    }
    selected.insert(selected.end(), remaining.begin(), remaining.begin() + needed);
  }

  std::sort(selected.begin(), selected.end());
  return selected;
}

std::vector<long long> BuildValues(const std::vector<int>& positions,
                                   int zone_len,
                                   const std::vector<Zone>& zones,
                                   Generator& gen) {
  std::vector<long long> values;
  values.reserve(positions.size());

  for (int pos : positions) {
    int zone_index = (pos - 1) / zone_len;
    const Zone& zone = zones[zone_index];
    bool inner = pos >= zone.inner_start && pos <= zone.inner_end;
    long long multiplier = inner ? gen.RandLong(50, 200) : gen.RandLong(20, 100);
    long long value = zone.base * multiplier;
    if (value > kMaxValue) {
      value = kMaxValue - gen.RandLong(0, 500000);
    }
    if (value < kMinValue) {
      value = kMinValue;
    }
    values.push_back(value);
  }
  return values;
}

long long ChooseCost(Generator& gen, int min_c, int max_c, double bias) {
  long long span = max_c - min_c;
  long long upper = min_c + static_cast<long long>(span * bias);
  if (upper < min_c) {
    upper = min_c;
  }
  return gen.RandLong(min_c, upper);
}

TestCase BuildTestCase(int subtask, bool stress, const std::vector<int>& primes, Generator& gen) {
  int X = 0;
  int N = 0;
  int min_c = 0;
  int max_c = 0;
  int delta_min = 0;
  int delta_max = 0;

  if (subtask == 1) {
    X = gen.RandInt(2, 1000);
    N = 2;
    min_c = 100;
    max_c = 10000;
    delta_min = 1;
    delta_max = std::min(50, X - 1);
  } else if (subtask == 2) {
    if (stress) {
      X = 1000;
      N = std::min(X, std::max(2, gen.RandInt(X / 2 - 20, X / 2 + 20)));
      min_c = 10000;
      max_c = 100000;
    } else {
      X = gen.RandInt(200, 1000);
      N = std::min(X, std::max(2, gen.RandInt(X / 3, X)));
      min_c = 1000;
      max_c = 100000;
    }
    delta_min = 50;
    delta_max = std::min(500, X - 1);
  } else if (subtask == 3) {
    X = gen.RandInt(5000, 100000);
    N = std::min(X, std::max(2, gen.RandInt(X / 4, X)));
    min_c = 5000;
    max_c = 500000;
    delta_min = 1;
    delta_max = std::min(20, X - 1);
  } else {
    if (stress) {
      X = gen.RandInt(900000, 1000000);
      N = std::min(X, std::max(2, gen.RandInt(300000, 500000)));
      min_c = 10000;
      max_c = 100000;
      delta_min = 100000;
      delta_max = std::min(500000, X - 1);
    } else {
      X = gen.RandInt(200000, 1000000);
      N = std::min(X, std::max(2, gen.RandInt(X / 3, std::min(500000, X))));
      min_c = 10000;
      max_c = 500000;
      delta_min = 100;
      delta_max = std::min(500000, X - 1);
    }
  }

  int u = gen.RandInt(1, std::max(1, X - delta_min));
  int delta = gen.RandInt(delta_min, std::min(delta_max, X - u));
  int v = u + delta;
  int w_opt = (u + v) / 2 + gen.RandInt(-5, 5);
  w_opt = std::max(u, std::min(v, w_opt));

  double cost_bias = stress ? 0.1 : 0.2;
  long long c = ChooseCost(gen, min_c, max_c, cost_bias);

  int zone_len = std::max(1, w_opt / 2);
  std::vector<Zone> zones = BuildZones(X, zone_len, w_opt, primes, gen);
  std::vector<int> positions = SelectPositions(N, X, zone_len, zones, gen);
  std::vector<long long> values = BuildValues(positions, zone_len, zones, gen);

  TestCase tc;
  tc.N = static_cast<int>(positions.size());
  tc.X = X;
  tc.u = u;
  tc.v = v;
  tc.c = c;
  tc.a = std::move(values);
  tc.x = std::move(positions);
  return tc;
}

void WriteTestCase(std::ostream& out, const TestCase& tc) {
  out << tc.N << ' ' << tc.X << ' ' << tc.u << ' ' << tc.v << ' ' << tc.c << '\n';
  for (int i = 0; i < tc.N; ++i) {
    out << tc.a[i] << (i + 1 == tc.N ? '\n' : ' ');
  }
  for (int i = 0; i < tc.N; ++i) {
    out << tc.x[i] << (i + 1 == tc.N ? '\n' : ' ');
  }
}

}  // namespace

int main() {
  Generator gen;
  std::vector<int> primes = gen.GeneratePrimes(100000);
  if (primes.size() < 300) {
    std::cerr << "Not enough primes generated.\n";
    return 1;
  }

  std::cout << "Select subtasks to generate:\n"
            << "1. Subtask 1 (N=2)\n"
            << "2. Subtask 2 (N, X, x_i, u, v <= 1000)\n"
            << "3. Subtask 3 (v-u <= 20)\n"
            << "4. Subtask 4 (Full constraints - can be stress test)\n"
            << "5. All subtasks\n";

  int choice = ReadIntWithDefault("Enter your choice (1-5): ", 5);
  int files_per_subtask =
      ReadIntWithDefault("Number of test files to generate (default 3): ", 3);
  std::string folder =
      ReadStringWithDefault("Output folder name (default 'generated_tests'): ",
                            "generated_tests");

  if (!fs::exists(folder)) {
    fs::create_directories(folder);
  }

  std::vector<int> subtasks;
  if (choice == 5) {
    subtasks = {1, 2, 3, 4};
  } else {
    subtasks = {choice};
  }

  int file_index = 1;
  for (int subtask : subtasks) {
    int stress_files = 0;
    if (subtask == 2 || subtask == 4) {
      stress_files = std::max(1, (files_per_subtask + 3) / 4);
    }

    std::cout << "Subtask " << subtask << ":\n";
    for (int i = 0; i < files_per_subtask; ++i) {
      bool stress = (subtask == 2 || subtask == 4) && (i >= files_per_subtask - stress_files);
      int case_count = 0;
      if (subtask == 1) {
        case_count = 8000;
      } else if (stress) {
        case_count = 1;
      } else if (subtask == 2) {
        case_count = gen.RandInt(1, 3);
      } else if (subtask == 3) {
        case_count = gen.RandInt(4, 10);
      } else {
        case_count = gen.RandInt(5, 10);
      }

      std::vector<TestCase> cases;
      cases.reserve(case_count);
      int min_n = std::numeric_limits<int>::max();
      int max_n = 0;
      int max_x = 0;

      for (int t = 0; t < case_count; ++t) {
        TestCase tc = BuildTestCase(subtask, stress, primes, gen);
        min_n = std::min(min_n, tc.N);
        max_n = std::max(max_n, tc.N);
        max_x = std::max(max_x, tc.X);
        cases.push_back(std::move(tc));
      }

      std::string filename = folder + "/" + std::to_string(file_index) + "." +
                             std::to_string(subtask) + ".in.txt";
      std::ofstream out(filename);
      out << case_count << '\n';
      for (const auto& tc : cases) {
        WriteTestCase(out, tc);
      }
      out.close();

      if (stress) {
        std::cout << "  [STRESS TEST] " << filename << " (" << case_count
                  << " test cases, STRESS: max N=" << max_n
                  << ", X=" << max_x << ")\n";
      } else {
        std::cout << "  " << filename << " (" << case_count
                  << " test cases, N=" << min_n << "-" << max_n << ")\n";
      }
      ++file_index;
    }
  }

  std::cout << "\nTest generation complete!\nOutput folder: " << folder << "\n";
  return 0;
}

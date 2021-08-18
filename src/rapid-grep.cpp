#include <array>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <vector>
#include <unistd.h>

struct Options {
  bool ignore_case;
  bool help_only;
  std::string pattern;
  std::string scanning_directory;

  static Options *read_args(int argc, char *argv[]) {
    static Options opts;
    for (;;) {
      int option_index = 0;
      const int c = getopt_long(argc, argv, "hir:", long_options_.data(), &option_index);
      if (c == -1) {
        break;
      }
      switch (c) {
        case 'h':
          opts.help_only = true;
          print_help();
          return &opts;
        case 'i':
          opts.ignore_case = true;
          break;
        case 'r':
          opts.scanning_directory = optarg;
          break;
        default:
          print_help();
          return {};
      }
    }
    if (optind == argc) {
      std::cerr << argv[0] << ": pattern file is required" << std::endl;
      print_help();
      return {};
    }
    if (optind + 1 != argc) {
      std::cerr << argv[0] << ": too many pattern, expected only one" << std::endl;
      print_help();
      return {};
    }
    opts.pattern = argv[optind];
    return &opts;
  }

private:
  static void print_help() {
    std::cout << "USAGE: rapid-grep [OPTIONS]... FILE" << std::endl
              << "Print lines that match pattern." << std::endl << std::endl
              << "Options:" << std::endl;
    for (auto opt : long_options_) {
      if (opt.val) {
        std::cout << " -" << static_cast<char>(opt.val) << ", --" << opt.name;
        if (opt.has_arg == required_argument) {
          std::cout << " OPTION";
        }
        std::cout << std::endl;
      }
    }
  }

  static constexpr std::array long_options_{
    option{"help", no_argument, nullptr, 'h'},
    option{"ignore-case", no_argument, nullptr, 'i'},
    option{"recursive", required_argument, nullptr, 'r'},
    option{0, 0, 0, 0}
  };
};

std::vector<std::string> read_input(std::istream &in) {
  std::vector<std::string> input;
  std::string line;
  while (std::getline(in, line)) {
    input.emplace_back(line);
  }
  return input;
}

bool is_stdout_terminal() {
  return static_cast<bool>(isatty(1));
}

void print_matches(std::string prefix, std::vector<std::string> input, std::string pattern, bool ignore_case) {
  if (ignore_case) {
    std::transform(pattern.begin(), pattern.end(), pattern.begin(), &tolower);
  }

  bool colorize = is_stdout_terminal();
  for (auto line : input) {
    if (ignore_case) {
      std::transform(line.begin(), line.end(), line.begin(), &tolower);
    }

    auto pos = line.find(pattern);
    if (pos == std::string::npos) {
      continue;
    } else {
      if (colorize) {
        std::cout << prefix << line.substr(0, pos) <<
                  "\033[1;31m" << line.substr(pos, pattern.size()) << "\033[0m"
                  << line.substr(pos + pattern.size()) << std::endl;
      } else {
        std::cout << prefix << line << std::endl;
      }
    }
  }
}

int main(int argc, char *argv[]) {
  const auto options = Options::read_args(argc, argv);
  if (!options) {
    return 1;
  }
  if (options->help_only) {
    return 0;
  }

  if (options->scanning_directory.empty()) {
    auto lines = read_input(std::cin);
    print_matches("", lines, options->pattern, options->ignore_case);
  } else {
    for (const auto &p: std::filesystem::recursive_directory_iterator(options->scanning_directory)) {
      if (p.is_regular_file()) {
        std::ifstream in{p.path()};
        auto lines = read_input(in);
        std::string prefix;
        if (is_stdout_terminal()) {
          prefix = "\033[0;35m" + p.path().string() + "\033[0m\033[0;36m:\033[0m";
        } else {
          prefix = p.path().string();
        }
        print_matches(prefix, lines, options->pattern, options->ignore_case);
      }
    }
  }

  return 0;
}

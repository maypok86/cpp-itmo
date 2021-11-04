#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>

namespace {
    std::vector<std::string> split(const std::string &line, const std::string &separator) {
        std::size_t prev = 0;
        std::size_t pos;
        std::vector<std::string> result;
        while ((pos = line.find_first_of(separator, prev)) != std::string::npos) {
            if (pos > prev) {
                result.push_back(line.substr(prev, pos - prev));
            }
            prev = pos + 1;
        }
        if (prev < line.length()) {
            result.push_back(line.substr(prev));
        }
        return result;
    }

    class Line {
    public:
        using value_type = std::string;

        Line(std::string line, const size_t index)
                : m_line(std::move(line)), m_index(index) {
        }

        size_t index() const { return m_index; }

        friend bool operator<(const Line &first, const Line &second) {
            return first.m_line < second.m_line;
        }

        friend std::ostream &operator<<(std::ostream &os, const Line &line) {
            os << line.m_line;
            return os;
        }

    private:
        std::string m_line;
        size_t m_index;
    };

    class Lines {
    public:
        using value_type = Line;
        using const_iterator = std::vector<Line>::const_iterator;

        Lines(std::string sep, std::optional<std::pair<size_t, size_t>> range_keys)
                : m_range(std::move(range_keys)), m_separator(std::move(sep)) {
        }

        void add(const std::string &line) {
            m_lines.emplace_back(line, m_lines.size());
            if (m_range.has_value()) {
                m_split_lines.push_back(split(line, m_separator));
            }
        }

        void sort() {
            const auto comparator = [this](const Line &first, const Line &second) {
                const auto contains_key = [this, &first, &second](const size_t key) {
                    const bool contains_first = m_split_lines[first.index()].size() > key - 1;
                    const bool contains_second = m_split_lines[second.index()].size() > key - 1;
                    if (contains_first && contains_second) {
                        return std::make_pair(true, m_split_lines[first.index()][key - 1] <
                                                    m_split_lines[second.index()][key - 1]);
                    }
                    if (contains_first != contains_second) {
                        return std::make_pair(true, contains_first < contains_second);
                    }
                    return std::make_pair(false, false);
                };
                if (m_range.has_value()) {
                    const size_t last_key = m_range->second == std::string::npos ? m_range->first : m_range->second;
                    for (size_t key = m_range->first; key <= last_key; ++key) {
                        const std::pair<bool, bool> contains = contains_key(key);
                        if (contains.first) {
                            return contains.second;
                        }
                    }
                }
                return first < second;
            };
            std::sort(m_lines.begin(), m_lines.end(), comparator);
        }

        const_iterator begin() const {
            return m_lines.begin();
        }

        const_iterator end() const {
            return m_lines.end();
        }

    private:
        const std::optional<std::pair<size_t, size_t>> m_range;
        const std::string m_separator;
        std::vector<Line> m_lines;
        std::vector<std::vector<std::string>> m_split_lines;
    };

    template<class C>
    void print_out(std::ostream &strm, const C &c) {
        std::ostream_iterator<typename C::value_type> out(strm, "\n");
        std::copy(c.begin(), c.end(), out);
    }

    void sort_stream(std::istream &input, const std::string &sep,
                     const std::optional<std::pair<size_t, size_t>> &range_keys = std::nullopt) {
        std::string separator;
        if (sep.empty()) {
            // standard c++ whitespaces
            separator = " \n\f\r\t\v";
        } else {
            separator = sep;
        }
        Lines lines(separator, range_keys);
        // read lines
        std::string line;
        while (std::getline(input, line)) {
            lines.add(line);
        }
        lines.sort();
        print_out(std::cout, lines);
    }

    size_t to_size_t(const std::string &number, const size_t default_value) {
        size_t result;
        std::istringstream iss(number);
        iss >> result;
        if (iss.fail()) {
            return default_value;
        } else {
            return result;
        }
    }

} // anonymous namespace

int main(int argc, char **argv) {
    const size_t MAX = std::string::npos;
    std::optional<std::pair<size_t, size_t>> keys = std::nullopt;
    std::string sep;
    const char *input_name = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (argv[i][1] != '-') {
                switch (argv[i][1]) {
                    case 'k':
                        keys = {to_size_t(argv[++i], 1), MAX};
                        break;
                    case 't':
                        sep += argv[++i];
                        break;
                }
            } else {
                const std::string arg = argv[i];
                const std::string key = "--key=";
                const std::string field_sep = "--field-separator=";
                const std::string sep_key = "],";
                if (arg.find(key) != std::string::npos) {
                    const size_t position = arg.find(sep_key);
                    if (position == std::string::npos) {
                        keys = {to_size_t(arg.substr(key.length()), 1), MAX};
                    } else {
                        keys = {to_size_t(arg.substr(key.length(), position - key.length()), 1),
                                to_size_t(
                                        arg.substr(position + sep_key.length(),
                                                   arg.length() - position - sep_key.length() - 1),
                                        MAX)};
                    }
                } else if (arg.find(field_sep) != std::string::npos) {
                    sep += arg.substr(field_sep.length());
                }
            }
        } else {
            input_name = argv[i];
        }
    }
    if (input_name != nullptr) {
        std::ifstream f(input_name);
        sort_stream(f, sep, keys);
    } else {
        sort_stream(std::cin, sep, keys);
    }

    return 0;
}

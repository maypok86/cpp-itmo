#include "searcher.h"

#include <algorithm>

// standard C++ whitespaces
const std::string sep = " \n\f\r\t\v";
const std::string query_sep = sep + "\"";

namespace {

std::string strip(const std::string & line, const size_t begin, const size_t end)
{
    size_t first = begin;
    while (first < end && std::ispunct(line[first])) {
        ++first;
    }
    if (first == end) {
        return "";
    }
    size_t last = end - 1;
    while (std::ispunct(line[last])) {
        --last;
    }
    std::string sub = line.substr(first, last - first + 1);
    std::transform(sub.begin(), sub.end(), sub.begin(), [](const unsigned char c) {
        return std::tolower(c);
    });
    return sub;
}

std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>> split_line(const std::string & line, const size_t begin, const size_t end, const bool is_query)
{
    size_t prev = begin;
    size_t pos;
    std::pair<std::vector<std::string>, std::vector<std::vector<std::string>>> result;
    const auto add = [&result, &line](const size_t prev, const size_t pos) {
        const auto & word = strip(line, prev, pos);
        if (!word.empty()) {
            result.first.emplace_back(word);
        }
    };
    const std::string & separator = is_query ? query_sep : sep;
    while ((pos = line.find_first_of(separator, prev)) < end) {
        if (is_query && line[pos] == '\"') {
            if (pos > 0 && query_sep.find(line[pos - 1]) == std::string::npos) {
                add(prev, pos);
            }
            ++pos;
            prev = pos;
            while (pos < end && line[pos] != '\"') {
                ++pos;
            }
            if (pos < end) {
                const auto & v = split_line(line, prev, pos, false).first;
                if (v.empty()) {
                    throw Searcher::BadQuery("find empty phrase.");
                }
                result.second.emplace_back(v);
            }
            else {
                throw Searcher::BadQuery("not found pair with '\"'.");
            }
            ++pos;
            prev = line.find_first_not_of(sep, pos);
            continue;
        }
        if (pos > prev) {
            add(prev, pos);
        }
        prev = pos + 1;
    }
    if (prev < end) {
        add(prev, end);
    }
    if (is_query && result.first.empty() && result.second.empty()) {
        throw Searcher::BadQuery("empty query.");
    }
    return result;
}

} // anonymous namespace

void Searcher::add_document(const Searcher::Filename & filename, std::istream & strm)
{
    if (m_documents.find(filename) == m_documents.end()) {
        m_documents.insert(filename);
    }
    else {
        remove_document(filename);
    }
    size_t count_word = 0;
    std::string line;
    while (std::getline(strm, line)) {
        for (const auto & word : split_line(line, 0, line.length(), false).first) {
            m_inverted_index[word][filename].emplace(count_word);
            ++count_word;
        }
    }
}

void Searcher::remove_document(const Searcher::Filename & filename)
{
    if (m_documents.find(filename) == m_documents.end()) {
        return;
    }
    m_documents.erase(filename);
    for (auto & [word, m] : m_inverted_index) {
        m.erase(filename);
    }
}

std::pair<Searcher::DocIterator, Searcher::DocIterator> Searcher::search(const std::string & query) const
{
    const auto & [unordered, ordered] = split_line(query, 0, query.length(), true);
    const auto docs = std::make_shared<std::unordered_set<Filename>>();
    if (check_word(unordered)) {
        return std::make_pair(DocIterator(docs, docs->end()), DocIterator(docs, docs->end()));
    }
    for (const auto & phrase : ordered) {
        if (check_word(phrase)) {
            return std::make_pair(DocIterator(docs, docs->end()), DocIterator(docs, docs->end()));
        }
    }
    if (unordered.empty()) {
        for (const auto & [file, s] : m_inverted_index.at(ordered[0][0])) {
            if (check_phrase(ordered, file)) {
                docs->emplace(file);
            }
        }
    }
    else {
        for (const auto & [file, s] : m_inverted_index.at(unordered[0])) {
            if (check_file(unordered, file)) {
                docs->emplace(file);
            }
        }
        if (!ordered.empty()) {
            auto file = docs->begin();
            while (file != docs->end()) {
                if (check_phrase(ordered, *file)) {
                    ++file;
                }
                else {
                    file = docs->erase(file);
                }
            }
        }
    }
    return {DocIterator(docs, docs->begin()), DocIterator(docs, docs->end())};
}

Searcher::DocIterator::DocIterator(const std::shared_ptr<std::unordered_set<Filename>> & set_pointer, const std::unordered_set<Filename>::iterator & current)
    : m_set_pointer(set_pointer)
    , m_current(current)
{
}

bool Searcher::DocIterator::operator==(const Searcher::DocIterator & that) const
{
    return m_current == that.m_current;
}

bool Searcher::DocIterator::operator!=(const Searcher::DocIterator & that) const
{
    return !(*this == that);
}

Searcher::DocIterator::reference Searcher::DocIterator::operator*() const
{
    return *m_current;
}

Searcher::DocIterator::pointer Searcher::DocIterator::operator->() const
{
    return &*m_current;
}

Searcher::DocIterator & Searcher::DocIterator::operator++()
{
    ++m_current;
    return *this;
}

Searcher::DocIterator Searcher::DocIterator::operator++(int)
{
    auto copy = *this;
    ++m_current;
    return copy;
}

Searcher::BadQuery::BadQuery(const std::string & message)
    : m_message("Search query syntax error: " + message)
{
}

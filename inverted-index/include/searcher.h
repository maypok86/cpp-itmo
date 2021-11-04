#pragma once

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class Searcher
{
public:
    using Filename = std::string; // or std::filesystem::path

    // index modification
    void add_document(const Filename & filename, std::istream & strm);

    void remove_document(const Filename & filename);

    // queries
    class DocIterator
    {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = const Filename;
        using pointer = const Filename *;
        using reference = const Filename &;
        using iterator_category = std::forward_iterator_tag;

        DocIterator(const std::shared_ptr<std::unordered_set<Filename>> & set_pointer, const std::unordered_set<Filename>::iterator & current);

        reference operator*() const;

        pointer operator->() const;

        DocIterator & operator++();

        DocIterator operator++(int);

        bool operator==(const DocIterator & that) const;

        bool operator!=(const DocIterator & that) const;

    private:
        std::shared_ptr<std::unordered_set<Filename>> m_set_pointer;
        std::unordered_set<Filename>::iterator m_current;
    };

    class BadQuery : public std::exception
    {
    public:
        explicit BadQuery(const std::string & message);

        const char * what() const noexcept override { return m_message.c_str(); }

    private:
        std::string m_message;
    };

    std::pair<DocIterator, DocIterator> search(const std::string & query) const;

private:
    bool check_positions(const std::vector<std::string> & phrase, const std::string & file) const
    {
        for (const auto pos : m_inverted_index.at(phrase[0]).at(file)) {
            size_t count = 1;
            for (size_t j = 1; j < phrase.size(); ++j) {
                if (m_inverted_index.at(phrase[j]).at(file).find(pos + j) != m_inverted_index.at(phrase[j]).at(file).end()) {
                    ++count;
                }
            }
            if (count == phrase.size()) {
                return true;
            }
        }
        return false;
    }

    bool check_phrase(const std::vector<std::vector<std::string>> & phrases, const std::string & file) const
    {
        for (const auto & phrase : phrases) {
            if (!(check_file(phrase, file) && check_positions(phrase, file))) {
                return false;
            }
        }
        return true;
    }

    bool check_file(const std::vector<std::string> & words, const std::string & file) const
    {
        for (const auto & w : words) {
            if (m_inverted_index.at(w).find(file) == m_inverted_index.at(w).end() || m_inverted_index.at(w).at(file).empty()) {
                return false;
            }
        }
        return true;
    }

    bool check_word(const std::vector<std::string> & words) const
    {
        for (const auto & w : words) {
            if (m_inverted_index.find(w) == m_inverted_index.end() || m_inverted_index.at(w).empty()) {
                return true;
            }
        }
        return false;
    }

    std::unordered_map<std::string, std::unordered_map<Filename, std::unordered_set<size_t>>> m_inverted_index;
    std::unordered_set<Filename> m_documents;
};

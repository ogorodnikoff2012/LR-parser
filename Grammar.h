//
// Created by xenon on 18.11.17.
//

#ifndef LR1_GRAMMAR_H
#define LR1_GRAMMAR_H

#include <string>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <map>

template <class T>
class Grammar {
public:
    typedef int symbol_t;
    typedef std::pair<symbol_t, std::vector<symbol_t>> Rule;
    static constexpr symbol_t EOL = 0;
private:
    std::vector<T> terminals;
    std::vector<std::string> nonterminals;
    std::vector<Rule> rules;
    std::set<symbol_t> epsProductive;
    std::map<symbol_t, std::set<symbol_t>> first;
    symbol_t rootSymbol;

    static void firstSetDfs(const std::vector<std::vector<symbol_t>> &gr,
                            symbol_t v, std::unordered_set<symbol_t> &accessible) {
        accessible.insert(v);
        if (v < 0) {
            return;
        }
        for (auto u : gr[v]) {
            if (accessible.find(u) == accessible.end()) {
                firstSetDfs(gr, u, accessible);
            }
        }
    }

public:
    Grammar() : terminals(), nonterminals(), rules(), epsProductive(), rootSymbol(-1) {
        newNonterminal("EOL");
    }

    symbol_t getRootSymbol() const {
        return rootSymbol;
    }

    void setRootSymbol(symbol_t rootSymbol) {
        this->rootSymbol = rootSymbol;
    }

    symbol_t newTerminal(const T& c) {
        terminals.push_back(c);
        return -static_cast<int>(terminals.size());
    }

    symbol_t newNonterminal(const std::string& name) {
        nonterminals.push_back(name);
        return static_cast<int>(nonterminals.size()) - 1;
    }

    int addRule(symbol_t leftPart, std::initializer_list<symbol_t> rightPart) {
        rules.push_back(std::make_pair(leftPart, std::vector<symbol_t>(rightPart)));
        return rules.size() - 1;
    }

    bool isValidSymbol(symbol_t token) const {
        return (token >= 0 && token < nonterminals.size()) ||
               (token < 0 && -token <= terminals.size());
    }

    void findEpsProductiveSymbols() {
        epsProductive.clear();
        unsigned long epsSize;
        do {
            epsSize = epsProductive.size();
            for (uint32_t i = 0; i < rules.size(); ++i) {
                bool isEpsProductive = true;
                for (symbol_t s : rules[i].second) {
                    isEpsProductive &= epsProductive.find(s) != epsProductive.end();
                }
                if (isEpsProductive) {
                    epsProductive.insert(rules[i].first);
                }
            }
        } while (epsSize != epsProductive.size());
    }

    bool isEpsProductive(symbol_t symbol) const {
        return epsProductive.find(symbol) != epsProductive.end();
    }

    void findFirstSets() {
        std::vector<std::vector<symbol_t>> graph(nonterminals.size());
        for (uint32_t i = 0; i < rules.size(); ++i) {
            for (uint32_t j = 0; j < rules[i].second.size(); ++j) {
                graph[rules[i].first].push_back(rules[i].second[j]);
                if (!isEpsProductive(rules[i].second[j])) {
                    break;
                }
            }
        }

        std::vector<std::unordered_set<symbol_t>> accessible(graph.size());
        for (uint32_t i = 0; i < graph.size(); ++i) {
            firstSetDfs(graph, i, accessible[i]);
        }

        for (uint32_t i = 0; i < accessible.size(); ++i) {
            for (symbol_t s : accessible[i]) {
                if (s < 0) {
                    first[i].insert(s);
                }
            }
        }

        for (uint32_t i = 0; i < terminals.size(); ++i) {
            first[kthTerminal(i)] = std::set<symbol_t>{kthTerminal(i)};
        }
    }

    const std::set<symbol_t>& getFirst(symbol_t symbol) const {
        return first.at(symbol);
    }

    void printRule(int ruleIdx, std::ostream& out = std::cout) const {
        out << nonterminals[rules[ruleIdx].first];
        out << " ->";
        for (symbol_t s : rules[ruleIdx].second) {
            out << ' ';
            if (s < 0) {
                out << terminals[-1 - s];
            } else {
                out << nonterminals[s];
            }
        }
    }

    symbol_t kthTerminal(uint32_t k) const {
        return -1 - static_cast<symbol_t>(k);
    }

    symbol_t kthNonterminal(uint32_t k) const {
        return static_cast<symbol_t>(k);
    }

    void printDebugInfo(std::ostream& out = std::cout) {
        out << "### Terminals:" << std::endl;
        for (uint32_t i = 0; i < terminals.size(); ++i) {
            symbol_t s = kthTerminal(i);
            out << s << " '" << terminals[i] << '\'' << std::endl;
        }

        out << "### Nonterminals:" << std::endl;
        for (uint32_t i = 0; i < nonterminals.size(); ++i) {
            symbol_t s = kthNonterminal(i);
            out << s << " " << nonterminals[i];
            if (isEpsProductive(s)) {
                out << " (eps-productive)";
            }
            out << std::endl;
        }

        out << "### Rules:" << std::endl;

        for (uint32_t i = 0; i < rules.size(); ++i) {
            printRule(i, out);
            out << std::endl;
        }

        out << "### First:" << std::endl;
        for (uint32_t i = 0; i < nonterminals.size(); ++i) {
            symbol_t s = kthNonterminal(i);
            out << "First(" << s << ") = {";
            if (!first[s].empty()) {
                auto iter = first[s].begin();
                out << *iter;
                ++iter;
                while (iter != first[s].end()) {
                    out << ", " << *iter;
                    ++iter;
                }
            }
            out << '}' << std::endl;
        }
    }

    int getRulesCount() const {
        return rules.size();
    }

    const Rule& getRule(int idx) const {
        return rules[idx];
    }

    void build() {
        findEpsProductiveSymbols();
        findFirstSets();
    }

    template <class Iter>
    std::unordered_set<symbol_t> getFirst(Iter begin, Iter end, symbol_t rightContext) const {
        std::unordered_set<symbol_t> answer;
        bool foundNotEpsProductive = false;
        while (begin != end && !foundNotEpsProductive) {
            symbol_t s = *begin;

            answer.insert(first.at(s).begin(), first.at(s).end());

            if (isEpsProductive(s)) {
                ++begin;
            } else {
                foundNotEpsProductive = true;
            }
        }
        if (!foundNotEpsProductive) {
            answer.insert(rightContext);
        }
        return answer;
    }

    const std::vector<Rule>& getRules() const {
        return rules;
    }

    const T& getTerminal(symbol_t s) const {
        return terminals[-1 - s];
    }

    const std::string& getNonterminal(symbol_t s) const {
        return nonterminals[s];
    }

    symbol_t getFirstValidSymbol() const {
        return -terminals.size();
    }

    symbol_t getLastValidSymbol() const {
        return nonterminals.size() - 1;
    }

    uint32_t getTerminalsCount() const {
        return terminals.size();
    }
};


#endif //LR1_GRAMMAR_H

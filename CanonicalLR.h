//
// Created by xenon on 18.11.17.
//

#ifndef LR1_CANONICALLR_H
#define LR1_CANONICALLR_H

#include <queue>
#include <iostream>
#include <map>
#include <fstream>
#include "Grammar.h"

std::string toPythonString(char c);

template <class T>
class CanonicalLR {
public:
    struct LRState {
        int ruleIdx;
        uint32_t cursor;
        typename Grammar<T>::symbol_t rightContext;
        bool operator< (const LRState& other) const {
            return (ruleIdx < other.ruleIdx) ||
                    (ruleIdx == other.ruleIdx && cursor < other.cursor) ||
                    (ruleIdx == other.ruleIdx && cursor == other.cursor && rightContext < other.rightContext);
        }
    };

    typedef std::set<LRState> State;
private:
    const Grammar<T>& grammar;
    std::map<State, int> states;
    std::vector<std::map<typename Grammar<T>::symbol_t, int>> goTable, returnTable;

    State closure(const State& st) {
        State answer = st;
        std::queue<LRState> q;
        for (LRState lr : st) {
            q.push(lr);
        }
        while (!q.empty()) {
            LRState lr = q.front();
            q.pop();

            const auto& curRule = grammar.getRule(lr.ruleIdx);
            if (lr.cursor < curRule.second.size()) {
                typename Grammar<T>::symbol_t s = curRule.second[lr.cursor];
                auto first = grammar.getFirst(curRule.second.begin() + lr.cursor + 1, curRule.second.end(), lr.rightContext);
                for (int i = 0; i < grammar.getRulesCount(); ++i) {
                    const auto& nextRule = grammar.getRule(i);
                    if (nextRule.first == s) {
                        for (auto c : first) {
                            LRState next{i, 0, c};
                            if (answer.find(next) == answer.end()) {
                                answer.insert(next);
                                q.push(next);
                            }
                        }
                    }
                }
            }
        }
        return answer;
    }

    State go(const State& s, typename Grammar<T>::symbol_t c) {
        State next;
        for (LRState lr : s) {
            const auto& rule = grammar.getRule(lr.ruleIdx);
            if (lr.cursor < rule.second.size() && rule.second[lr.cursor] == c) {
                next.insert(LRState{lr.ruleIdx, lr.cursor + 1, lr.rightContext});
            }
        }

        return closure(next);
    }

    State initialState() {
        State s;
        for (int i = 0; i < grammar.getRulesCount(); ++i) {
            if (grammar.getRule(i).first == grammar.getRootSymbol()) {
                s.insert(LRState{i, 0, Grammar<T>::EOL});
            }
        }
        return closure(s);
    }

public:
    CanonicalLR(const Grammar<T>& grammar) : grammar(grammar), states() {
    }

    void buildGoTable() {
        State s0 = initialState();
        states[s0] = 0;
        goTable.resize(1);
        std::queue<State> q;
        q.push(s0);

        while (!q.empty()) {
            State s = q.front();
            q.pop();

            for (auto symbol = grammar.getFirstValidSymbol(),
                         j = grammar.getLastValidSymbol();
                    symbol <= j; ++symbol) {
                State next = go(s, symbol);
                if (!next.empty()) {
                    int idx;
                    auto iter = states.find(next);
                    if (iter == states.end()) {
                        idx = states.size();
                        goTable.emplace_back();
                        states[next] = idx;
                        q.push(next);
                    } else {
                        idx = iter->second;
                    }

                    goTable[states[s]][symbol] = idx;
                }
            }
        }
    }

    bool buildReturnTable() {
        returnTable.resize(goTable.size());
        for (const auto& p : states) {
            const State& s = p.first;
            int idx = p.second;

            for (const LRState& lr : s) {
                const auto& rule = grammar.getRule(lr.ruleIdx);
                if (rule.second.size() == lr.cursor) {
                    auto iter1 = returnTable[idx].find(lr.rightContext);
                    auto iter2 = lr.rightContext == Grammar<T>::EOL ?
                                 goTable[idx].end() : goTable[idx].find(lr.rightContext);
                    if (iter1 == returnTable[idx].end() && iter2 == goTable[idx].end()) {
                        returnTable[idx][lr.rightContext] = lr.ruleIdx;
                    } else {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void printDebugInfo(std::ostream& out = std::cout) {
        out << "States:" << std::endl;
        for (const auto& p: states) {
            out << "State #" << p.second << std::endl;
            for (LRState lr : p.first) {
                grammar.printRule(lr.ruleIdx);
                out << "; pos = " << lr.cursor << "; rightContext: ";
                if (lr.rightContext == Grammar<T>::EOL) {
                    out << "EOL";
                } else {
                    out << grammar.getTerminal(lr.rightContext);
                }
                out << std::endl;
            }
        }
    }

    void exportTables(const std::string& filename) {
        std::ofstream out(filename);

        auto firstSymbol = grammar.getFirstValidSymbol();
        auto lastSymbol = grammar.getLastValidSymbol();
        out <<  "#! /usr/bin/python3\n\n";

        out <<  "class TokenError(Exception):\n"
                "    def __init__(self, text, pos):\n"
                "        self.text = text\n"
                "        self.pos = pos\n\n"
                "    def __str__(self):\n"
                "        return 'Unknown symbol in string \"' + self.text + '\" at pos ' + str(self.pos)\n\n\n";

        out <<  "class ParserError(Exception):\n"
                "    def __init__(self):\n"
                "        pass\n\n\n";

        out <<  "class TokenStream:\n"
                "    def __init__(self, text):\n"
                "        self.table = {\n"
                "            ";

        for (uint32_t i = 0; i < grammar.getTerminalsCount(); ++i) {
            auto s = grammar.kthTerminal(i);
            const T& term = grammar.getTerminal(s);
            out << toPythonString(term) << ": " << s - firstSymbol << ", ";
        }

        out <<  "\n"
                "        }\n"
                "        self.text = text\n"
                "        self.pos = 0\n\n"
                "    def peek(self):\n"
                "        if self.pos >= len(self.text):\n"
                "            return " << Grammar<T>::EOL - firstSymbol << ", '\\0'\n"
                "        c = self.text[self.pos]\n"
                "        if c not in self.table:\n"
                "            raise TokenError(self.text, self.pos)\n"
                "        return self.table[c], c\n\n"
                "    def get(self):\n"
                "        id, c = self.peek()\n"
                "        self.pos += 1\n"
                "        return id, c\n\n\n";

        out <<  "class Parser:\n"
                "    def __init__(self):\n"
                "        self.table = (\n";

        for (uint32_t i = 0; i < states.size(); ++i) {
            out << "            (";
            for (auto symbol = firstSymbol; symbol <= lastSymbol; ++symbol) {
                auto iter = goTable[i].find(symbol);
                if (iter != goTable[i].end()) {
                    out << "(1, " << iter->second << "), ";
                } else {
                    iter = returnTable[i].find(symbol);
                    if (iter != returnTable[i].end()) {
                        out << "(2, " << iter->second << "), ";
                    } else {
                        out << "(0, 0), ";
                    }
                }
            }
            out << "),\n";
        }

        out << "        )\n";
        out << "        self.root_symbol = " << grammar.getRootSymbol() - firstSymbol << "\n";
        out << "        self.root_state = 0\n";
        out << "        self.rules = (\n";
        for (const auto& rule : grammar.getRules()) {
            out << "            (" << (rule.first - firstSymbol) << ", " << rule.second.size() << "),\n";
        }
        out << "        )\n\n";
        out << "    def parse(self, token_stream):\n";
        out << "        stack = [((-1, []), 0)]\n";
        out << "        while True:\n";
        out << "            symbol, ch = token_stream.peek()\n";
        out << "            action = self.table[stack[-1][1]][symbol]\n";
        out << "            if action[0] == 0:\n";
        out << "                raise ParserError()\n";
        out << "            elif action[0] == 1:\n";
        out << "                stack.append(((symbol, [ch]), action[1]))\n";
        out << "                token_stream.get()\n";
        out << "            else:  # action[0] == 2:\n";
        out << "                rule = self.rules[action[1]]\n";
        out << "                stack, children = stack[:len(stack)-rule[1]], [x[0] for x in stack[len(stack)-rule[1]:]]\n";
        out << "                if rule[0] == self.root_symbol:\n";
        out << "                    return rule[0], children\n";
        out << "                go = self.table[stack[-1][1]][rule[0]]\n";
        out << "                if go[0] != 1:\n";
        out << "                    raise ParserError()\n";
        out << "                stack.append(((rule[0], children), go[1]))\n\n\n";

        out << "def get_symbol_name(s):\n";
        out << "    symbol_names = [";

        for (auto i = firstSymbol; i <= lastSymbol; ++i) {
            if (i < 0) {
                out << toPythonString(grammar.getTerminal(i)) << ", ";
            } else {
                out << '"' << grammar.getNonterminal(i) << "\", ";
            }
        }
        out << "]\n";
        out << "    return symbol_names[s]\n\n\n";

        out << "if __name__ == '__main__':\n";
        out << "    text = input('Enter string: ')\n";
        out << "    print(Parser().parse(TokenStream(text)))";
        out.close();
    }
};

#endif //LR1_CANONICALLR_H

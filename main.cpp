#include <iostream>
#include <functional>
#include "Grammar.h"
#include "CanonicalLR.h"

void initGrammarWellFormedParentheses(Grammar<char>& gr) {
    auto L_BRACE = gr.newTerminal('(');
    auto R_BRACE = gr.newTerminal(')');

    auto S = gr.newNonterminal("S");
    auto S_ = gr.newNonterminal("S'");
    gr.setRootSymbol(S_);

    gr.addRule(S_, {S});
    gr.addRule(S, {L_BRACE, S, R_BRACE, S});
    gr.addRule(S, {});
}

void initGrammarArithmetics(Grammar<char>& gr) {
    auto X = gr.newTerminal('x');
    auto PLUS = gr.newTerminal('+');
    auto MUL = gr.newTerminal('*');
    auto L_BRACE = gr.newTerminal('(');
    auto R_BRACE = gr.newTerminal(')');

    auto S_ = gr.newNonterminal("S'");
    auto S = gr.newNonterminal("S");
    auto T = gr.newNonterminal("T");
    auto U = gr.newNonterminal("U");
    gr.setRootSymbol(S_);

    gr.addRule(S_, {S});
    gr.addRule(S, {S, PLUS, T});
    gr.addRule(S, {T});
    gr.addRule(T, {T, MUL, U});
    gr.addRule(T, {U});
    gr.addRule(U, {X});
    gr.addRule(U, {L_BRACE, S, R_BRACE});
}

void buildAnalyzer(std::function<void(Grammar<char> &)> initGrammar, const std::string& filename) {
    Grammar<char> gr;
    initGrammar(gr);
    gr.build();
    gr.printDebugInfo();

    CanonicalLR<char> clr(gr);
    clr.buildGoTable();
    clr.printDebugInfo();
    if (clr.buildReturnTable()) {
        std::cout << "LR(1) analyzer has been successfully built. Exporting to " << filename << std::endl;
        clr.exportTables(filename);
    } else {
        std::cout << "Error while building LR(1) analyzer" << std::endl;
    }
}

int main() {
    std::cout << "Well-Formed Parentheses" << std::endl;
    buildAnalyzer(initGrammarWellFormedParentheses, "wfp.py");

    std::cout << std::endl << "Arithmetics" << std::endl;
    buildAnalyzer(initGrammarArithmetics, "arith.py");
    return 0;
}
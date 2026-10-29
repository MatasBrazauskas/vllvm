#include <fstream>
#include <iostream>
#include <string>

#include "CodeGen.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "StaticCheck.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <source file>\n";
        return 1;
    }

    std::ifstream file{argv[1]};
    if (!file) {
        std::cerr << "cannot open " << argv[1] << '\n';
        return 1;
    }

    Lexer lexer;

    for (std::string line; std::getline(file, line);){
        lexer.tokenize(line);
    }

    const auto tokens = lexer.getTokens();

    Parser parser{tokens};
    const auto program = parser.parse();

    StaticCheck staticCheck;
    staticCheck.staticAnalysis(program);

    //CodeGen codeGen{program};
    //codeGen.print();
}

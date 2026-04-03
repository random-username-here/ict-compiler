/**
 * \file
 * \brief A demo program to tokenize itself 
 */
#include "misclib/parse.hpp"
#include "misclib/dump_stream.hpp"
using namespace misc::color;

int main(int argc, const char *argv[]) {
    if (argc > 2) {
        misc::outs() << "Usage: " << argv[0] << " [SOURCE_FILE]\n";
        return 1;
    }

    const char *fileName = argc == 2 ? argv[1] : __FILE__;
    std::string source = misc::readWholeFile(fileName);
    misc::outs() << BOLD << PURPLE << "==== " << fileName << ":\n\n" << RST;
    misc::outs() << misc::beginBlock << source << misc::endBlock << "\n";

    misc::outs() << BOLD << PURPLE << "==== Tokens:\n" << RST;
    misc::outs() << misc::beginBlock;

    misc::Token tok;
    misc::View leftover = source;
    try {
        do {
            leftover = misc::tokenize(leftover, misc::TOKF_NL, &tok);
            misc::outs() << BLUE << (char) tok.type << DGRAY << " -- " 
                << GREEN << "`" << tok.view << "`";
            if (tok.type == misc::TOK_NUM)
                misc::outs() << DGRAY << " = " << YELLOW << tok.decodeNum();
            else if (tok.type == misc::TOK_STR)
                misc::outs() << DGRAY << " = (decoded) " << GREEN << tok.decodeStr();
            misc::outs() << RST << "\n";

        } while (tok.type != misc::TOK_EOF);
    } catch (const misc::SourceError &e) {
        e.writeFormatted(misc::outs(), __FILE__, source);
    }
}

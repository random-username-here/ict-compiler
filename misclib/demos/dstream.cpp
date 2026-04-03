#include "misclib/dump_stream.hpp"
#include <vector>
using namespace misc::color;

struct Node {
    std::string name;
    // for simplicity sake we are not caring about freeing that
    std::vector<Node*> children;

    // I have added custom overload for printing anything
    // which has `dump(std::ostream&) const` method.
    void dump(std::ostream &ds) const {
        ds << BLUE << name << DGRAY << " @ "
        << YELLOW << this << RST << misc::beginBlock << "\n";
        for (const auto &i : children)
            ds << *i;
        ds << misc::endBlock;       
    }
};

// I would've used unique_ptr, but initializer list does not work with them
#define NODE(n, ...) (new Node { .name = (n), .children = { __VA_ARGS__ } })

int main() {

    auto tree 
        = NODE("Module",
            NODE("Import"),
            NODE("Function",
                NODE("Print"),
                NODE("ReturnStmnt")
            )
        );

    misc::outs().enableColor();
    misc::outs().setIndentStr("    ");
    misc::outs() << "Some random tree:\n";
    misc::outs() << *tree;
    misc::outs() << "Now that tree will be also written to dump.txt\n";

    misc::DumpFileStream fst("dump.txt");
    fst << *tree;

    return 0;
}

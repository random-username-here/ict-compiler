/**
 * \file
 * \brief A demo of how to use `tree.hpp`
 *
 * There are modules, modules have entries,
 * which can be labels or instructions. Instructions
 * have up to 8 arguments.
 *
 */
#include "misclib/tree.hpp"
#include "misclib/dump_stream.hpp"
#include <memory>

using std::make_unique;
using misc::UPtr;
using namespace misc::color;

class Module;
class Instruction;
class Arg;

class Arg : 
        public misc::Item<Arg, Instruction>
{
public:
    void dump(std::ostream &os) const {
        // Each node has access to parent (if it is Item) and its child list (if it is Container)
        os << YELLOW << "Arg of " << parent() << "\n" << RST;
    }
};

class ModuleEntry :
        public misc::Item<ModuleEntry, Module>
{
public:
    virtual void dump(std::ostream &os) const = 0;
    virtual ~ModuleEntry() {};
};

class Instruction : 
        public misc::VarContainer<Instruction, Arg>,
        public ModuleEntry
{
public:
    void dump(std::ostream &os) const override {
        os << BLUE << "Instruction " << this << "\n" << RST;
        os << misc::beginBlock;
        for (const auto &it : *this) {
            os << *it;
        }
        os << misc::endBlock;
    }
};

class Label :
        public ModuleEntry
{
public:
    void dump(std::ostream &os) const override {
        os << GREEN << "Label " << this << "\n" << RST;
    }
};

class Module : 
        public misc::VarContainer<Module, ModuleEntry> 
{
public:
    void dump(std::ostream &os) const {
        os << RED << "Module " << this << "\n" << RST;
        os << misc::beginBlock;
        for (const auto &it : *this) {
            os << *it;
        }
        os << misc::endBlock;
    }
};

int main() {
    auto instr = make_unique<Instruction>();
    instr->push(make_unique<Arg>());
    instr->push(make_unique<Arg>());

    auto i2 = make_unique<Instruction>();

    Module mod;
    mod.push(make_unique<Label>());
    mod.push(std::move(instr));
    mod.push(std::move(i2));

    misc::outs() << "The tree:\n";
    misc::outs() << mod;

    auto second = mod[1];
    second->replaceWith(make_unique<Label>());

    misc::outs() << "After replacing one of its elements:\n";
    misc::outs() << mod;

    return 0;
}

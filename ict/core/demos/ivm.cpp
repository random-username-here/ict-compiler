#include "ict/ir.hpp"
#include "misclib/dump_stream.hpp"
#include <cassert>
#include <vector>

// TODO: pre-copying pass
//
//      %b ... (used after %a)
//          <-- %b.copy IVM.Pull %b
//      %twp Const 2
//      %a Div %b %two
//          <-- replace %b with %b.copy
//
// TODO: do not copy when instruction eats arguments

class Codegen {
protected:
    std::ostream &p_out;
    const ict::Block *p_block; // TODO: replace with function
public:
    Codegen(std::ostream &o, const ict::Block *b) :p_out(o), p_block(b) {}
    virtual void generate() = 0;
};

class IvmCodegen : public Codegen {
    std::vector<size_t> m_stackMap;
    size_t m_curTop;

    bool m_isUsedAfter(const ict::Op *op, const ict::Op *after) {
        for (auto i : op->refs())
            if (i->parent()->slotInParent() > after->slotInParent())
                return true;
        return false;
    }

    void m_pushed(size_t slot) {
        m_stackMap[slot] = m_curTop;
        m_curTop++;
    }

    void m_fetch(const ict::Op *op) {
        size_t slot = op->slotInParent();

        if (slot != m_curTop - 1) {
            p_out << "push " << m_curTop - slot - 1 << '\n';
            p_out << "pull\n";
        } else {
            p_out << "dup\n";
        }
        ++m_curTop;
    }

    void m_emitOp(const ict::Op *op) {
        p_out << "// " << *op;
        switch (op->kind()) {
            case ict::CONST: {
                p_out << "push " << op->arg<ict::ConstArg>(0)->value() << "\n";
                m_pushed(op->slotInParent());
                break;
            }
            case ict::ADD:
            case ict::SUB: {
                m_fetch(op->arg<ict::VRegArg>(1)->ptr());
                m_fetch(op->arg<ict::VRegArg>(0)->ptr());
                switch (op->kind()) {
                    case ict::ADD: p_out << "add\n"; break;
                    case ict::SUB: p_out << "sub\n"; break;
                    default: assert(false);
                }
                m_curTop -= 2;
                m_pushed(op->slotInParent());
                break;
            }
            case ict::DEBUG_PRINT: {
                for (size_t i = 0; i < op->size(); ++i)
                    m_fetch(op->arg<ict::VRegArg>(i)->ptr());
                p_out << "push " << op->size() << "\n";
                p_out << "push _ict_debug_print\n";
                m_curTop -= op->size();
                break;
            }
            default: {
                assert(false);
            }
        }
    }

public:
    IvmCodegen(std::ostream &o, const ict::Block *b) :Codegen(o, b) {
        m_stackMap.resize(b->size());
        m_curTop = 0;
    }

    virtual void generate() {
        for (const auto &op : *p_block)
            m_emitOp(op.get());
    }
    
};

int main() {
    misc::outs().setIndentStr("    ");

    auto mod = ict::Module::create();

    auto func = mod->push(ict::Function::create("demo"));
    auto blk = func->push(ict::Block::create());
    auto builder = ict::BlockBuilder(blk);
    
    auto one = builder.create(ict::CONST, 1);
    auto two = builder.create(ict::CONST, 2);
    auto res = builder.create(ict::ADD, one, two);
    builder.create(ict::DEBUG_PRINT, res);
    
    misc::outs() << *func;

    misc::outs() << "Generated code:\n";
    misc::outs() << misc::beginBlock;
    IvmCodegen codegen(misc::outs(), blk);
    codegen.generate();
    misc::outs() << misc::endBlock;

}

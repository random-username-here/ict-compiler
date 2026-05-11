#include "ict/mod.hpp"
#include "ict/analyses/numbering.hpp"

namespace ict::an {

class NumberingAnalyzer : public Analyzer {
public:
  std::string_view id() const override { return "ict.an.numbering"; }
  std::string_view brief() const override { return "ICT analysis which enumerates all function's arguments, blocks and operations"; }

    virtual size_t analysisId() const override { return typeid(Numbering).hash_code(); }

    virtual UPtr<Analysis> run(const FunctionImpl *impl) const override {
        auto n = std::make_unique<Numbering>();

        size_t idx = 1;
        for (auto i : impl->decl()->args())
            n->m_num[i] = idx++;

        for (auto b : impl->blocks()) {
            n->m_num[b] = idx++;
            for (auto op : b->operations())
                n->m_num[op] = idx++;
        }

        return n;
    }
};

extern "C" Mod *modlib_create() { return new NumberingAnalyzer(); }

}

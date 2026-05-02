#pragma once
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "modlib_manager.hpp"
#include "modlib_mod.hpp"
#include <unordered_map>
namespace ict {

class Manager;

class Analysis
{
public:
    virtual ~Analysis() {}
};

class Analyzer :
        public Mod 
{
public:
    virtual size_t analysisId() const = 0;
    virtual UPtr<Analysis> run(Manager *mgr) const = 0;
    virtual ~Analyzer() {}
};

class Pass :
        public Mod
{
public:
    /**
     * \brief Pass order.
     * Pass orders < 0 are done before instruction selection,
     * passes > 0 are done after. Pass with order = 0 does instruction selection.
     */
    virtual int order() const = 0;
    
    /**
     * \brief Run the pass.
     */
    virtual void run(Manager *mgr) const = 0;
    virtual ~Pass() {}
};

class Backend :
        public Mod
{
public:
    virtual misc::View archName() const = 0;
    virtual int str2lowOp(View name) const = 0;
    virtual misc::View lowOp2str(int k) const = 0;
    virtual bool lowOpRequiresType(int k) const = 0;
    virtual UPtr<Type> createReturnType(Operation *op) const = 0;

    virtual void emit(Manager *mgr, std::ostream &output) const = 0;
    virtual ~Backend() {}
};

class Frontend :
        public Mod
{
public:
    virtual View langName() const = 0;
    virtual bool takesFile(View name) const = 0;
    virtual bool compile(Manager *mgr, Module *into) const = 0;
    virtual ~Frontend() {}
};

class Manager {
    ModManager &m_mm;
    Backend *m_backend = nullptr;
    Frontend *m_frontend = nullptr;
    std::string m_source;
    std::string m_filename;
    UPtr<Module> m_module;
    std::vector<Pass*> m_passes;
    std::unordered_map<size_t, Analyzer*> m_analyzers;
    std::unordered_map<size_t, UPtr<Analysis>> m_analyses;
public:

    static Manager *main();

    Manager(ModManager &mm);
    
    void setSource(View source, View filename);
    void loadSourceFromFile(View filename);

    bool setTargetArch(View name);
    bool setFrontend(View name);
    bool choseFrontendByFileExt();

    template<typename T>
    T *analysis() {
        auto id = typeid(T).hash_code();
        if (m_analyses.count(id) == 0) {
            if (m_analyzers.count(id) == 0)
                return nullptr;
            m_analyses.insert({ id, m_analyzers.at(id)->run(this) });
        }
        return static_cast<T*>(m_analyses.at(id).get());
    }

    template<typename T>
    void invalidateAnalysis() {
        auto id = typeid(T).hash_code();
        m_analyses.erase(id);
    }
    inline void invalidateAllAnalyses() {
        m_analyses.clear();
    }

    bool parse();
    void runAllPasses();
    void emit(std::ostream &out);

    View source() { return m_source; }
    View filename() { return m_filename; }
    Module *module() { return m_module.get(); }
    Backend *backend() { return m_backend; }
    Frontend *frontend() { return m_frontend; }
    const Backend *backend() const { return m_backend; }
    const Frontend *frontend() const { return m_frontend; }
};

}

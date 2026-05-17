#pragma once
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "misclib/parse.hpp"
#include "modlib_manager.hpp"
#include "modlib_mod.hpp"
#include <filesystem>
#include <list>
#include <optional>
#include <unordered_map>
namespace ict {

struct SourceFile {
    std::filesystem::path path, abspath;
    std::string contents;
};

class Manager;

class Analyzer :
        public Mod 
{
public:
    virtual size_t analysisId() const = 0;
    virtual UPtr<Analysis> run(const FunctionImpl *impl) const = 0;
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
     * Returns if pass actually ran, and did not skip because this is bad arch.
     */
    virtual bool run(Manager *mgr) const = 0;
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
    virtual bool lowOpHasSideEffects(int k) const = 0;
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
    virtual bool compile(Manager *mgr, SourceFile *source) const = 0;
    virtual ~Frontend() {}
};


class Manager {
    ModManager &m_mm;
    Backend *m_backend = nullptr;
    UPtr<Module> m_module;
    std::vector<Pass*> m_passes;
    std::unordered_map<size_t, Analyzer*> m_analyzers;
    std::vector<std::filesystem::path> m_includePaths;
    std::list<SourceFile> m_files;

public:

    static Manager *main();

    Manager(ModManager &mm);
    
    SourceFile *loadFile(const std::filesystem::path &path);
    bool hasFile(const std::filesystem::path &file) const;

    SourceFile *findFileByView(View view);
    void printError(const misc::SourceError &err, std::ostream &os);

    void addIncludePath(const std::filesystem::path &p) { m_includePaths.push_back(p); }
    std::optional<std::filesystem::path> resolveInclude(View path);
    std::optional<std::filesystem::path> resolveInclude(View path, const std::filesystem::path &self);

    bool setTargetArch(View name);

    Frontend *findFrontend(View name);
    Frontend *choseFrontendByFileExt(const std::filesystem::path &filename);

    UPtr<Analysis> runAnalyzer(size_t id, const FunctionImpl *on) {
        if (m_analyzers.count(id) == 0)
            return nullptr;
        return m_analyzers.at(id)->run(on);
    }

    void runAllPasses();
    void emit(std::ostream &out);

    Module *module() { return m_module.get(); }
    Backend *backend() { return m_backend; }
    const Backend *backend() const { return m_backend; }
};

}

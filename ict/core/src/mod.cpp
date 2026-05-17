#include "ict/mod.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include "modlib_manager.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <optional>
#include <ostream>

using namespace misc::color;

#define TAG "ict.core"

namespace ict {

static Manager *l_main_mgr = nullptr;

Manager *Manager::main() {
    return l_main_mgr;
}

Manager::Manager(ModManager &mm) :m_mm(mm)
{
    for (auto i : mm.allOfType<Analyzer>())
        m_analyzers[i->analysisId()] = i;
    m_passes = mm.allOfType<Pass>();
    l_main_mgr = this;
    m_module = Module::create();
}

bool Manager::setTargetArch(View name)
{
    for (auto i : m_mm.allOfType<Backend>()) {
        if (i->archName() == name) {
            m_backend = i;
            return true;
        }
    }
    return false;
}

Frontend *Manager::choseFrontendByFileExt(const std::filesystem::path &name)
{
    for (auto i : m_mm.allOfType<Frontend>())
        if (i->takesFile(name.c_str()))
            return i;
    return nullptr;
}

void Manager::runAllPasses()
{
    std::sort(m_passes.begin(), m_passes.end(), [](Pass *a, Pass *b){
        return a->order() < b->order();
    });
    for (auto pass : m_passes) {
        misc::info(TAG) << "Running pass " << ACCENT << pass->id() << RST;
        bool done = pass->run(this);
        if (done)
            misc::info(TAG) << "Pass " << ACCENT << pass->id() << RST << " gave:\n" 
                << misc::beginBlock << *module() << misc::endBlock;
        else
            misc::info(TAG) << "Pass " << ACCENT << pass->id() << RST << " did nothing";
    }
}

std::optional<std::filesystem::path> Manager::resolveInclude(View path) {
    for (auto i : m_includePaths) {
        auto result = i / path;
        if (std::filesystem::is_regular_file(result))
            return result;
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> Manager::resolveInclude(View path, const std::filesystem::path &self) {
    auto rel = self / path;
    if (std::filesystem::is_regular_file(rel))
        return rel;
    return resolveInclude(path);
}

SourceFile *Manager::loadFile(const std::filesystem::path &path) {
    auto abs = std::filesystem::absolute(path);
    auto content = misc::readWholeFile(path.c_str());
    m_files.push_back({
        .path = path,
        .abspath = abs,
        .contents = content
    });
    return &m_files.back();
}

SourceFile *Manager::findFileByView(View view) {
    for (auto &i : m_files) {
        if (i.contents.data() <= view.data() && view.data() + view.size() <= i.contents.data() + i.contents.size())
            return &i;
    }
    return nullptr;
}

bool Manager::hasFile(const std::filesystem::path &file) const {
    auto abs = std::filesystem::absolute(file);
    for (auto &i : m_files)
        if (i.abspath == abs)
            return true;
    return false;
}

void Manager::printError(const misc::SourceError &err, std::ostream &os) {
    auto file = findFileByView(err.where());
    assert(file);
    // TODO: move that here, and add multi-file errors
    err.writeFormatted(os, file->path.c_str(), file->contents);
}

void Manager::emit(std::ostream &out)
{
    assert(module());
    assert(backend());
    misc::info(TAG) << "Running backend " << ACCENT << backend()->id() << RST << " for arch " << ACCENT << backend()->archName() << RST;
    backend()->emit(this, out);
}

};

#include "ict/mod.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include "modlib_manager.hpp"
#include <algorithm>
#include <cassert>
#include <ostream>

using namespace misc::color;

#define TAG "ict.core"

namespace ict {

void Manager::setSource(View source, View filename)
{
    m_source = source;
    m_filename = filename;
}

void Manager::loadSourceFromFile(View filename)
{
    m_filename = filename;
    m_source = misc::readWholeFile(filename);
}

Manager::Manager(ModManager &mm) :m_mm(mm)
{
    for (auto i : mm.allOfType<Analyzer>())
        m_analyzers[i->analysisId()] = i;
    m_passes = mm.allOfType<Pass>();
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

bool Manager::setFrontend(View name)
{
     for (auto i : m_mm.allOfType<Frontend>()) {
        if (i->langName() == name) {
            m_frontend = i;
            return true;
        }
    }
    return false;
}

bool Manager::choseFrontendByFileExt()
{
     for (auto i : m_mm.allOfType<Frontend>()) {
        if (i->takesFile(filename())) {
            m_frontend = i;
            return true;
        }
    }
    return false;
}

void Manager::parse()
{
    assert(frontend());
    misc::info(TAG) << "Running frontend " << ACCENT << frontend()->id() << RST << " for language " << ACCENT << frontend()->langName() << RST;
    m_module = frontend()->compile(this);
    m_module->m_mgr = this;
}
void Manager::runAllPasses()
{
    std::sort(m_passes.begin(), m_passes.end(), [](Pass *a, Pass *b){
        return a->order() < b->order();
    });
    for (auto pass : m_passes) {
        misc::info(TAG) << "Running pass " << ACCENT << pass->id() << RST;
        pass->run(this);
    }
}
void Manager::emit(std::ostream &out)
{
    assert(module());
    assert(backend());
    misc::info(TAG) << "Running backend " << ACCENT << backend()->id() << RST << " for arch " << ACCENT << backend()->archName() << RST;
    backend()->emit(this, out);
}

};

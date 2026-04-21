#include "ict/mod.hpp"
#include "misclib/dump_stream.hpp"
#include "modlib_manager.hpp"

using namespace misc::color;
#define TAG "ict.main"

int main(int argc, const char *argv[]) {

    if (argc < 2) {
        misc::errs() << "Expected source file name!\n";
        return -1;
    }

    misc::info(TAG) << "Loading mods";
    ModManager mm;
    mm.loadAllFromDir(".");
    {
        auto msg = misc::info(TAG);
        msg << "Loaded mods:\n" << misc::beginBlock;
        for (auto &i : mm.all()) {
            msg << ACCENT << i->id() << DGRAY << " -- " << RST << i->brief();
            if (auto backend = dynamic_cast<ict::Backend*>(i.get()))
                msg << PURPLE << " [ict::Backend for " << backend->archName() << "]";
            if (auto frontend = dynamic_cast<ict::Frontend*>(i.get()))
                msg << YELLOW << " [ict::Frontend for " << frontend->langName() << "]";
            if (auto analysis = dynamic_cast<ict::Analyzer*>(i.get()))
                msg << GREEN << " [ict::Anaylsis]";
            if (auto pass = dynamic_cast<ict::Pass*>(i.get()))
                msg << BLUE << " [ict::Pass of order " << pass->order() << "]";

            msg << "\n";
        }
        msg << misc::endBlock;
    }

    misc::info(TAG) << "Will be compiling " << ACCENT << argv[1] << RST;

    ict::Manager mgr(mm);
    mgr.loadSourceFromFile(argv[1]);

    misc::info(TAG) << "Source file contents:\n" << misc::beginBlock << GREEN << mgr.source() << RST << misc::endBlock;

    if (!mgr.choseFrontendByFileExt()) {
        misc::error(TAG) << "Failed to find frontend for " << ACCENT << argv[1] << RST;
        return 1;
    }

    // TODO: read it from args
    if (!mgr.setTargetArch("ivm")) {
        misc::error(TAG) << "Failed to find backend for arch " << ACCENT << "ivm" << RST;
        return 1;
    }

    mgr.parse();
   
    if (!mgr.module()) {
        misc::error(TAG) << "Frontend failed, bailing out!";
        return 1;
    }

    misc::info(TAG) << "Parsed:\n" << misc::beginBlock << *mgr.module() << misc::endBlock;

    mgr.runAllPasses();

    misc::DumpStringStream ds;
    ds.enableColor();
    mgr.emit(ds);

    {
        auto msg = misc::info(TAG);
        msg << "Assembled into:\n" << misc::beginBlock << ds.str() << misc::endBlock;
    }

    misc::DumpFileStream fs("program.s");
    mgr.emit(fs);

    misc::info(TAG) << "Done";
    return 0;
}

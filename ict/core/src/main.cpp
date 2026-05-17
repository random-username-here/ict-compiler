#include "ict/mod.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include "modlib_manager.hpp"
#include <cstring>
#include <filesystem>
#include <getopt.h>
#include <linux/limits.h>
#include <unistd.h>

using namespace misc::color;
#define TAG "ict.main"

static const option l_longOpts[] = {
    { "machine", required_argument, 0, 'm' }, // architecture
    { "include", required_argument, 0, 'I' },
    { "plugins", required_argument, 0, 'p' },
    { "out", required_argument, 0, 'o' },
};

int main(int argc, char *argv[]) {

    if (argc < 2) {
        misc::errs() << "Expected source file name!\n";
        return -1;
    }

    misc::info(TAG) << "Loading mods";
    ModManager mm;

    char buf[PATH_MAX] = {0};
    if (readlink("/proc/self/exe", buf, sizeof(buf)) == -1) {
        misc::error(TAG) << "Failed to get compiler's path: " << strerror(errno);
        return 1;
    }

    std::filesystem::path path(buf);
    path = path.parent_path().parent_path().parent_path(); // go to build dir from ict/core/ict
    mm.loadAllFromDir(path.string());

    std::string machine = "ivm", output = "program.s";
    std::vector<std::filesystem::path> incPaths;

    int opt;
    while ((opt = getopt_long(argc, argv, "m:I:p:o:", l_longOpts, nullptr)) != -1) {
        switch (opt) {
            case 'm': machine = optarg; break;
            case 'I': incPaths.push_back(optarg); break;
            case 'p': mm.loadAllFromDir(optarg); break;
            case 'o': output = optarg; break;
            default:
                misc::error(TAG) << "Unknown option `" << opt << "`";
                return 1;
        }
    }

    mm.initLoaded();
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

    if (optind == argc)
        misc::warn(TAG) << "No input files specified";

    ict::Manager mgr(mm);
    for (auto i : incPaths) {
        misc::info(TAG) << "Adding include path " << ACCENT << i << RST;
        mgr.addIncludePath(i);
    }

    if (machine != "none" && !mgr.setTargetArch(machine)) {
        misc::error(TAG) << "Failed to find backend for arch " << ACCENT << machine << RST;
        return 1;
    }


    for (size_t i = optind; i < argc; ++i) {
        misc::info(TAG) << "Reading file " << ACCENT << argv[i] << RST;
        auto f = mgr.loadFile(argv[i]);
        misc::info(TAG) << ACCENT << argv[i] << RST << " contents:\n" 
            << misc::beginBlock << GREEN << f->contents << RST << misc::endBlock;
        auto frontend = mgr.choseFrontendByFileExt(argv[i]);
        if (!frontend) {
            misc::error(TAG) << "Failed to find frontend for " << ACCENT << argv[i] << RST;
            return 1;
        }
        misc::info(TAG) << "Will be using " << ACCENT << frontend->langName() << RST << " frontend";
        bool ok = frontend->compile(&mgr, f);
        if (!ok) {
            misc::error(TAG) << "Frontend failed, bailing out!";
            return 1;
        }
    }

    misc::info(TAG) << "Sum of all frontends:\n" << misc::beginBlock << *mgr.module() << misc::endBlock;
    misc::info(TAG) << "Verifying...";
    if (mgr.module()->verify()) {
        misc::info(TAG) << misc::GREEN << "Generated IR is good\n";
    } else {
        misc::error(TAG) << misc::RED << "Generated IR has problems, bailing out\n";
        return 1;
    }

    mgr.runAllPasses();

    if (machine == "none") return 0;

    misc::DumpStringStream ds;
    ds.enableColor();
    mgr.emit(ds);

    {
        auto msg = misc::info(TAG);
        msg << "Assembled into:\n" << misc::beginBlock << ds.str() << misc::endBlock;
    }

    misc::DumpFileStream fs(output);
    mgr.emit(fs);

    misc::info(TAG) << "Done";
    return 0;
}

#include "misclib/defs.hpp"
#include "misclib/dump_stream.hpp"
#include <iomanip>
#include <iostream>
namespace misc {

const int DumpStream::xindex = std::ios_base::xalloc();

DumpStream &outs() {
    static DumpStream ds(*std::cout.rdbuf(), true);
    return ds;
}
DumpStream &errs() {
    static DumpStream ds(*std::cerr.rdbuf(), true);
    return ds;
}

impl::LoggerRai log(View topic, LogLevel level) {
    const Color colors[] = { color::RED, color::GREEN, color::YELLOW, color::BLUE, color::PURPLE, color::CYAN };

    size_t n_colors = sizeof(colors) / sizeof(colors[0]);
    size_t hash = std::hash<View>{}(topic);
    errs() << colors[hash % n_colors] << std::right << std::setw(20) << topic << " " << DGRAY << "> " << RST;
    switch (level) {
        case VERBOSE:   errs() << GREEN << "verb" << setAccent(GREEN);        break;
        case INFO:      errs() << BLUE << "info" << setAccent(BLUE);        break;
        case WARN:      errs() << YELLOW << "warn" << setAccent(YELLOW);    break;
        case ERROR:     errs() << RED << BOLD << "ERRR" << setAccent(RED);  break;
    }
    errs() << RST << DGRAY << " > " << RST;
    //                 tag                  > type > 
    //                |01234567890123456789   0123   |
    const char *tab = "                              ";
    errs().pushPrefix(tab);
    return impl::LoggerRai(errs()); // due to rvo, we can do this
}

};

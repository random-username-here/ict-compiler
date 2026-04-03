/**
 * A wrapper which adds fancy formatting capabilities.
 * This is not standard-complaint, TODO: make it complaint
 * later.
 */
#pragma once
#include "misclib/defs.hpp"
#include <cassert>
#include <fstream>
#include <ios>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

namespace misc {

/**
 * ANSI color codes for use with DumpStream.
 */
namespace color {

    struct Color { int ec; constexpr Color(int c) :ec(c) {} };

    constexpr static Color RST = 0;

    constexpr static Color BOLD = 1;
    constexpr static Color DIM = 2;
    constexpr static Color NORMAL = 22;

    constexpr static Color ITALIC = 3;
    constexpr static Color NOITALIC = 23;

    constexpr static Color UND = 4;
    constexpr static Color NOUND = 24;

    constexpr static Color DEL = 9;
    constexpr static Color NODEL = 29;

    constexpr static Color LGRAY = 37;
    constexpr static Color DGRAY = 90;
    constexpr static Color RED = 91;
    constexpr static Color GREEN = 92;
    constexpr static Color YELLOW = 93;
    constexpr static Color BLUE = 94;
    constexpr static Color PURPLE = 95;
    constexpr static Color CYAN = 96;
    constexpr static Color WHITE = 97;

    constexpr static Color ACCENT = -1;
};
using namespace color;

namespace impl {

/**
 * A streambuf which has facilities to enable/disable color
 * and indent blocks.
 */
class DumpStreamBuf : public std::streambuf {

    std::streambuf &m_sbuf;
    View m_indentStr = "    ";
    std::string m_pref;
    std::vector<size_t> m_prefPos;
    bool m_hasColor = false;
    bool m_atNL = true;
    Color m_accent = color::RST;

    void l_writeTabs() {
        m_sbuf.sputn(m_pref.data(), m_pref.size());
    }

public:
    DumpStreamBuf(std::streambuf &sbuf) :m_sbuf(sbuf) {}

protected:
    
    inline int sync() override {
        // just forward
        return m_sbuf.pubsync();
    }

    inline int_type overflow(int_type ch) override {
        // FIXME: proper result handling
        if (m_atNL) {
            l_writeTabs();
            m_atNL = false;
        }
        m_sbuf.sputc(ch);
        if (ch == '\n') {
            m_atNL = true;
        }
        return 1;
    }

    inline std::streamsize xsputn(const char_type *s, std::streamsize n) override {
        for (std::streamsize i = 0; i < n; ++i)
            overflow(s[i]);
        return n; // FIXME
    }

public:

    void putControl(const char *base, size_t size) {
        if (hasColor())
        m_sbuf.sputn(base, size);
    }

    void setIndentStr(View str) {
        m_indentStr = str;
    }

    void pushPrefix(View str) {
        m_prefPos.push_back(m_pref.size());
        m_pref += str;
    }

    void popPrefix() {
        if (m_prefPos.empty()) return;
        m_pref.resize(m_prefPos.back());
        m_prefPos.pop_back();
    }

    void indent(int d) {
        if (d < 0) {
            for (size_t i = 0; i < -d; ++i)
                popPrefix();
        } else {
            for (size_t i = 0; i < d; ++i)
                pushPrefix(m_indentStr);
        }
    }

    Color accentColor() const { return m_accent; }
    void setAccentColor(Color c) { m_accent = c; }
    bool hasColor() const { return m_hasColor; }
    void enableColor(bool en = true) { m_hasColor = en; }

    bool isAtNewline() const { return m_atNL; }
};

};

class DumpStream : public std::ostream {
    impl::DumpStreamBuf m_buf;
public:
    static const int xindex;
    DumpStream(std::streambuf &buf, bool color = false) :m_buf(buf) {
        init(&m_buf);
        m_buf.enableColor(color);
        this->pword(xindex) = this;
    }
    void indent(int d) {
        m_buf.indent(d);
    }
    Color accentColor() const { return m_buf.accentColor(); }
    void setAccentColor(Color c) { m_buf.setAccentColor(c); }

    bool hasColor() const { return m_buf.hasColor(); }
    void enableColor(bool en = true) { m_buf.enableColor(en); }

    void pushPrefix(View str) { m_buf.pushPrefix(str); }
    void popPrefix() { m_buf.popPrefix(); }
    void setIndentStr(View str) { m_buf.setIndentStr(str); }
    impl::DumpStreamBuf &buf() { return m_buf; }
    bool isAtNewline() const { return m_buf.isAtNewline(); }
};


class DumpFileStream : public DumpStream {
    std::filebuf m_fbuf;
public:
    DumpFileStream(const char *fname) :m_fbuf(), DumpStream(m_fbuf) {
        m_fbuf.open(fname, std::ios::out);
    }
    DumpFileStream(const std::string &fname) :m_fbuf(), DumpStream(m_fbuf) {
        m_fbuf.open(fname, std::ios::out);
    }
};

class DumpStringStream : public DumpStream {
    std::stringbuf m_sbuf;
public:
    DumpStringStream() :m_sbuf(), DumpStream(m_sbuf) {}
    std::string str() const { return m_sbuf.str(); }
};

namespace color { // please the ADL
inline std::ostream &operator<<(std::ostream &os, color::Color c) {
    if (os.pword(DumpStream::xindex) != &os)
        return os;
    DumpStream &ds = static_cast<DumpStream&>(os);
    if (c.ec == color::ACCENT.ec)
        c = ds.accentColor();
    if (ds.hasColor()) {
        char str[16];
        int n = snprintf(str, sizeof(str)-1, "\x1b[%dm", c.ec);
        ds.buf().putControl(str, n);
    }
    return os;
}
};

struct setAccent {
    Color c;
    constexpr setAccent(Color c) :c(c) {}
};

inline std::ostream &operator<<(std::ostream &os, setAccent c) {
    if (os.pword(DumpStream::xindex) != &os)
        return os;
    DumpStream &ds = static_cast<DumpStream&>(os);
    ds.setAccentColor(c.c);
    return os;
}

struct indentBy {
    int d;
    constexpr indentBy(int d) :d(d) {}
};

constexpr indentBy beginBlock = indentBy(1);
constexpr indentBy endBlock = indentBy(-1);

inline std::ostream &operator<<(std::ostream &os, indentBy c) {
    if (os.pword(DumpStream::xindex) != &os)
        return os;
    DumpStream &ds = static_cast<DumpStream&>(os);
    ds.indent(c.d);
    return os;
}

DumpStream &outs();
DumpStream &errs();

enum LogLevel {
    VERBOSE,
    INFO,
    WARN,
    ERROR
};

namespace impl {
    class LoggerRai {
        bool m_finished = false;
        DumpStream &m_ds;
    public:
        LoggerRai(DumpStream &ds) :m_ds(ds) {}
        LoggerRai(const LoggerRai &) = delete;
        LoggerRai &operator=(const LoggerRai &) = delete;

        DumpStream &stream() {
            return m_ds;
        }

        void end() {
            if (m_finished) return;
            if (!m_ds.isAtNewline()) m_ds << '\n';
            m_ds << RST;
            m_ds.popPrefix();
            m_finished = true;
        }

        ~LoggerRai() {
            end();
        }
        template<typename T>
        std::ostream &operator<<(const T &v) {
            return m_ds << v;
        }
    };

};

impl::LoggerRai log(View topic, LogLevel level);
inline impl::LoggerRai verb(View topic) { return log(topic, VERBOSE); }
inline impl::LoggerRai info(View topic) { return log(topic, INFO); }
inline impl::LoggerRai warn(View topic) { return log(topic, WARN); }
inline impl::LoggerRai error(View topic) { return log(topic, ERROR); }

};

// dump dumpable objects
template<
    typename T,
    typename = decltype(std::declval<const T&>().dump(std::declval<std::ostream&>()))
>
inline std::ostream &operator<<(std::ostream &os, const T &c) {
    c.dump(os);
    return os;
}

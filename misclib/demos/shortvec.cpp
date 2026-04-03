/**
 * \file
 * \brief A smoke test of `ShortVec`.
 */
#include "misclib/shortvec.hpp"
#include "misclib/dump_stream.hpp"
#include <iostream>
using namespace misc::color;

[[noreturn]] void die(const char *fmt) {
    misc::outs() << "die(): " << fmt << "\n";
    abort();
}

void title(const char *t) {
    misc::outs() << "\n" << BOLD << PURPLE
        << "====== " << t << "\n\n" << RST;
}

#define LOG_METHOD()\
    do {\
        misc::outs() << BLUE << "    (this = " << this << ") "\
            << __PRETTY_FUNCTION__ << "\n" << RST;\
    } while (0)

/// a crash dummy who observes what monitors what happens to him
class Dummy {
public:
    Dummy() {
        LOG_METHOD();
    }
    ~Dummy() {
        LOG_METHOD();
    }
    Dummy(const Dummy &obj) {
        LOG_METHOD();
    }
    Dummy(Dummy &&obj) { // steal him
        LOG_METHOD();
    }
    Dummy &operator=(const Dummy &obj) {
        LOG_METHOD();
        return *this;
    }
    Dummy &operator=(Dummy &&obj) {
        LOG_METHOD();
        return *this;
    }
};

int main() {

    title("Empty vector");
    {
        puts("Before vector ctor");
        {
            misc::ShortVec<Dummy, 16> vec;
            puts("After vector ctor, no object should've initialized");
        }
        puts("After vector destroyed, no object should've been destroyed");
    }

    title("Push two, pop one");
    {
        Dummy val;
        {
            misc::ShortVec<Dummy, 16> vec;
            puts("Pushing lvalue");
            vec.push(val);
            puts("Pushed, pushing again");
            vec.push(val);
            puts("Popping one");
            vec.pop();
            puts("Done, now object in shortvec should be destroyed");
        }
        puts("And lvalue we were pushing");
    }

    title("Initializer list");
    {
        puts("Now we will construct vector with two elements");
        puts("Initializer list provides copy-only access, so no move ctors here");
        misc::ShortVec<Dummy, 16> vec = {Dummy(), Dummy()};
        puts("Done, vector will be destroyed");
    }

    title("Move-pushing and move-popping elements");
    {
        puts("We will move-push element into the shortvec");
        puts("Vector is created");
        misc::ShortVec<Dummy, 16> vec;
        puts("Element is pushed");
        vec.push(Dummy());
        puts("Element is popped");
        Dummy v = vec.pop();
        puts("Done, now vector and popped element will be destroyed");
    }

    title("Copying vector");
    {
        puts("Initializing vector A with one element");
        misc::ShortVec<Dummy, 16> a;
        a.push(Dummy());
        puts("Copy-constructing vector B");
        {
            auto b = a;
            puts("Done, now B will be destroyed");
        }
        puts("And A will also");
    }

    title("Moving vector");
    {
        puts("Initializing A again");
        misc::ShortVec<Dummy, 16> a;
        a.push(Dummy());
        puts("Move-constructing vector B");
        {
            auto b = std::move(a);
            puts("Done, B will die now");
        }
        puts("And A will too");
    }

    title("Iterating vector (of ints)");
    {
        puts("Initializing it");
        misc::ShortVec<int, 16> vec = {1, 2, 3, 4, 5};
        puts("Iterating");
        for (int i : vec)
            std::cout << "Element with value of " << i << '\n';
        puts("The end");
    }
}

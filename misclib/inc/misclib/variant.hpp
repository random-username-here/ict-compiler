#pragma once

#include <variant>
namespace misc {

/** std::variant with some helpful methods */
template<typename ...Ts>
class Variant : public std::variant<Ts...> {
public:
    /** Check if this holds given type. */
    template<typename T>
    bool is() const { return std::holds_alternative<T>(*this); }

    /** Obtain given type out of this. */
    template<typename T>
    T &get() { return std::get<T>(*this); }

    template<typename T>
    const T &get() const { return std::get<T>(*this); }

    /** Obtain given type, or die trying with an exception. */
    template<typename T, typename Ex, typename ...Params>
    T& getOrDie(Params ...params) {
        if (!is<T>())
            throw Ex(params...);
        return get<T>();
    }

    template<typename T, typename Ex, typename ...Params>
    const T& getOrDie(Params ...params) const {
        if (!is<T>())
            throw Ex(params...);
        return get<T>();
    }
};

};

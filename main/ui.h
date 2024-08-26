#pragma once

#include <array>
#include <functional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

struct UiSettings {
    std::array<uint8_t, 4> mState{}; // Up to 4 levels deep (increase if needed)
    int8_t mDepth {-1};
};

// Allows overloading lambdas for std::visit
template<typename ... Ts>
struct Overload : Ts ... {
    using Ts::operator() ...;
};
template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

// Helper to detect if a type has a method
#define GENERATE_HAS(func) \
template <typename T, typename, typename... Args> \
struct detect_##func : std::false_type {}; \
template <typename T, typename... Args> \
struct detect_##func<T, std::void_t<decltype(std::declval<const T&>().func(std::declval<Args>()...))>, Args...> : std::true_type {}; \
template <typename T, typename... Args> \
using has_##func = detect_##func<T, void, Args...>;


// Generate methods the classes can implement to override defaults
GENERATE_HAS(name) // What to print by parents
GENERATE_HAS(render) // What to render to screen
GENERATE_HAS(sub) // Used recursively to get deeper elements
GENERATE_HAS(capture_input) // Used to automatically do depth++
GENERATE_HAS(button)
GENERATE_HAS(button_menu)
GENERATE_HAS(button_updown)
GENERATE_HAS(button_back)

// Forward declare for render() calls
class Display;
class Time;

namespace UI {

struct Name {
    std::string baseName;
    std::string name() const { return baseName; };
};

struct Text {
    std::function<std::string()> fName;
    std::string name() const { return fName(); };
};


struct RefBool : public Name {
    bool& ref;
    std::string name() const { return (ref ? "X " : "O ") + baseName; }
    void button_menu() const {ref = !ref;} // Toggles
};
struct Bool : public Name {
    std::function<bool()> get;
    std::function<void(bool)> set;

    std::string name() const { return (get() ? "X " : "O ") + baseName; }
    void button_menu() const {set(!get());} // Toggles
};

// Small options that have small int items that we loop trough them
template<typename T>
struct Loop : public Name {
    std::function<T()> get;
    std::function<void()> tick;

    std::string name() const { return std::to_string(get()) + " " + baseName; }
    void button_menu() const {tick();}
};

struct Action : public Name {
    std::function<void()> action;

    void button_menu() const {action();}
};

struct Custom : public Name {
    std::function<void(bool)> change;
    std::function<void()> action;
    std::function<void()> render;
};

// NumberItem capture the user input and will be affected by up/down
struct Number : public Name {
    std::function<int()> get;
    std::function<void(int)> change;

    void capture_input() const {};

    std::string name() const { return std::to_string(get()) + " " + baseName; }
    void button_updown(int v) const {change(v);}
    void render(Display&) const;
};

struct DateTime : public Name {
    Time& mTime;

    void capture_input() const {};

    void button_menu() const;
    void button_updown(int v) const;
    void render(Display&) const;
};

class Sub;
class Menu;

using Any = std::variant<
    DateTime,
    Menu,
    Sub,
    Action,
    Loop<int>,
    Bool,
    RefBool,
    Number,
    Text,
    Name>;

struct Sub : public Name {
    std::vector<Any> items;

    void capture_input() const {};
    const Any& sub(uint8_t index) const {
        return items[index];
    };
    void button_menu() const;
    void button_updown(int b) const;
    int index() const;
};

struct Menu : public Sub {
    void render(Display&) const;
};

} // namespace UI


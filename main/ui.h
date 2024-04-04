#pragma once

template<unsigned N>
struct FixedString 
{
    char buf[N + 1]{};
    constexpr FixedString(char const* s) 
    {
        for (unsigned i = 0; i != N; ++i) buf[i] = s[i];
    }
    constexpr operator char const*() const { return buf; }
    //constexpr operator std::string_view() const { return buf; }

    // not mandatory anymore
    auto operator<=>(const FixedString&) const = default;
};
template<unsigned N> FixedString(char const (&)[N]) -> FixedString<N - 1>;

constexpr FixedString noName = "UNKNOWN";
// Template to hold configuration entry
template <typename T, T Low = std::numeric_limits<T>::min(), T High = std::numeric_limits<T>::max(), FixedString Name = noName>
struct NumConfigEntry {
    T mData{};
    operator T&() {return mData;}
    const char* name() const { return Name; }
    static constexpr const T mLow = Low;
    static constexpr const T mHigh = High;
};

#include <variant>
#include <string>

struct Item {
    const char * name;
    //const char * desc;
};

struct TextItem : public Item {
    // Print some arbitrary text
    std::function<std::string()> get;
};

struct BoolItem : public Item {
    std::function<bool()> get;
    std::function<void(bool)> set;
    void toggle() { set(!get()); }
};

// Small options that have small int items
struct LoopItem : public Item {
    std::function<int()> get;
    std::function<void()> tick;
};

// NumberItem capture the user input and will be affected by up/down
struct NumberItem : public Item {
    std::function<int()> get;
    std::function<void(bool)> change;
};

struct ActionItem : public Item {
    std::function<void()> action;
};

struct CustomItem : public Item {
    std::function<void(bool)> change;
    std::function<void()> action;
    std::function<void()> render;
};

struct MenuItem; // Forward declare

using AnyItem = std::variant<
    MenuItem,
    ActionItem,
    LoopItem,
    BoolItem,
    NumberItem,
    Item>;

struct MenuItem : public Item {
    std::vector<AnyItem> items;
};
#include "core.h"

void Core::showMenu(const AnyItem& item, const uint8_t index) {
    // Text size for all the UI
    mDisplay.setTextSize(3);

    std::visit([&](auto&& e){
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, MenuItem>) {
            // Print the menu title on top, centered
            mDisplay.setTextColor(1, 0);
            mDisplay.print(" ");
            mDisplay.println(e.name);
            //mDisplay.setCursor(mDisplay.getCursorX(), mDisplay.getCursorY() + 5);
            for(auto i = 0;i < e.items.size(); i++) {
                if (i == index) {
                    mDisplay.setTextColor(0, 1);
                } else {
                    mDisplay.setTextColor(1, 0);
                }
                // Depending on the menuitem type we might print differently
                std::visit([&](auto&& sub){
                    using U = std::decay_t<decltype(sub)>;
                    if constexpr (std::is_same_v<U, BoolItem>) {
                        // BoolItem show value + name
                        mDisplay.print(sub.get() ? "O " : "X ");
                        mDisplay.println(sub.name);
                    } else if constexpr (std::is_same_v<U, LoopItem>) {
                        // BoolItem show value + name
                        mDisplay.print(sub.get());
                        mDisplay.print(' ');
                        mDisplay.println(sub.name);
                    } else if constexpr (std::is_same_v<U, NumberItem>) {
                        // BoolItem show value + name
                        mDisplay.print(sub.get());
                        mDisplay.print(' ');
                        mDisplay.println(sub.name);
                    } else {
                        // Default just print the name
                        mDisplay.println(sub.name);
                    }
                }, e.items[i]);
            }
        } else if constexpr (std::is_same_v<T, NumberItem>) {
            // TODO: An Action Item renders its upper level menu, but with a selection mark
            mDisplay.setTextColor(1, 0);
            mDisplay.println(e.get());
        }
    }, item);

    mDisplay.writeAllAndRefresh(); 
    // This leaves the backbuffer in wrong state, but will be rewritten
}
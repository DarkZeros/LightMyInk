#include "ui.h"
#include "settings.h"
#include "display.h"

namespace {
    auto& ui = kSettings.mUi;
    void updownUiState(int add, int size) {
        ui.mState[ui.mDepth] = (ui.mState[ui.mDepth] + size + add) % size;
    }
    int ind(std::size_t size) {
        return (ui.mState[ui.mDepth] + size) % size;
    }
    void renderHeader(Display& mDisplay, const std::string& name) {
        // Text size for all the Menu
        mDisplay.setTextSize(3);

        // Print the menu title on top, centered
        mDisplay.setTextColor(1, 0);
        mDisplay.print(" "); // TODO: Center properly
        mDisplay.println(name.c_str());
    }
}

namespace UI {

void Sub::button_menu() const {
    auto& item = items[index()];
    std::visit([&](auto& e) {
        if constexpr (has_capture_input<decltype(e)>::value) {
            ui.mDepth++; // Increase depth and let the sub handle it
        } else if constexpr (has_button<decltype(e)>::value) {
            e.button(Touch::Menu);
        } else if constexpr (has_button_menu<decltype(e)>::value) {
            e.button_menu();
        } else {
            ESP_LOGE("","Uimplemented!");
        }
    }, item);
}
void Sub::button_updown(int b) const {
    updownUiState(-b, items.size());
}

int Sub::index() const {
    return ind(items.size());
}

void Menu::render(Display& mDisplay) const {
    renderHeader(mDisplay, baseName);
    
    auto ind = index();
    for(auto i = 0; i < items.size(); i++) {
        if (i == ind) {
            mDisplay.setTextColor(0, 1);
        } else {
            mDisplay.setTextColor(1, 0);
        }
        std::visit([&](auto& e){
            if constexpr (has_name<decltype(e)>::value) {
                mDisplay.println(e.name().c_str());
            } else {
                // Default just print the name
                mDisplay.println("NO_NAME");
            }
        }, items[i]);
    }

    mDisplay.writeAllAndRefresh(); 
}

void Number::render(Display& mDisplay) const {
    renderHeader(mDisplay, baseName);

    mDisplay.println();
    mDisplay.print("< ");
    mDisplay.print(get());
    mDisplay.println(" >");
    mDisplay.println();

    mDisplay.writeAllAndRefresh(); 
}

namespace {
    std::array<std::tuple<const char *, const char *, int, int>, 6> kDateTime = {{
        {" ", "%02d", 2, 0},
        {":", "%02d", 1, 0},
        {":", "%02d", 0, 0},
        {"\n\n", "%02d", 4, 0},
        {"/", "%02d", 5, 0},
        {"/", "%04d", 6, 1970},
    }};
}

void DateTime::button_menu() const{
    updownUiState(1, 6);
}
void DateTime::button_updown(int v) const{
    //Take current selected item up/down
    auto selected = ind(kDateTime.size());
    auto cur = mTime.getElements();
    auto curCast = reinterpret_cast<uint8_t *>(&cur);
    curCast[std::get<2>(kDateTime[selected])] += v;
    mTime.setTime(makeTime(cur));
}
void DateTime::render(Display& mDisplay) const {
    renderHeader(mDisplay, baseName);

    mDisplay.println();

    auto selected = ind(kDateTime.size());
    auto time = mTime.getElements();
    auto timeCast = reinterpret_cast<uint8_t *>(&time);

    for (auto i=0; i<kDateTime.size(); i++) {
        auto& [pre, mid, ind, add] = kDateTime[i];
        mDisplay.print(pre);
        if (i == selected) {
            mDisplay.setTextColor(0, 1);
        }
        mDisplay.printf(mid, timeCast[ind] + add);
        mDisplay.setTextColor(1, 0);
    }

    mDisplay.writeAllAndRefresh(); 
}

} // namespace UI

#include "ui.h"
#include "settings.h"
#include "display.h"

namespace {
    auto& ui = kSettings.mUi;
    void updownUiState(int add, int size) {
        ui.mState[ui.mDepth] = (ui.mState[ui.mDepth] + size + add) % size;
    }
    int curIndex(std::size_t size) {
        return (ui.mState[ui.mDepth] + size) % size;
    }

    void renderHeader(Display& mDisplay, const std::string& name, size_t size = 2) {
        // Text size for all the Menu
        mDisplay.setTextSize(size);

        // Print the menu title on top, centered
        mDisplay.setTextColor(1, 0);
        auto w = mDisplay.getTextRect(name.c_str()).w;
        mDisplay.setCursor((mDisplay.WIDTH - w) / 2, mDisplay.getCursorY());
        mDisplay.println(name.c_str());
        // Underscore the title then leave a gap
        mDisplay.drawFastHLine((mDisplay.WIDTH - w) / 2, mDisplay.getCursorY(), w, 1);
        mDisplay.setCursor(mDisplay.getCursorX(), mDisplay.getCursorY() + 5);
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
    return curIndex(items.size());
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
    std::array<std::tuple<bool, const char *, const char *, int, int, int, const char *>, 6> kDateTime = {{
        {true, " ", "%02d", 2, 0, 23, ""},
        {false, ":", "%02d", 1, 0, 59, ""},
        {false, ":", "%02d", 0, 0, 59, "\n\n\n"},
        {true, "", "%02d", 4, 0, 28, ""},
        {false, "/", "%02d", 5, 0, 12, ""},
        {false, "/", "%04d", 6, 1970, 255, "\n\n\n"},
    }};
}

void DateTime::button_menu() const{
    updownUiState(1, kDateTime.size());
}
void DateTime::button_updown(int v) const{
    //Take current selected item up/down
    auto selected = curIndex(kDateTime.size());
    auto cur = mTime.getElements();
    auto curCast = reinterpret_cast<uint8_t *>(&cur);
    auto& val = curCast[std::get<3>(kDateTime[selected])];
    if (val == 0 && v == -1)
        val = std::get<4>(kDateTime[selected]);
    else
        val += v;
    mTime.setTime(makeTime(cur));
}
void DateTime::render(Display& mDisplay) const {
    renderHeader(mDisplay, baseName);

    mDisplay.println();

    // Fixed width size
    auto w = mDisplay.getTextRect("00/00/0000").w;

    auto selected = curIndex(kDateTime.size());
    auto time = mTime.getElements();
    auto timeCast = reinterpret_cast<uint8_t*>(&time);

    mDisplay.print("\n");

    for (auto i=0; i<kDateTime.size(); i++) {
        auto& [center, pre, mid, ind, add, _, end] = kDateTime[i];
        if (center)
            mDisplay.setCursor((mDisplay.WIDTH - w) / 2, mDisplay.getCursorY());
        mDisplay.print(pre);
        char text[6];
        snprintf(text, sizeof(text), mid, timeCast[ind] + add);
        auto textSize = strlen(text);
        if (i == selected) {
            // Draw /\ and \/
            auto x = mDisplay.getCursorX(), y = mDisplay.getCursorY();
            mDisplay.setCursor(x, y - 2 * 8);
            if (textSize == 4)
                mDisplay.print(" ");
            mDisplay.print("/\\");
            mDisplay.setCursor(x, y + 2 * 8);
            if (textSize == 4)
                mDisplay.print(" ");
            mDisplay.print("\\/");
            mDisplay.setCursor(x, y);
            mDisplay.setTextColor(0, 1);
        }
        mDisplay.printf(text);
        mDisplay.setTextColor(1, 0);
        mDisplay.printf(end);
    }

    // Day of the week
    auto weekday = dayStr(timeCast[3]);
    auto w_dayofWeek = mDisplay.getTextRect(weekday).w;
    mDisplay.setCursor((mDisplay.WIDTH - w_dayofWeek) / 2, mDisplay.getCursorY());
    mDisplay.printf(weekday);

    mDisplay.writeAllAndRefresh(); 
}

} // namespace UI

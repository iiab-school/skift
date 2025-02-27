#include <karm-main/main.h>
#include <karm-ui/app.h>
#include <karm-ui/dialog.h>
#include <karm-ui/layout.h>
#include <karm-ui/react.h>
#include <karm-ui/row.h>
#include <karm-ui/scafold.h>
#include <karm-ui/scroll.h>
#include <karm-ui/view.h>

#include "app.h"

namespace Settings {

Ui::Child sidebar(State const &state) {
    return Ui::vscroll(
        Ui::spacing(
            8,
            Ui::vflow(
                8,

                Ui::navRow(state.page == Page::ACCOUNT, Model::bind<GoTo>(Page::ACCOUNT), Mdi::ACCOUNT, "Accounts"),
                Ui::navRow(state.page == Page::PERSONALIZATION, Model::bind<GoTo>(Page::PERSONALIZATION), Mdi::PALETTE, "Personalization"),
                Ui::navRow(state.page == Page::APPLICATIONS, Model::bind<GoTo>(Page::APPLICATIONS), Mdi::WIDGETS_OUTLINE, "Applications"),

                Ui::navRow(state.page == Page::SYSTEM, Model::bind<GoTo>(Page::SYSTEM), Mdi::LAPTOP, "System"),
                Ui::navRow(state.page == Page::NETWORK, Model::bind<GoTo>(Page::NETWORK), Mdi::WIFI, "Network"),
                Ui::navRow(state.page == Page::SECURITY, Model::bind<GoTo>(Page::SECURITY), Mdi::SECURITY, "Security & Privacy"),

                Ui::navRow(state.page == Page::UPDATES, Model::bind<GoTo>(Page::UPDATES), Mdi::UPDATE, "Updates"),
                Ui::navRow(state.page == Page::ABOUT, Model::bind<GoTo>(Page::ABOUT), Mdi::INFORMATION_OUTLINE, "About"))));
}

/* --- Pages ---------------------------------------------------------------- */

Ui::Child pageContent(State const &state) {
    switch (state.page) {
    case Page::HOME:
        return pageHome(state);

    case Page::ABOUT:
        return pageAbout(state);

    default:
        return Ui::grow(
            Ui::center(
                Ui::text("Content")));
    }
}

/* --- Body ----------------------------------------------------------------- */

Ui::Child appBody(State const &state) {
    if (state.sidebarOpen) {
        return Ui::hflow(
            sidebar(state),
            Ui::separator(),
            pageContent(state));
    }

    return pageContent(state);
}

Ui::Child app() {
    return Ui::reducer<Model>({}, [](State const &state) {
        auto titlebar = Ui::titlebar(
            Mdi::COG,
            "Settings",
            Ui::TitlebarStyle::DEFAULT);

        return Ui::dialogLayer(
            Ui::minSize(
                {700, 500},
                Ui::vflow(
                    titlebar,
                    Ui::toolbar(
                        Ui::button(Model::bind<ToggleSidebar>(), Ui::ButtonStyle::subtle(), state.sidebarOpen ? Mdi::MENU_OPEN : Mdi::MENU),
                        Ui::button(Model::bind<GoTo>(Page::HOME), Ui::ButtonStyle::subtle(), Mdi::HOME),
                        Ui::button(NONE, Ui::ButtonStyle::subtle(), Mdi::ARROW_LEFT),
                        Ui::button(NONE, Ui::ButtonStyle::subtle(), Mdi::ARROW_RIGHT)),
                    appBody(state) | Ui::grow())));
    });
}

} // namespace Settings

Res<> entryPoint(Ctx &ctx) {
    return Ui::runApp(ctx, Settings::app());
}

#include "row.h"

namespace Karm::Ui {

Child row(Child child) {
    return child |
           vcenter() |
           spacing(16) |
           minSize({UNCONSTRAINED, 64});
}

Child row(Opt<Child> leading, String title, Opt<String> subtitle, Opt<Child> trailing) {
    auto lead = leading
                    ? *leading |
                          center() |
                          sizing(26, {UNCONSTRAINED, 26}) |
                          spacing({0, 0, 12, 0})
                    : empty();

    auto t = subtitle
                 ? vflow(
                       8,
                       labelLarge(title),
                       labelMedium(*subtitle))
                 : labelLarge(title);

    auto trail = trailing
                     ? spacing({12, 0, 0, 0}, sizing(26, {UNCONSTRAINED, 26}, center(*trailing)))
                     : empty();

    return minSize(
        {UNCONSTRAINED, 48},
        spacing(
            {12, 0},
            hflow(
                0,
                Layout::Align::VCENTER | Layout::Align::HFILL,
                lead,
                t | grow(),
                trail)));
}

Child titleRow(String t) {
    return titleMedium(t) | spacing({12, 16, 12, 8});
}

Child pressableRow(OnPress onPress, Opt<Child> leading, String title, Opt<String> subtitle, Opt<Child> trailing) {
    return button(
        std::move(onPress),
        ButtonStyle::subtle(),
        row(
            leading,
            title,
            subtitle,
            trailing));
}

Child buttonRow(OnPress onPress, Mdi::Icon i, String title, String subtitle) {
    return button(
        std::move(onPress),
        ButtonStyle::subtle(),
        row(
            icon(i, 24),
            title,
            subtitle,
            NONE));
}

Child buttonRow(OnPress onPress, String title, String text) {
    return row(
        NONE,
        title,
        NONE,
        button(std::move(onPress), ButtonStyle::primary(), text));
}

Child toggleRow(bool value, OnChange<bool> onChange, String title) {
    return row(
        NONE,
        title,
        NONE,
        toggle(value, std::move(onChange)));
}

Child checkboxRow(bool value, OnChange<bool> onChange, String title) {
    return row(
        NONE,
        title,
        NONE,
        checkbox(value, std::move(onChange)));
}

Child radioRow(bool value, OnChange<bool> onChange, String title) {
    return row(
        radio(value, std::move(onChange)),
        title,
        NONE,
        NONE);
}

Child sliderRow(SliderStyle style, f64 value, OnChange<f64> onChange, String title) {
    return row(
        NONE,
        title,
        NONE,
        slider(style, value, std::move(onChange)));
}

Child sliderRow(f64 value, OnChange<f64> onChange, String title) {
    return sliderRow(SliderStyle::regular(), value, std::move(onChange), title);
}

Child colorRow(Gfx::Color c, OnChange<Gfx::Color> onChange, String title) {
    return row(
        NONE,
        title,
        NONE,
        color(c, std::move(onChange)));
}

Child navRow(bool selected, OnPress onPress, Mdi::Icon i, String title) {
    auto buttonStyle = ButtonStyle::regular();

    buttonStyle.idleStyle = {
        .borderRadius = 4,
        .backgroundPaint = selected ? Gfx::ZINC800 : Gfx::ALPHA,
    };

    auto indicator = box(BoxStyle{
                             .borderRadius = 99,
                             .backgroundPaint = selected ? Gfx::BLUE600 : Gfx::ALPHA,
                         },
                         empty(4));

    return button(
        std::move(onPress),
        buttonStyle,
        spacing(
            {0, 8, 12, 8},
            hflow(
                indicator,
                empty(8),
                icon(i, 26),
                empty(12),
                labelMedium(title) | center())));
}

Child treeRow(Opt<Child> leading, String title, Opt<String> subtitle, Child child) {
    return state(false, [=](State<bool> state) {
        return vflow(
            0,
            pressableRow(
                state.bindToggle(),
                leading,
                title,
                subtitle,
                icon(state.value() ? Mdi::CHEVRON_UP : Mdi::CHEVRON_DOWN, 24)),
            state.value() ? spacing(
                                {38, 0, 0, 0},
                                child)
                          : empty());
    });
}

Child treeRow(Opt<Child> leading, String title, Opt<String> subtitle, Children children) {
    return treeRow(leading, title, subtitle, vflow(children));
}

} // namespace Karm::Ui

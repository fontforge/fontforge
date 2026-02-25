/* Copyright (C) 2025 by Maxim Iorsh
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "css_builder.hpp"

#include <cstring>
#include <functional>
#include <map>
#include <vector>

namespace ff::app {

using CssPropertyEvalCB =
    std::function<std::string(const GBox& box_resource, bool enabled)>;

static std::string css_color(Color col, bool enabled = true) {
    // There is no convenient hex formatting in C++17...
    char value_string[250] = "\0";

    // Build semitransparent color expression for disabled elements
    const char* format = enabled ? "#%06x" : "alpha(#%06x, 0.5)";

    sprintf(value_string, format, col);
    return value_string;
}

template <auto GBox::* PROP>
std::string color_property(const GBox& box_resource, bool enabled) {
    return css_color(box_resource.*PROP, enabled);
}

static std::string box_shadow_value(const GBox& box_resource, bool enabled) {
    std::string inner_shadow =
        (box_resource.flags & box_foreground_border_inner)
            ? "inset 0px 0px 0px 1px " +
                  css_color(box_resource.border_inner, enabled)
            : "";
    std::string outer_shadow =
        (box_resource.flags & box_foreground_border_outer)
            ? "0px 0px 0px 1px " + css_color(box_resource.border_outer, enabled)
            : "";

    if (inner_shadow.empty()) {
        return outer_shadow.empty() ? "none" : outer_shadow;
    } else {
        return outer_shadow.empty() ? inner_shadow
                                    : inner_shadow + "," + outer_shadow;
    }
}

static std::string gradient_value(const GBox& box_resource, bool enabled) {
    if (!(box_resource.flags & box_gradient_bg)) {
        return "none";
    }

    Color gradient_bg_start = enabled ? box_resource.main_background
                                      : box_resource.disabled_background;
    return "linear-gradient(to bottom, " +
           css_color(gradient_bg_start, enabled) + ", " +
           css_color(box_resource.gradient_bg_end, enabled) + ")";
}

static std::string border_width(const GBox& box_resource, bool enabled) {
    if (box_resource.border_type != bt_none) {
        // GDraw converts points to pixels with integer rounding. It usually
        // doesn't matter, except for 1pt, which we shall round to 1px for
        // sharpness.
        std::string unit = (box_resource.border_width == 1) ? "px" : "pt";
        return std::to_string(box_resource.border_width) + unit;
    } else {
        return "0";
    }
}

static std::string padding(const GBox& box_resource, bool enabled) {
    // The "padding" property mimics the specifics of GDraw border width
    // application
    return std::to_string(box_resource.padding +
                          box_resource.border_width / 2) +
           "pt";
}

static std::string margin(const GBox& box_resource, bool enabled) {
    // From GBoxExtraSpace()
    return "2pt";
}

static std::string border_style(const GBox& box_resource, bool enabled) {
    static const std::map<enum border_type, std::string> border_type_map = {
        {bt_none, "none"},     {bt_box, "solid"},       {bt_raised, "outset"},
        {bt_lowered, "inset"}, {bt_engraved, "groove"}, {bt_embossed, "ridge"},
        {bt_double, "double"}};
    return border_type_map.at(
        static_cast<enum border_type>(box_resource.border_type));
}

static std::string border_radius(const GBox& box_resource, bool enabled) {
    if (box_resource.border_shape == bs_roundrect) {
        if (box_resource.rr_radius < 1) {
            // Set the radius to arbitrarily huge value, the CSS renderer will
            // decrease it to half-height as necessary
            return "1000pt";
        } else {
            return std::to_string(box_resource.rr_radius) + "pt";
        }
    } else if (box_resource.border_shape == bs_elipse) {
        return "50%";
    }

    return "0";
}

std::map<std::string, std::string> evaluate_css_properties(
    const std::vector<std::pair<std::string, CssPropertyEvalCB>>& property_map,
    const GBox& box_resource, bool enabled) {
    std::map<std::string, std::string> collection;

    for (const auto& [css_property_name, eval] : property_map) {
        std::string css_value = eval(box_resource, enabled);
        if (!css_value.empty()) {
            collection[css_property_name] = css_value;
        }
    }

    return collection;
}

std::map<std::string, std::string> collect_css_properties(
    const GBox& box_resource, const GResFont* font) {
    static const std::vector<std::pair<std::string, CssPropertyEvalCB>>
        css_property_map = {
            {"color", color_property<&GBox::main_foreground>},
            {"background-color", color_property<&GBox::main_background>},
            {"border-left-color", color_property<&GBox::border_brightest>},
            {"border-top-color", color_property<&GBox::border_brighter>},
            {"border-bottom-color", color_property<&GBox::border_darker>},
            {"border-right-color", color_property<&GBox::border_darkest>},
            {"box-shadow", box_shadow_value},
            {"background-image", gradient_value},
            {"border-width", border_width},
            {"padding", padding},
            {"margin", margin},
            {"border-style", border_style},
            {"border-radius", border_radius},
        };

    std::map<std::string, std::string> collection =
        evaluate_css_properties(css_property_map, box_resource, true);

    if (font && font->fi) {
        collection["font-family"] = font->fi->rq.utf8_family_name;
        collection["font-size"] =
            std::to_string(font->fi->rq.point_size) + "pt";
    }

    return collection;
}

std::map<std::string, std::string> collect_css_properties_disabled(
    const GBox& box_resource) {
    static const std::vector<std::pair<std::string, CssPropertyEvalCB>>
        css_property_map = {
            {"color", color_property<&GBox::disabled_foreground>},
            {"background-color", color_property<&GBox::disabled_background>},
            {"border-left-color", color_property<&GBox::border_brightest>},
            {"border-top-color", color_property<&GBox::border_brighter>},
            {"border-bottom-color", color_property<&GBox::border_darker>},
            {"border-right-color", color_property<&GBox::border_darkest>},
            {"box-shadow", box_shadow_value},
            {"background-image", gradient_value},
        };

    return evaluate_css_properties(css_property_map, box_resource, false);
}

std::map<std::string, std::string> collect_css_properties_selected(
    const GBox& box_resource) {
    static const std::vector<std::pair<std::string, CssPropertyEvalCB>>
        css_property_map = {
            {"color", color_property<&GBox::main_foreground>},
            {"background-color", color_property<&GBox::active_border>},
        };

    return evaluate_css_properties(css_property_map, box_resource, true);
}

std::map<std::string, std::string> collect_css_properties_main(
    const struct resed* ri_extras) {
    std::map<std::string, std::string> collection;

    for (const struct resed* rec = ri_extras; rec && rec->resname; ++rec) {
        if (std::strcmp(rec->resname, "Foreground") == 0) {
            collection["color"] = css_color(*(Color*)(rec->val));
        }
        if (std::strcmp(rec->resname, "Background") == 0) {
            collection["background-color"] = css_color(*(Color*)(rec->val));
        }
    }

    return collection;
}

std::map<std::string, std::string> collect_css_properties_color(
    const GBox& box_resource) {
    static const std::vector<std::pair<std::string, CssPropertyEvalCB>>
        css_property_map = {
            {"color", color_property<&GBox::main_foreground>},
            {"background-color", color_property<&GBox::main_background>},
        };

    return evaluate_css_properties(css_property_map, box_resource, true);
}

std::string build_style(const std::string& selector,
                        const std::map<std::string, std::string>& properties) {
    std::string property_list;
    for (auto& [property, value] : properties) {
        property_list += "    " + property + ": " + value + ";\n";
    }

    return selector + " {\n" + property_list + "}\n";
}

std::string build_unset_style(
    const std::string& selector,
    const std::map<std::string, std::string>& properties) {
    std::string property_list;
    for (auto& [property, value] : properties) {
        property_list += "    " + property + ": unset;\n";
    }

    return selector + " {\n" + property_list + "}\n";
}

std::string build_styles(const GResInfo* gdraw_ri) {
    struct Selector {
        std::string node_name;

        // Exclude this selector when appears in the following containers, e.g.
        // "label" should not be modified when appears inside "button".
        std::vector<std::string> excluded_containers;
    };

    static const std::map<std::string, Selector> css_selector_map = {
        {"", {"box", {"tooltip"}}},
        {"GLabel", {"label", {"button", "tooltip", "tab"}}},
        {"GButton", {"button", {"spinbutton"}}},
        {"GDefaultButton", {"button#ok", {}}},
        {"GCancelButton", {"button#cancel", {}}},
        {"GNumericField", {"spinbutton", {}}},
        {"GNumericFieldSpinner", {"spinbutton button", {}}},
        {"GTextField", {"entry", {"spinbutton"}}},
        {"GGadget.Popup", {"tooltip", {}}},
    };

    // Some GTK widgets have substantially different structure from their GDraw
    // analogs. For them we collect only color properties.
    static const std::vector<std::pair<std::string, std::string>>
        css_selector_map_color = {
            {"GList", "treeview"},
            {"GTabSet", "header"},
            {"GTabSet", "tab"},
        };

    std::string styles;

    for (const GResInfo* ri = gdraw_ri; ri->next != NULL; ri = ri->next) {
        for (const auto& [resname, css_node] : css_selector_map_color) {
            if (resname == ri->resname) {
                auto props_color = collect_css_properties_color(*(ri->boxdata));
                styles += build_style(css_node, props_color);

                auto props_selected =
                    collect_css_properties_selected(*(ri->boxdata));
                styles += build_style(css_node + ":selected", props_selected);
                styles += build_style(css_node + ":checked", props_selected);
            }
        }
    }

    for (const GResInfo* ri = gdraw_ri; ri->next != NULL; ri = ri->next) {
        auto sel_it = css_selector_map.find(ri->resname);
        if (sel_it == css_selector_map.end()) {
            continue;
        }
        const Selector& selector = sel_it->second;

        if ((std::strcmp(ri->resname, "") == 0 ||
             std::strcmp(ri->resname, "GGadget.Popup") == 0) &&
            ri->extras) {
            // Collect some default properties from the base class.
            auto props_main = collect_css_properties_main(ri->extras);
            styles += build_style(selector.node_name, props_main);

            // When the node is inside predefined containers, we shall unset the
            // affected properties
            for (const std::string& container : selector.excluded_containers) {
                styles += build_unset_style(
                    container + " " + selector.node_name, props_main);
            }

            continue;
        }

        if (!ri->boxdata) {
            continue;
        }

        auto props = collect_css_properties(*(ri->boxdata), ri->font);
        styles += build_style(selector.node_name, props);

        auto props_disabled = collect_css_properties_disabled(*(ri->boxdata));
        styles += build_style(selector.node_name + ":disabled", props_disabled);

        // When the node is inside predefined containers, we shall unset the
        // affected properties
        for (const std::string& container : selector.excluded_containers) {
            styles +=
                build_unset_style(container + " " + selector.node_name, props);
            styles += build_unset_style(
                container + " " + selector.node_name + ":disabled",
                props_disabled);
        }
    }

    styles += "tab { margin-bottom: 1px; }\n";

    return styles;
}

}  // namespace ff::app

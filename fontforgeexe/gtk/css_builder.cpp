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

#include <map>
#include <vector>

static std::string css_color(Color col, bool enabled = true) {
    // There is no convenient hex formatting in C++17...
    char value_string[250] = "";

    // Build semitransparent color expression for disabled elements
    const char* format = enabled ? "#%06x" : "alpha(#%06x, 0.5)";

    sprintf(value_string, format, col);
    return value_string;
}

static std::string box_shadow_value(const GBox& box_resource, bool enabled) {
    return "0px 0px 0px 1px " + css_color(box_resource.border_outer, enabled) +
           ", inset 0px 0px 0px 1px " +
           css_color(box_resource.border_inner, enabled);
}

static std::string gradient_value(const GBox& box_resource, bool enabled) {
    Color gradient_bg_start = enabled ? box_resource.main_background
                                      : box_resource.disabled_background;
    return "linear-gradient(to bottom, " +
           css_color(gradient_bg_start, enabled) + ", " +
           css_color(box_resource.gradient_bg_end, enabled) + ")";
}

std::map<std::string, std::string> collect_css_properties(
    const GBox& box_resource) {
    static const std::vector<std::pair<Color GBox::*, std::string>>
        css_property_map = {
            {&GBox::main_foreground, "color"},
            {&GBox::main_background, "background-color"},
            {&GBox::border_brightest, "border-left-color"},
            {&GBox::border_brighter, "border-top-color"},
            {&GBox::border_darker, "border-bottom-color"},
            {&GBox::border_darkest, "border-right-color"},
        };

    std::map<std::string, std::string> collection;

    for (const auto& [gbox_field, css_property_name] : css_property_map) {
        collection[css_property_name] = css_color(box_resource.*gbox_field);
    }
    collection["box-shadow"] = box_shadow_value(box_resource, true);
    collection["background-image"] = gradient_value(box_resource, true);

    if (box_resource.border_type != bt_none) {
        collection["border-width"] =
            std::to_string(box_resource.border_width) + "pt";
    }

    if (box_resource.border_type == bt_box) {
        collection["border-style"] = "solid";
    }

    if (box_resource.border_shape == bs_roundrect) {
        if (box_resource.rr_radius < 1) {
            // Set the radius to arbitrarily huge value, the CSS renderer will
            // decrease it to half-height as necessary
            collection["border-radius"] = "1000pt";
        } else {
            collection["border-radius"] =
                std::to_string(box_resource.rr_radius) + "pt";
        }
    } else if (box_resource.border_shape == bs_elipse) {
        collection["border-radius"] = "50%";
    }

    return collection;
}

std::map<std::string, std::string> collect_css_properties_disabled(
    const GBox& box_resource) {
    static const std::vector<std::pair<Color GBox::*, std::string>>
        css_property_map = {
            {&GBox::disabled_foreground, "color"},
            {&GBox::disabled_background, "background-color"},
        };
    static const std::vector<std::pair<Color GBox::*, std::string>>
        css_border_property_map = {
            {&GBox::border_brightest, "border-left-color"},
            {&GBox::border_brighter, "border-top-color"},
            {&GBox::border_darker, "border-bottom-color"},
            {&GBox::border_darkest, "border-right-color"},
        };

    std::map<std::string, std::string> collection;

    for (const auto& [gbox_field, css_property_name] : css_property_map) {
        collection[css_property_name] =
            css_color(box_resource.*gbox_field, false);
    }
    for (const auto& [gbox_field, css_property_name] :
         css_border_property_map) {
        collection[css_property_name] =
            css_color(box_resource.*gbox_field, false);
    }
    collection["box-shadow"] = box_shadow_value(box_resource, false);
    collection["background-image"] = gradient_value(box_resource, false);

    return collection;
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
        {"GLabel", {"label", {"button"}}}};

    std::string styles;

    for (const GResInfo* ri = gdraw_ri; ri->next != NULL; ri = ri->next) {
        if (!ri->boxdata || !css_selector_map.count(ri->resname)) {
            continue;
        }

        const Selector& selector = css_selector_map.at(ri->resname);

        auto props = collect_css_properties(*(ri->boxdata));
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

    return styles;
}

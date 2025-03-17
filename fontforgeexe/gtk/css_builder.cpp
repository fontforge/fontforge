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

static std::string css_color(Color col) {
    // There is no convenient hex formatting in C++17...
    char value_string[250] = "";
    sprintf(value_string, "#%06x", col);
    return value_string;
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
        collection[css_property_name] = css_color(box_resource.*gbox_field);
    }
    for (const auto& [gbox_field, css_property_name] :
         css_border_property_map) {
        // Make semitransparent border for disabled elements
        collection[css_property_name] =
            "alpha(" + css_color(box_resource.*gbox_field) + ", 0.5)";
    }

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

std::string build_styles(const GResInfo* gdraw_ri) {
    static const std::map<std::string, std::string> css_selector_map = {
        {"GLabel", "label"}};

    std::string styles;

    for (const GResInfo* ri = gdraw_ri; ri->next != NULL; ri = ri->next) {
        if (!ri->boxdata || !css_selector_map.count(ri->resname)) {
            continue;
        }

        auto props = collect_css_properties(*(ri->boxdata));
        styles += build_style(css_selector_map.at(ri->resname), props);

        auto props_disabled = collect_css_properties_disabled(*(ri->boxdata));
        styles += build_style(css_selector_map.at(ri->resname) + ":disabled",
                              props_disabled);
    }

    return styles;
}

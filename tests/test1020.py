#Test getter and setter of font.style_set_names
import fontforge

font = fontforge.font()
assert font.style_set_names == ()

for style_set_names in [
    (("English (US)", "ss01", "My english ss01"), ("English (US)", "ss02", "My english ss02")), # same language, more
    (("English (British)", "ss01", "My english ss01"), ("French Côte d'Ivoire", "ss01", "Mon ss01 français")), # same style set, non-ascii
    (("English (British)", "ss09", "My english ss09"),), # fewer
    ((0x809, "ss09", "My english ss09"),), # language as int
    (("Chinese (PRC)", "ss10", "我的中文ss10"),), # Unicode
    (("English (British)", "ss20", "😀𝄞"),), # Unicode
    ()
]:
    font.style_set_names = style_set_names
    style_set_names_with_str_lang = []
    for name_spec in style_set_names:
        if name_spec[0] == 0x809:
            style_set_names_with_str_lang.append(("English (British)",) + name_spec[1:])
        else:
            style_set_names_with_str_lang.append(name_spec)
    assert font.style_set_names == tuple(style_set_names_with_str_lang)


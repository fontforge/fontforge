#Test getter and setter of font.style_set_names
import fontforge

font = fontforge.font()
assert font.style_set_names == ()

for style_set_names in [
    (("English (US)", "ss01", "My english ss01"), ("English (US)", "ss02", "My english ss02")), # same language, more
    (("English (British)", "ss01", "My english ss01"), ("French Côte d'Ivoire", "ss01", "Mon ss01 français")), # same style set, non-ascii
    (("English (British)", "ss09", "My english ss09"),), # fewer
    (("Chinese (PRC)", "ss10", "我的中文ss10"),), # Unicode
    (("English (British)", "ss20", "😀𝄞"),), # Unicode
    ()
]:
    font.style_set_names = style_set_names
    assert font.style_set_names == style_set_names


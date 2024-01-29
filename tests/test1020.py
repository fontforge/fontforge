#Test getter and setter of font.style_set_names including errors
import fontforge

font = fontforge.font()
assert font.style_set_names == ()

def lang_to_lang_str(style_set_names):
    style_set_names_with_str_lang = []
    for name_spec in style_set_names:
        if name_spec[0] == 0x409:
            style_set_names_with_str_lang.append(("English (US)",) + name_spec[1:])
        elif name_spec[0] == 0x809:
            style_set_names_with_str_lang.append(("English (British)",) + name_spec[1:])
        else:
            style_set_names_with_str_lang.append(name_spec)
    return style_set_names_with_str_lang

for style_set_names in [
    (("English (US)", "ss01", "My English ss01"), ("English (US)", "ss02", "My English ss02")), # same language, more
    (("English (British)", "ss01", "My English ss01"), ("French Côte d'Ivoire", "ss01", "Mon ss01 français")), # same style set, non-ascii
    (("English (British)", "ss09", "My English ss09"),), # fewer
    ((0x809, "ss09", "My English ss09"),), # language as int
    (("Chinese (PRC)", "ss10", "我的中文ss10"),), # Unicode
    (("English (British)", "ss20", "😀𝄞"),), # Unicode
    (("English (British)", "ss20", "My English ss20 with a different name"),),
    ((0x809, "ss20", "My English ss20 with another different name"),),
    (("English (British)", "ss20", "My English ss20 with another Name"),),
    ()
]:
    font.style_set_names = style_set_names
    style_set_names_with_str_lang = lang_to_lang_str(style_set_names)
    assert font.style_set_names == tuple(style_set_names_with_str_lang)

# tests for error

for style_set_names, e_type, e_str in [
    ("test", TypeError, "Style set names must be set as a tuple."),
    (1.23, TypeError, "Style set names must be set as a tuple."),
    (("test",), TypeError, "Style set name specification must be a tuple."),
    ((1.23,), TypeError, "Style set name specification must be a tuple."),
    ((("English", "My name"),), ValueError, "Style set name specification must have 3 elements (language, style set and name)."),
    ((("English (US)", "ss01", "My name"), ("English", "My name")), ValueError, "Style set name specification must have 3 elements (language, style set and name)."),
    ((("English", "ss01", "My name"),), ValueError, "The first element of a style set name specification must be a valid language name if it is a string."),
    (((0, "ss01", "My name"),), ValueError, "The first element of a style set name specification must be a valid language id if it is an integer."),
    ((("English (US)", 1, "My name"),), TypeError, "The second element of a style set name specification must be a string."),
    ((("English (US)", "01", "My name"),), ValueError, "The second element of a style set name specification must be a valid style set tag."),
    ((("English (US)", "ss00", "My name"),), ValueError, "The second element of a style set name specification must be a valid style set tag."),
    ((("English (US)", "ss21", "My name"),), ValueError, "The second element of a style set name specification must be a valid style set tag."),
    ((("English (US)", "ss30", "My name"),), ValueError, "The second element of a style set name specification must be a valid style set tag."),
    ((("English (US)", "ss001", "My name"),), ValueError, "The second element of a style set name specification must be a valid style set tag."),
    ((("English (US)", "ss0D", "My name"),), ValueError, "The second element of a style set name specification must be a valid style set tag."),
    ((("English (US)", "ssOI", "My name"),), ValueError, "The second element of a style set name specification must be a valid style set tag."),
    ((("English (US)", "ss01", 123),), TypeError, "The third element of a style set name specification must be a string."),
    ((("English (US)", "ss01", "My name1"), ("English (US)", "ss01", "My name2")), ValueError, "The feature ss01 is named twice in language English (US):\n  My name1\n  My name2"),
    ((("English (US)", "ss01", "My name1"), (0x409, "ss01", "My name2")), ValueError, "The feature ss01 is named twice in language English (US):\n  My name1\n  My name2"),
    ((("English (US)", "ss01", "My name"), ("English (US)", "ss01", "My name")), ValueError, "The feature ss01 is named twice in language English (US):\n  My name\n  My name")
]:
    try:
        font = fontforge.font()
        font.style_set_names = style_set_names
        style_set_names_with_str_lang = lang_to_lang_str(style_set_names)
        raise AssertionError("no error raised, but expected")
    except e_type as e:
        assert str(e) == e_str, f"wrong error message for correct error type"
    except Exception:
        raise AssertionError("wrong error type")

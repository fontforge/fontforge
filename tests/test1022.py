# Test importOutline for Python file stream

import fontforge
from io import BytesIO
import os
import unittest


def export_outlines_value(output_path, character):
    character.export(output_path)
    file_content: str = ""
    with open(output_path, 'r') as file:
        file_content = file.read()
    os.remove(output_path)

    # fontforge modifies the glyphs, so all you can check is if the file has content
    assert "svg" in file_content
    assert "path" in file_content
    assert "d" in file_content

# NOTE: I just dumped test files into my '/tmp' directory


def generic_file_stream_testing(file_type: str, character):
    IMAGE = '/tmp/test.' + file_type
    OUT = '/tmp/img.' + file_type
    file_bytes = None
    with open(IMAGE, "rb") as file:
        file_bytes = BytesIO(file.read())
    character.importOutlines(file_bytes, type=file_type)
    character.export(OUT)
    with open(OUT, "rb") as file:
        assert len(file.read()) > 0
    os.remove(OUT)


class TestFontForgeImportOutlines(unittest.TestCase):
    def setUp(self):
        self.font = fontforge.font()
        self.char = self.font.createChar(65)
        self.svg_string = """
        <svg height="210" width="400" xmlns="http://www.w3.org/2000/svg">
          <path d="M140 5 L79 200 L225 150 Z"
          fill="none" stroke="black" stroke-width="3" />
        </svg>
        """
        self.INPUT = '/tmp/file.svg'
        self.OUTPUT = '/tmp/output.svg'

    # This is the traditional way of importing outlines
    def test_import_outlines_from_file(self):
        with open(self.INPUT, 'w') as file:
            file.write(self.svg_string)
        self.char.importOutlines(self.INPUT)
        os.remove(self.INPUT)

        export_outlines_value(self.OUTPUT, self.char)

    def test_import_outlines_from_file_filename_keyword(self):
        with open(self.INPUT, 'w') as file:
            file.write(self.svg_string)
        self.char.importOutlines(filename=self.INPUT)
        os.remove(self.INPUT)

        export_outlines_value(self.OUTPUT, self.char)

    # As a side effect of my changes even if you want to load a file
    # , but that file does not have a file extension now can be imported
    def test_import_outlines_from_file_without_ext(self):
        file_without_ext = '/tmp/file_without_ext'
        with open(file_without_ext, 'w') as file:
            file.write(self.svg_string)
        self.char.importOutlines(file_without_ext, type='svg')
        os.remove(file_without_ext)

        export_outlines_value(self.OUTPUT, self.char)

    def test_import_outlines_from_file_without_ext_missing_type(self):
        with self.assertRaises(OSError):
            file_without_ext = '/tmp/file_without_ext'
            with open(file_without_ext, 'w') as file:
                file.write(self.svg_string)
            self.char.importOutlines(file_without_ext)
            os.remove(file_without_ext)

            export_outlines_value(self.OUTPUT, self.char)

    # This is the Happy Path
    def test_import_outlines_from_stream_with_type(self):
        self.char.importOutlines(
            BytesIO(bytes(self.svg_string, 'utf-8')), type='svg')

        export_outlines_value(self.OUTPUT, self.char)

    def test_import_outlines_missing_type(self):
        with self.assertRaises(TypeError):
            self.char.importOutlines(BytesIO(bytes(self.svg_string, 'utf-8')))

    def test_import_png_outlines(self):
        PNG_IMAGE = '../doc/sphinx/images/allgreek.png'
        OUT_PNG = '/tmp/img.png'
        self.char.importOutlines(PNG_IMAGE)
        self.char.export(OUT_PNG)
        with open(OUT_PNG, "rb") as file:
            assert len(file.read()) > 0
        os.remove(OUT_PNG)

    def test_import_png_outlines_from_stream_with_type(self):
        PNG_IMAGE = '../doc/sphinx/images/a_dieresis_macron.png'
        OUT_PNG = '/tmp/img.png'
        png_bytes = None
        with open(PNG_IMAGE, "rb") as file:
            png_bytes = BytesIO(file.read())
        self.char.importOutlines(png_bytes, type='png')
        self.char.export(OUT_PNG)
        with open(OUT_PNG, "rb") as file:
            assert len(file.read()) > 0
        os.remove(OUT_PNG)

    def test_import_jpeg_outlines_from_stream_with_type(self):
        try:
            generic_file_stream_testing('jpeg', self.char)
        except TypeError as e:
            # Because you cannot export to jpeg
            assert str(e) == "Unknown extension to export: .jpeg"

    def test_import_eps_outlines_from_stream_with_type(self):
        generic_file_stream_testing('eps', self.char)

    def test_import_glif_outlines_from_stream_with_type(self):
        generic_file_stream_testing('glif', self.char)

    def test_import_tiff_outlines_from_stream_with_type(self):
        with self.assertRaises(IOError):
            generic_file_stream_testing('tiff', self.char)

    def test_import_gif_outlines_from_stream_with_type(self):
        with self.assertRaises(IOError):
            generic_file_stream_testing('gif', self.char)

    def test_import_xbm_outlines_from_stream_with_type(self):
        generic_file_stream_testing('xbm', self.char)

    def test_import_xpm_outlines_from_stream_with_type(self):
        generic_file_stream_testing('xpm', self.char)

if __name__ == '__main__':
    unittest.main()

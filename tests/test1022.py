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
        self.char.importOutlines(BytesIO(bytes(self.svg_string, 'utf-8')), type='svg')

        export_outlines_value(self.OUTPUT, self.char)

    def test_import_outlines_missing_type(self):
        with self.assertRaises(TypeError):
            self.char.importOutlines(BytesIO(bytes(self.svg_string, 'utf-8')))

if __name__ == '__main__':
    unittest.main()

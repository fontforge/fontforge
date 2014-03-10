"""
    Copyright (C) 2013 Mayank Jha <mayank25080562@gmail.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""
#!/usr/bin/env python
from distutils.core import setup
from setuptools import setup 
setup(name='Font Compare',
      version='1.0',
      description='FontForge based utility for aesthetic testing  of fonts',
      author='Mayank Jha',
      author_email='mayank25080562@gmail.com',
      url='https://github.com/mjnovice/FontCompare',
      include_package_data=True,
      packages=['fc'],
      package_data={'fc/data': ['data/*.mcy','data/*.bmp','data/*.png'],
      'fc/docs': ['docs/*.txt']},
      scripts = ['fontcompare'],
      install_requires=[
      "Pillow >=2.0",
      ],
    )

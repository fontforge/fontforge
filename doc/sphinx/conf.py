# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

import sys, os

sys.path.append(os.path.abspath('_extensions'))

# -- Project information -----------------------------------------------------

project = 'FontForge'
copyright = '2000-2012 by George Williams, 2012-2020 by FontForge authors'
author = 'FontForge authors'

# The full version, including alpha/beta/rc tags
release = '20230101'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.githubpages',
    'sphinx.ext.mathjax',

    'flex_grid',
    'fontforge_scripting_domain'
]

# interesting to see if it can be adapted: docutils.utils.math.math2html

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'alabaster'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Don't make images clickable to the original if they're scaled
html_scaled_image_link = False

# Set the favicon
html_favicon = 'images/fftype16.png'

# Include the old Japanese/German docs
html_extra_path = ['olddocs']

# Don't copy source rst files into the output
html_copy_source = False

# Set the main page
master_doc = 'index'

# Custom roles must be in the prolog, not the epilog!
rst_prolog = '''
.. role:: small
'''

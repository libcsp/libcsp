# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Imports -----------------------------------------------------------------
from pathlib import Path
import clang.cindex as cl
import pygit2

# -- Path setup --------------------------------------------------------------
cl.Config.set_library_file('/usr/lib/llvm-14/lib/libclang-14.so')
cl.Config.set_library_path('/usr/lib/llvm-14/lib/libclang-14.so')

# -- Project information -----------------------------------------------------
project = 'Lib CSP'
#copyright = ''
#author = ''

# -- Constants ---------------------------------------------------------------

# -- General configuration ---------------------------------------------------
# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx_rtd_theme',
    'myst_parser',
    'sphinx_c_autodoc',
    'sphinx_c_autodoc.viewcode',
    "sphinx_design",
    "sphinx_git",
    "sphinx_copybutton"
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# -- API C files -------------------------------------------------------------
c_autodoc_roots = ['../include/csp']
c_autodoc_compilation_args = ["-I./../include/"]

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

autosectionlabel_prefix_document = True
html_theme_options = {
    'logo_only': False,
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': True,
    'vcs_pageview_mode': '',
    'style_nav_header_background': 'rgba(52,49,49,1) 100%;',
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 2,
    'includehidden': True,
    'titles_only': True,
    'sticky_navigation': True
}

def include_readme_file(app, docname, source):
    """
        This hook reads the contents of the README.md file, replaces the
        link for `git-commit` and inserts the modified contents in the index.md
        file before the first occuarance of  ```{toctree}
    """
    if docname == 'index':
        # Read and modify the contents of README
        readme = Path(app.srcdir) / ".." / "README.md"
        print(readme)
        with readme.open("r") as file:
            readme_contents = file.read()

        # Here we change the link for the `git-commit` page
        readme_contents = readme_contents.replace("](./doc/", "](")

        # Find the index of the first occurrence of ```{toctree}
        toctree_index = source[0].find('```{toctree}')
        if toctree_index != -1:
            # Insert the modified README files
            source[0] = source[0][:toctree_index] + readme_contents + source[0][toctree_index:]

def setup(app):
    app.connect('source-read', include_readme_file)

version = pygit2.Repository('.').head.shorthand

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_show_sourcelink = False
html_title = "Cubesat Space Protocol"
#html_static_path = ['_static']
#html_logo = "_static/logo.png"

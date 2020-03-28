# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
#import os
#import sys
#p=os.path.abspath(os.path.join('..','..','tesicjtag'))
##p=os.path.abspath(os.path.join('..','..'))
#print(p)
#assert(os.path.exists(p))
#sys.path.insert(0, p)
#
#
## -- Project information -----------------------------------------------------
#
#project = 'tesicjtag'
#copyright = '2020, Sebastien Riou'
#author = 'Sebastien Riou'


import os
import re
import sys

# pip3 install sphinx-autodoc-typehints sphinx-pypi-upload
# python3 setup.py build_sphinx
# sphinx-build -b html ../pytesicjtag/tesicjtag/docs .

topdir = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                      os.pardir, os.pardir))
assert(os.path.exists(topdir))
sys.path.append(topdir)


def read(where, *parts):
    """
    Build an absolute path from *parts* and and return the contents of the
    resulting file.  Assume UTF-8 encoding.
    """
    with open(os.path.join(where, *parts), 'rt') as f:
        return f.read()


def find_meta(meta):
    """
    Extract __*meta*__ from meta_file.
    """
    meta_match = re.search(
        r"^__{meta}__ = ['\"]([^'\"]*)['\"]".format(meta=meta),
        meta_file, re.M
    )
    if meta_match:
        return meta_match.group(1)
    raise RuntimeError("Unable to find __{meta}__ string.".format(meta=meta))


meta_file = read(topdir, 'pysatl', '__init__.py')

version = find_meta('version')

needs_sphinx = '2.1'
extensions = ['sphinx.ext.autodoc',
              'sphinx.ext.doctest',
              'sphinx_autodoc_typehints',
              'sphinx.ext.napoleon']
templates_path = ['templates']
source_suffix = '.rst'
master_doc = 'index'
project = find_meta('title')
contact = '%s <%s>' % (find_meta('author'), find_meta('email'))
copyright = find_meta('copyright')
show_authors = True

html_theme = 'sphinx_rtd_theme'
htmlhelp_basename = 'doc'

preamble = r'''
\usepackage{wallpaper}
\usepackage{titlesec}

\titleformat{\chapter}[display]{}{\filleft\scshape\chaptername\enspace\thechapter}{-2pt}{\filright \Huge \bfseries}[\vskip4.5pt\titlerule]
\titleformat{name=\chapter, numberless}[block]{}{}{0pt}{\filright \Huge \bfseries}[\vskip4.5pt\titlerule]

\titlespacing{\chapter}{0pt}{0pt}{1cm}
'''

latex_elements = {
  'papersize': 'a4paper',
  'fncychap': '',  # No Title Page
  'releasename': '',
  'sphinxsetup': 'hmargin={2.0cm,2.0cm}, vmargin={2.5cm,2.5cm}, marginpar=5cm',
  'classoptions': ',openany,oneside',  # Avoid blank page aftre TOC, etc.
  'preamble': preamble,
  'releasename': ''
}

latex_documents = [
  ('index', '%s.tex' % project.lower(),
   '%s Documentation' % project,
   contact, u'manual'),
]

# For "manual" documents, if this is true, then toplevel headings are parts,
# not chapters.
latex_toplevel_sectioning = "chapter"

man_pages = [
  ('index', project,
   '%s Documentation' % project,
   [contact], 1)
]

texinfo_documents = [
  ('index', project,
   '%s Documentation' % project,
   contact, '',
   '%s Documentation' % project,
   'Miscellaneous'),
]

on_rtd = os.environ.get('READTHEDOCS', None) == 'True'
import shutil
if not on_rtd:  # only import and set the theme if we're building docs locally
  import sphinx_rtd_theme
  html_theme = 'sphinx_rtd_theme'
  html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
  #html_style = 'css/theme_overrides.css'
  #shutil.makedirs()
  print(os.getcwd())
  os.makedirs('../build/html/_static/css/',exist_ok=True)
  shutil.copyfile('theme_overrides.css', '../build/html/_static/css/theme_overrides.css')
else:
  html_context = {
    'css_files': [
        'https://media.readthedocs.org/css/sphinx_rtd_theme.css',
        'https://media.readthedocs.org/css/readthedocs-doc-embed.css',
        '_static/css/theme_overrides.css',
    ],
  }


def setup(app):
    app.add_stylesheet('https://fonts.googleapis.com/css?family=Raleway')
    app.add_stylesheet('css/theme_overrides.css')

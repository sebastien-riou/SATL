pysatl's documentation
#########################

``pysatl`` is a package to communicate using `SATL <https://github.com/sebastien-riou/SATL>`__ protocol.

SATL stands for 'Simple APDU Transport Layer'. It is a simple way to exchange ISO7816-4 APDUs over interfaces not covered in ISO7816-3.

Other pages (online)

- `project page on GitHub`_
- `Download Page`_ with releases
- This page, when viewed online is at https://satl.readthedocs.io/en/latest/

.. _`project page on GitHub`: https://github.com/sebastien-riou/SATL/tree/master/implementations/python3
.. _`Download Page`: http://pypi.python.org/pypi/pysatl


Installation
************
This installs a package that can be used from Python (``import pysatl``).

From PyPI
=========
To install for the current user:

.. code-block:: python

    python3 -m pip install --user pysatl

To install for all users on the system, administrator rights (root)
may be required.

.. code-block:: python

    python3 -m pip install pysatl

Classes
*******

PySatl
======

.. autoclass :: pysatl.PySatl
    :members:

CAPDU
=====

.. autoclass :: pysatl.CAPDU
    :members:


RAPDU
=====

.. autoclass :: pysatl.RAPDU
    :members:

Utils
=====

.. autoclass :: pysatl.Utils
    :members:
    :undoc-members:

SocketComDriver
===============

.. autoclass :: pysatl.SocketComDriver
    :members:
    :undoc-members:

StreamComDriver
===============

.. autoclass :: pysatl.StreamComDriver
    :members:
    :undoc-members:
.. toctree::

Indices and tables
##################

* :ref:`genindex`
* :ref:`search`

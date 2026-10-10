API Reference: Structured Data
==============================

This is the API reference for the functions, classes, and enums in the ``renderccccc`` module which represents the underlying interface that the UI is built on top of. For more high-level information and instructions on using the python API, see :doc:`../index`.

.. contents:: Sections
   :local:

.. currentmodule:: renderccccc

Type information
----------------

.. autoclass:: SDType
  :members:

.. autoclass:: SDBasic
  :members:

.. autoclass:: SDTypeFlags
  :members:

Objects
-------

.. autoclass:: SDObject
  :members:

.. autoclass:: SDObjectData
  :members:

.. autoclass:: SDObjectPODData
  :members:

Chunks
------

.. autoclass:: SDChunk
  :members:

.. autoclass:: SDChunkMetaData
  :members:

.. autoclass:: SDChunkFlags
  :members:

Structured File
---------------

.. autoclass:: SDFile
  :members:

Creation Helper Functions
-------------------------

.. autofunction:: renderccccc.makeSDArray
.. autofunction:: renderccccc.makeSDBool
.. autofunction:: renderccccc.makeSDEnum
.. autofunction:: renderccccc.makeSDFloat
.. autofunction:: renderccccc.makeSDInt32
.. autofunction:: renderccccc.makeSDInt64
.. autofunction:: renderccccc.makeSDResourceId
.. autofunction:: renderccccc.makeSDString
.. autofunction:: renderccccc.makeSDStruct
.. autofunction:: renderccccc.makeSDUInt32
.. autofunction:: renderccccc.makeSDUInt64

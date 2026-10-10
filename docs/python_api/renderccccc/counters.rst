API Reference: Performance Counters
===================================

This is the API reference for the functions, classes, and enums in the ``renderccccc`` module which represents the underlying interface that the UI is built on top of. For more high-level information and instructions on using the python API, see :doc:`../index`.

.. contents:: Sections
   :local:

.. currentmodule:: renderccccc

Counters
--------

.. autoclass:: renderccccc.CounterDescription
  :members:

.. autoclass:: renderccccc.CounterUnit
  :members:

.. autoclass:: renderccccc.Uuid
  :members:

Counter Types
-------------

.. autoclass:: renderccccc.GPUCounter
  :members:

.. autofunction:: renderccccc.IsAMDCounter
.. autofunction:: renderccccc.IsARMCounter
.. autofunction:: renderccccc.IsGenericCounter
.. autofunction:: renderccccc.IsIntelCounter
.. autofunction:: renderccccc.IsNvidiaCounter
.. autofunction:: renderccccc.IsVulkanExtendedCounter

Results
-------

.. autoclass:: renderccccc.CounterResult
  :members:

.. autoclass:: renderccccc.CounterValue
  :members:

API Reference: Utilities
========================

This is the API reference for the functions, classes, and enums in the ``renderccccc`` module which represents the underlying interface that the UI is built on top of. For more high-level information and instructions on using the python API, see :doc:`../index`.

.. contents:: Sections
   :local:

.. currentmodule:: renderccccc

Maths
-----

.. autoclass:: FloatVector
  :members:

.. autofunction:: renderccccc.HalfToFloat
.. autofunction:: renderccccc.FloatToHalf

Logging & Versioning
--------------------

.. autofunction:: renderccccc.LogMessage
.. autofunction:: renderccccc.SetDebugLogFile
.. autofunction:: renderccccc.GetLogFile
.. autofunction:: renderccccc.GetCurrentProcessMemoryUsage
.. autofunction:: renderccccc.DumpObject

.. autoclass:: LogType
  :members:


Versioning
----------

.. autofunction:: renderccccc.GetVersionString
.. autofunction:: renderccccc.GetCommitHash
.. autofunction:: renderccccc.IsReleaseBuild

Settings
--------

.. autofunction:: renderccccc.GetConfigSetting
.. autofunction:: renderccccc.SetConfigSetting
.. autofunction:: renderccccc.SaveConfigSettings

Self-hosted captures
--------------------

.. autofunction:: renderccccc.CanSelfHostedCapture
.. autofunction:: renderccccc.StartSelfHostCapture
.. autofunction:: renderccccc.EndSelfHostCapture

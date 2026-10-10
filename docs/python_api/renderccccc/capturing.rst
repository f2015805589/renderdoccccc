API Reference: Capturing
========================

This is the API reference for the functions, classes, and enums in the ``renderccccc`` module which represents the underlying interface that the UI is built on top of. For more high-level information and instructions on using the python API, see :doc:`../index`.

.. contents:: Sections
   :local:

.. currentmodule:: renderccccc

Execution & Injection
---------------------

.. autofunction:: renderccccc.ExecuteAndInject
.. autofunction:: renderccccc.InjectIntoProcess

.. autoclass:: renderccccc.CaptureOptions
  :members:
  
.. autoclass:: renderccccc.EnvironmentModification
  :members:

.. autoclass:: renderccccc.EnvMod
  :members:

.. autoclass:: renderccccc.EnvSep
  :members:

.. autoclass:: renderccccc.ExecuteResult
  :members:

Global Hooking
--------------

.. autofunction:: renderccccc.StartGlobalHook
.. autofunction:: renderccccc.StopGlobalHook
.. autofunction:: renderccccc.IsGlobalHookActive
.. autofunction:: renderccccc.CanGlobalHook

Target Control
--------------

.. autofunction:: renderccccc.EnumerateRemoteTargets
.. autofunction:: renderccccc.CreateTargetControl

.. autoclass:: renderccccc.TargetControl
  :members:

.. autoclass:: renderccccc.TargetControlMessage
  :members:

.. autoclass:: renderccccc.TargetControlMessageType
  :members:

.. autoclass:: renderccccc.NewCaptureData
  :members:

.. autoclass:: renderccccc.APIUseData
  :members:

.. autoclass:: renderccccc.BusyData
  :members:

.. autoclass:: renderccccc.NewChildData
  :members:


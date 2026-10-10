API Reference: Replay Control
=============================

This is the API reference for the functions, classes, and enums in the ``renderccccc`` module which represents the underlying interface that the UI is built on top of. For more high-level information and instructions on using the python API, see :doc:`../index`.

.. contents:: Sections
   :local:

.. currentmodule:: renderccccc

Initialisation and Shutdown
---------------------------

.. autofunction:: renderccccc.InitialiseReplay
.. autofunction:: renderccccc.ShutdownReplay

.. autoclass:: renderccccc.GlobalEnvironment
  :members:

.. autoclass:: renderccccc.ResultCode
  :members:

.. autoclass:: renderccccc.ResultDetails
  :members:

Capture File Access
-------------------

.. autofunction:: renderccccc.OpenCaptureFile

.. autoclass:: renderccccc.CaptureAccess
  :members:

.. autoclass:: renderccccc.CaptureFile
  :members:

.. autoclass:: renderccccc.ReplaySupport
  :members:

.. autoclass:: renderccccc.CaptureFileFormat
  :members:

.. autoclass:: renderccccc.SectionProperties
  :members:

.. autoclass:: renderccccc.SectionType
  :members:

.. autoclass:: renderccccc.SectionFlags
  :members:

.. autoclass:: renderccccc.Thumbnail
  :members:

GPU Enumeration
---------------

.. autoclass:: renderccccc.GPUDevice
  :members:

.. autoclass:: renderccccc.GPUVendor
  :members:

.. autofunction:: renderccccc.GPUVendorFromPCIVendor

.. autoclass:: renderccccc.GraphicsAPI
  :members:

.. autofunction:: renderccccc.IsD3D

.. autofunction:: renderccccc.GetDriverInformation

.. autoclass:: renderccccc.DriverInformation
  :members:

Replay Controller
-----------------

.. autoclass:: renderccccc.ReplayController
  :members:

.. autoclass:: renderccccc.ReplayOptions
  :members:

.. autoclass:: renderccccc.ReplayOptimisationLevel
  :members:

.. autoclass:: renderccccc.APIProperties
  :members:

Device Protocols
----------------

.. autoclass:: renderccccc.DeviceProtocolController
  :members:

.. autofunction:: renderccccc.GetSupportedDeviceProtocols
.. autofunction:: renderccccc.GetDeviceProtocolController

Remote Servers
--------------

.. autoclass:: renderccccc.RemoteServer
  :members:

.. autofunction:: renderccccc.CreateRemoteServerConnection
.. autofunction:: renderccccc.CheckRemoteServerConnection
.. autofunction:: renderccccc.BecomeRemoteServer

.. autoclass:: renderccccc.PathEntry
  :members:

.. autoclass:: renderccccc.PathProperty
  :members:

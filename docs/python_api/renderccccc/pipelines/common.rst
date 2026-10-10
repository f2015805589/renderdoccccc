API Reference: Common Pipeline State Abstraction
================================================

This is the API reference for the functions, classes, and enums in the ``renderccccc`` module which represents the underlying interface that the UI is built on top of. For more high-level information and instructions on using the python API, see :doc:`../../index`.

.. contents:: Sections
   :local:

.. currentmodule:: renderccccc

.. autoclass:: PipeState
  :members:

General
-------

.. autoclass:: renderccccc.Offset
  :members:

Vertex Inputs
-------------

.. autoclass:: renderccccc.BoundVBuffer
  :members:

.. autoclass:: renderccccc.VertexInputAttribute
  :members:

.. autoclass:: renderccccc.Topology
  :members:
  
.. autofunction:: renderccccc.NumVerticesPerPrimitive
.. autofunction:: renderccccc.VertexOffset
.. autofunction:: renderccccc.PatchList_Count
.. autofunction:: renderccccc.PatchList_Topology
.. autofunction:: renderccccc.IsStrip

Shader Resource Bindings
------------------------

.. autoclass:: renderccccc.UsedDescriptor
  :members:

.. autoclass:: renderccccc.BindType
  :members:

.. autoclass:: renderccccc.TextureSwizzle
  :members:

.. autoclass:: renderccccc.TextureSwizzle4
  :members:

Samplers
--------

.. autoclass:: renderccccc.AddressMode
  :members:

.. autoclass:: renderccccc.TextureFilter
  :members:

.. autoclass:: renderccccc.FilterMode
  :members:
  
.. autoclass:: renderccccc.FilterFunction
  :members:

.. autoclass:: renderccccc.ChromaSampleLocation
  :members:

.. autoclass:: renderccccc.YcbcrConversion
  :members:

.. autoclass:: renderccccc.YcbcrRange
  :members:

Viewport and Scissor
--------------------

.. autoclass:: renderccccc.Viewport
  :members:

.. autoclass:: renderccccc.Scissor
  :members:

Rasterizer
----------

.. autoclass:: renderccccc.CullMode
  :members:

.. autoclass:: renderccccc.DepthBiasMode
  :members:

.. autoclass:: renderccccc.FillMode
  :members:

.. autoclass:: renderccccc.ConservativeRaster
  :members:

.. autoclass:: renderccccc.LineRaster
  :members:

.. autoclass:: renderccccc.ShadingRateCombiner
  :members:

.. autoclass:: renderccccc.RasterState
  :members:


Depth and Stencil
-----------------

.. autoclass:: renderccccc.DepthTestState
  :members:

.. autoclass:: renderccccc.StencilFace
  :members:

.. autoclass:: renderccccc.StencilOperation
  :members:

.. autoclass:: renderccccc.CompareFunction
  :members:

Blending
--------

.. autoclass:: renderccccc.ColorBlend
  :members:

.. autoclass:: renderccccc.BlendEquation
  :members:

.. autoclass:: renderccccc.BlendMultiplier
  :members:

.. autoclass:: renderccccc.BlendOperation
  :members:

.. autoclass:: renderccccc.LogicOperation
  :members:

Shader Messages
---------------

.. autoclass:: renderccccc.ShaderMessage
  :members:

.. autoclass:: renderccccc.ShaderMessageLocation
  :members:

.. autoclass:: renderccccc.ShaderMeshMessageLocation
  :members:

.. autoclass:: renderccccc.ShaderVertexMessageLocation
  :members:

.. autoclass:: renderccccc.ShaderPixelMessageLocation
  :members:

.. autoclass:: renderccccc.ShaderGeometryMessageLocation
  :members:

.. autoclass:: renderccccc.ShaderComputeMessageLocation
  :members:

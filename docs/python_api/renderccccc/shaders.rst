API Reference: Shaders
======================

This is the API reference for the functions, classes, and enums in the ``renderccccc`` module which represents the underlying interface that the UI is built on top of. For more high-level information and instructions on using the python API, see :doc:`../index`.

.. contents:: Sections
   :local:

.. currentmodule:: renderccccc

Descriptors
-----------

.. autoclass:: renderccccc.Descriptor
  :members:

.. autoclass:: renderccccc.SamplerDescriptor
  :members:

.. autoclass:: renderccccc.DescriptorFlags
  :members:
   
.. autoclass:: renderccccc.DescriptorCategory
  :members:
   
.. autoclass:: renderccccc.DescriptorType
  :members:
   
.. autofunction:: renderccccc.CategoryForDescriptorType
.. autofunction:: renderccccc.IsConstantBlockDescriptor
.. autofunction:: renderccccc.IsReadOnlyDescriptor
.. autofunction:: renderccccc.IsReadWriteDescriptor
.. autofunction:: renderccccc.IsSamplerDescriptor

.. autoclass:: renderccccc.DescriptorLogicalLocation
  :members:

.. autoclass:: renderccccc.DescriptorRange
  :members:

.. autoclass:: renderccccc.DescriptorAccess
  :members:

Reflection
----------

.. autoclass:: renderccccc.ShaderReflection
  :members:

.. autoclass:: renderccccc.ShaderStage
  :members:

.. autoclass:: renderccccc.ShaderStageMask
  :members:

.. autofunction:: renderccccc.MaskForStage
.. autofunction:: renderccccc.FirstStageForMask

.. autoclass:: renderccccc.SigParameter
  :members:

.. autoclass:: renderccccc.ShaderBuiltin
  :members:

.. autoclass:: renderccccc.ConstantBlock
  :members:

.. autoclass:: renderccccc.ShaderSampler
  :members:

.. autoclass:: renderccccc.ShaderResource
  :members:

Debug Info
----------

.. autoclass:: renderccccc.ShaderDebugInfo
  :members:

.. autoclass:: renderccccc.ShaderEncoding
  :members:
  
.. autoclass:: renderccccc.KnownShaderTool
  :members:
  
.. autofunction:: renderccccc.ToolExecutable
.. autofunction:: renderccccc.ToolInput
.. autofunction:: renderccccc.ToolOutput

.. autofunction:: renderccccc.IsTextRepresentation

.. autoclass:: renderccccc.ShaderEntryPoint
  :members:

.. autoclass:: renderccccc.ShaderSourceFile
  :members:

.. autoclass:: renderccccc.ShaderCompileFlags
  :members:

.. autoclass:: renderccccc.ShaderCompileFlag
  :members:

.. autoclass:: renderccccc.ShaderSourcePrefix
  :members:

Shader Constants
----------------

.. autoclass:: renderccccc.ShaderConstant
  :members:

.. autoclass:: renderccccc.ShaderConstantType
  :members:

.. autoclass:: renderccccc.ShaderVariableFlags
  :members:

.. autoclass:: renderccccc.VarType
  :members:

.. autofunction:: renderccccc.VarTypeByteSize
.. autofunction:: renderccccc.VarTypeCompType

Shader Debugging
----------------

.. autoclass:: renderccccc.ShaderDebugTrace
  :members:

.. autoclass:: renderccccc.ShaderDebugger
  :members:

.. autoclass:: renderccccc.SourceVariableMapping
  :members:

.. autoclass:: renderccccc.DebugVariableReference
  :members:

.. autoclass:: renderccccc.DebugVariableType
  :members:

.. autoclass:: renderccccc.LineColumnInfo
  :members:

.. autoclass:: renderccccc.InstructionSourceInfo
  :members:

.. autoclass:: renderccccc.ShaderDebugState
  :members:

.. autoclass:: renderccccc.ShaderEvents
  :members:

.. autoclass:: renderccccc.ShaderVariableChange
  :members:

Shader Variables
----------------
  
.. autoclass:: renderccccc.ShaderVariable
  :members:

.. autoclass:: renderccccc.ShaderValue
  :members:

.. autoclass:: renderccccc.PointerVal
  :members:

.. autoclass:: renderccccc.ShaderBindIndex
  :members:

.. autoclass:: renderccccc.ShaderDirectAccess
  :members:

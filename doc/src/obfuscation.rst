Obfuscation
~~~~~~~~~~~

Config
------

.. autoclass:: omvll.ObfuscationConfig
  :members:
  :inherited-members:
  :undoc-members:

Template
########

Here is a template for the main O-MVLL configuration file:

.. code-block:: python

  import omvll
  from functools import lru_cache

  class MyConfig(omvll.ObfuscationConfig):
      def __init__(self):
          super().__init__()

      def obfuscate_string(self, module: omvll.Module, func: omvll.Function,
                                 string: bytes):

          if func.demangled_name == "Hello::say_hi()":
              return True

          if "debug.cpp" in module.name:
              return "<REMOVED>"

          return False


  @lru_cache(maxsize=1)
  def omvll_get_config() -> omvll.ObfuscationConfig:
      return MyConfig()

Options
-------

Anti-Hooking
############

.. autoclass:: omvll.AntiHookOpt
  :members:
  :inherited-members:
  :undoc-members:

Arithmetic Obfuscation
######################

.. autoclass:: omvll.ArithmeticOpt
  :members:
  :inherited-members:
  :undoc-members:

Control-Flow Breaking
#####################

.. autoclass:: omvll.BreakControlFlowOpt
  :members:
  :inherited-members:
  :undoc-members:

Control-Flow Flattening
#######################

.. autoclass:: omvll.ControlFlowFlatteningOpt
  :members:
  :inherited-members:
  :undoc-members:

Opaque Constants
################

.. autoclass:: omvll.OpaqueConstantsBool
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.OpaqueConstantsLowerLimit
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.OpaqueConstantsSet
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.OpaqueConstantsSkip
  :members:
  :inherited-members:
  :undoc-members:


Opaque Fields Access
####################

.. autoclass:: omvll.StructAccessOpt
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.VarAccessOpt
  :members:
  :inherited-members:
  :undoc-members:

Strings Encoding
################

.. autoclass:: omvll.StringEncOptSkip
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.StringEncOptReplace
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.StringEncOptGlobal
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.StringEncOptDefault
  :members:
  :inherited-members:
  :undoc-members:

.. autoclass:: omvll.StringEncOptStack
  :members:
  :inherited-members:
  :undoc-members:



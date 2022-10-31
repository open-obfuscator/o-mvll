<p align="center">
  <br />
  <br />
  <a href="https://obfuscator.re/omvll">
    <img src=".github/img/banner.webp" alt="O-MVLL" width="100%">
  </a>
</p>

<p align="center">
  <a href="LICENSE">
    <img src="https://img.shields.io/github/license/open-obfuscator/omvll">
  </a>
</p>


# O-MVLL

O-MVLL (in reference to [O-LLVM](https://github.com/obfuscator-llvm/obfuscator)) is a LLVM-based obfuscator
driven by a Python API and by new LLVM pass manager:

```bash
clang++ -fpass-plugin=omvll.so main.cpp -o main
```

```python
import omvll

class MyConfig(omvll.ObfuscationConfig):
    def __init__(self):
        super().__init__()

    def flatten_cfg(self, mod: omvll.Module, func: omvll.Function):
        if func.name == "check_password":
            return True
        return False
```

<img src=".github/img/omvll-pipeline.webp" alt="O-MVLL Pipeline" width="100%">

O-MVLL can be used with the Android NDK and an iOS toolchain but **it only supports the AArch64 architecture**.

For the details, you can checkout the documentation: [obfuscator.re/omvll](https://obfuscator.re/omvll)

### Contact

You can reach out by email at this address: `ping@obfuscator.re`

#### Maintainers

- [Romain Thomas](https://www.romainthomas.fr): [@rh0main](https://twitter.com/rh0main) - `me@romainthomas.fr`

#### Credits

- [LLVM](https://llvm.org/)
- [obfuscator-llvm](https://github.com/obfuscator-llvm/obfuscator)
- [obfuscator-llvm](https://github.com/eshard/obfuscator-llvm) from [eShard](https://eshard.com/)
- [Hikari](https://github.com/HikariObfuscator/Hikari)
- [DragonFFI](https://github.com/aguinet/dragonffi)

### License

O-MVLL is released under the same License as LLVM: [Apache License, Version 2.0](./LICENSE)
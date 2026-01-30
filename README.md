# SC Faust

SC Faust is a server extensions which allows to compile [Faust](https://faust.grame.fr/) code on a SuperCollider server and use this compiled code as a UGen.

## Installation

Builds are made available via <https://github.com/capital-G/sc_faust/releases>.
Download and extract the content of the archive into the `Platform.userExtensionDir` folder.

### macOS De-Quarantine

Since the plugin is not notarized it needs to be de-quarantined.
Run the following command within SuperCollider, assuming you have installed sc_faust like specified above

```supercollider
"xattr -rd com.apple.quarantine \"%/sc_faust\"".format(Platform.userExtensionDir).unixCmd;
```

### Build

sc_faust links faust statically such that it becomes independent of local faust installations.

It is necessary to have faust or faustlib installed which can be obtained via <https://github.com/grame-cncm/faust/releases>.
For Linux and macOS, this directory referenced via `FAUST_LIB_DIR` should include a file `lib/libfaustwithllvm.a` and for windows `lib/libfaustwithllvm.lib`.

See `.github/workflows/build.yml` for detailed instructions on how to build.

```shell
git clone --recursive https://github.com/capital-G/sc_faust.git
cd sc_faust 

# replace SC_PATH with the path to your local source code copy of SuperCollider
# and FAUST_LIB_DIR with the path of your faust lib files
cmake \
    -S . \
    -B build \
    -DSC_PATH=/Users/scheiba/github/supercollider \
    -DFAUST_LIB_DIR=~/libs/Faust-2.83.1
cmake --build build --config Release
cmake --install build --config Release
```

By default, sc_faust will be installed to the default SuperCollider user extension directory which can be located by evaluating `Platform.userExtensionDir;` within sclang.
You can override the installation path by setting the `SC_INSTALL_DIR` variable.

## Development

Please install [`pre-commit`](https://pre-commit.com/) and run `pre-commit install` to make sure that all commits are formatted according to the clang-format rules.

## License

Faust is licensed via GPL-2.0

This project is licensed via GPL-3.0

# Building Traceeshark from source

The general flow of building Traceeshark is as follows:

1. Clone Traceeshark

2. Clone Wireshark under the Traceeshark directory

3. Set up the build environment

4. Checkout the desired Wireshark version to build Traceeshark against

5. Configure Wireshark normally and install Wireshark's headers

6. Build Traceeshark

7. Optional: create a distribution archive

8. Repeat from step 4 to build against another version of Wireshark

### 1. Clone Traceeshark

:warning: On Windows, before cloning Traceeshark, make sure to run the following command: `git config --global core.autocrlf false`. Without this, git will insert carriage returns into the files and will break an important shell script!

```bash
git clone git@bitbucket.org:scalock/traceeshark.git
cd traceeshark
```

### 2. Clone Wireshark

Clone the Wireshark source into the Traceeshark directory (Makefile and build scripts depend on this location):

```bash
git clone https://github.com/wireshark/wireshark
```

### 3. Setting up the build environment

Follow Wireshark's instructions for setting up the build environment:

- [Linux & Mac](https://www.wireshark.org/docs/wsdg_html_chunked/ChapterSetup.html#ChSetupUNIXBuildEnvironmentSetup)

- [Windows](https://www.wireshark.org/docs/wsdg_html_chunked/ChSetupWindows.html) - follow sections 2.2.2 and 2.2.3. Ignore the instructions about setting up environmet variables, instead edit `scripts\setup_env.bat` with the appropriate paths to the Wireshark repository, the Qt installation and the Visual Studio installation. Run this script **in any new shell** you use to build Traceeshark/Wireshark.

### 4. Checkout the desired Wireshark version

Release versions of Wireshark have tags in the form `wireshark-x.y.z`. Checkout the desired tag, for example:

```bash
cd wireshark
git checkout wireshark-4.2.6
cd ..
```

:information_source: Note that source code compatibility of Traceeshark is not guaranteed, not all versions of Wireshark are supported. Incompatible versions will result in compilation errors.

### 5. Configure Wireshark and install headers

Configure Wireshark normally so the headers are generated and can be installed.

On Linux and Mac:

```bash
mkdir wireshark/build
cd wireshark/build
cmake ..
```

On Windows:

```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..\wireshark
```

:information_source: If `Visual Studio 17 2022` is not a valid toolchain on your system, you can list the available options using `cmake -G`

Next, install Wireshark's headers.

On Linux and Mac:

```bash
sudo make install-headers
```

On Windows (requires an elevated command prompt):

```batch
msbuild install-headers.vcxproj
```

Keep in mind that this stage needs to be repeated for every Wireshark version you want to build Traceeshark against.

### 6. Build Traceeshark

Building Traceeshark is managed using a Makefile on Linux and Mac and build scripts on Windows.

Before building for the first time, Wireshark needs to be configured again, this time for Traceeshark. This step needs to be performed any time there is a change to the Wireshark repository or to the file structure of Traceeshark.

On Linux and Mac:

```bash
make cmake
```

:warning: If your system does not have a Qt6 package available (e.g. Ubuntu 20.04 and older), run `make cmake USE_QT5=y` instead (this is necessary only if you plan running the Wireshark version that will be built, if you have a working Wireshark installation this is not necessary).

On Windows:

```batch
scripts\cmake.bat
```

Next, build Wireshark together with the Traceeshark plugins.

On Linux and Mac:

```bash
make
```

On Windows:

```batch
scripts\build.bat
```

The Linux and Mac Makefile has a few extra targets that are useful for development:

```bash
# Install plugins and other Traceeshark
# files into their destinations.
make install

# Run Wireshark from the build directory.
# Runs the install target automatically.
make run

# Same as run target, but with debug output enabled
make debug
```

### 7. Create a distribution archive

A distribution archive with an installation script can be created for anyone with the same OS and architecture. Note that a distribution targets the specific Wireshark version that was used while building. To build for a different version, go back to step 4.

On Linux and Mac:

```bash
make dist
```

On Windows:

```batch
scripts\dist.bat
```

The archive will be written to the `dist` directory.
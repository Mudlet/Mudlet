# Setting up for tests

## Linux

Have Mudlet and Qt6 development packages installed:

```sh
# Ubuntu/Debian
sudo apt-get install build-essential cmake qt6-base-dev qt6-tools-dev libqt6test6

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qttools-devel

# Arch
sudo pacman -S base-devel cmake qt6-base qt6-tools
```

## macOS

```sh
brew install cmake qt6
```

## Windows

- Install Visual Studio (the [free community edition](https://visualstudio.microsoft.com/vs/community/) works)
- Install [Qt6](https://www.qt.io/download-qt-installer) with the Qt Test module
- Install [CMake](https://cmake.org/download/)

You're now ready to build and run the tests.

## Running tests

1. Build Mudlet from source (see [Compiling Mudlet](https://wiki.mudlet.org/w/Compiling_Mudlet))

2. From the build directory, run all tests:

```sh
# Run all C++ tests
ctest

# Run with verbose output
ctest --verbose

# Run tests in parallel
ctest -j4
```

3. Or run individual tests:

```sh
# From the build/test directory
./TEntityResolverTest
./TMxpTagParserTest
./TLuaInterfaceTest
```

## Creating tests

See [Qt Test documentation](https://doc.qt.io/qt-6/qtest-overview.html) and currently existing tests for examples on how to write tests.

### Test structure

Each test file should test a specific C++ class or module. Tests follow Qt Test conventions with test methods as `private slots`.

```cpp
#include <QtTest/QtTest>
#include "YourClass.h"

class YourClassTest : public QObject {
    Q_OBJECT

private slots:
    void testBasicFunctionality()
    {
        YourClass instance;
        QCOMPARE(instance.someMethod(), expectedValue);
        QVERIFY(instance.isValid());
    }

    void testErrorHandling()
    {
        YourClass instance;
        QCOMPARE(instance.invalidOperation(), false);
    }
};

#include "YourClassTest.moc"
QTEST_MAIN(YourClassTest)
```

To add your test to the build, edit `test/CMakeLists.txt`:

```cmake
add_executable(YourClassTest YourClassTest.cpp ../src/YourClass.cpp)
add_test(NAME YourClassTest COMMAND YourClassTest)
```

See existing test files for examples and ask on Discord if you still need help.

# Mudlet C++ Qt Tests

This directory contains unit tests for Mudlet's C++ codebase using the Qt Test framework.

## Running Tests

### Command Line

From the build directory, you can run tests using several methods:

#### Run all tests
```bash
# go into the build directory, wherever that might be
cd build/

# run test
ctest --verbose
```

#### Run specific tests
```bash
# Run a single test by name
ctest -R TEntityResolverTest

# Run tests matching a pattern
ctest -R "TMxp.*"
```

### Qt Creator integration

Qt Creator provides excellent integration for running and debugging Qt tests:

#### Running tests from Qt Creator

1. **Open the Project**: Open Mudlet's main CMakeLists.txt in Qt Creator
2. **Run tests**: `Tools` → `Tests` → `Run All Tests`


## Contributing New Tests

### Test Structure

All Qt tests in Mudlet follow this basic structure:

```cpp
#include "GroupedTest.h"
#include <SourceFileToTest.h>

class MyComponentTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup before all tests
    }

    void init() {
        // Setup before each test
    }

    void testBasicFunctionality() {
        // Your test implementation
        QCOMPARE(actualValue, expectedValue);
        QVERIFY(condition);
    }

    void cleanup() {
        // Cleanup after each test
    }

    void cleanupTestCase() {
        // Cleanup after all tests
    }
};

MUDLET_GROUPED_TEST_MAIN(MyComponentTest)
#include "MyComponentTest.moc"
```

### Adding a New Test

1. **Create Test File**: Create `YourTestName.cpp` in the `test/` directory. Keep the words `install`, `uninstall`, `setup`, `update` and `patch` out of the name: Windows takes an unsigned executable named that way for an installer and will not start it from an ordinary, non-elevated Windows shell. CMake rejects such a name at configure time.

2. **Implement Test Class**: Follow the structure above, inheriting from `QObject` and using `Q_OBJECT` macro. Name the class after the file: ctest runs it by that name.

3. **Add to CMakeLists.txt**: Add `YourTestName` to the `UNIT_TESTS` list in `test/CMakeLists.txt`. Every test in that list is compiled into one `unit_tests` executable linked against the Mudlet library, so `MUDLET_GROUPED_TEST_MAIN()` (or `MUDLET_GROUPED_TEST_GUILESS_MAIN()` for a test that needs no display) takes the place of `QTEST_MAIN()`. Each test is still its own ctest case and runs in its own process; to run one by hand, name it: `./unit_tests YourTestName`.

A test that needs only a source file or two and not the whole Mudlet library gets an `add_executable()` of its own instead - see `SemVerTest` in `test/CMakeLists.txt`.

### Best Practices

- **Test Naming**: Use descriptive test method names like `testEntityResolutionWithStandardEntities()`
- **Test Independence**: Each test should be independent and not rely on other tests
- **Setup/Cleanup**: Use `init()`/`cleanup()` for per-test setup, `initTestCase()`/`cleanupTestCase()` for global setup
- **Assertions**: Use appropriate Qt Test macros:
  - `QCOMPARE(actual, expected)` for equality checks
  - `QVERIFY(condition)` for boolean conditions
  - `QVERIFY_EXCEPTION_THROWN(expression, exception)` for exception testing
- **Test Data**: For data-driven tests, use `QFETCH` and `QTest::addColumn`

### Example: Simple Test Implementation

```cpp
#include "GroupedTest.h"
#include <TEntityResolver.h>

class TEntityResolverTest : public QObject {
    Q_OBJECT

private slots:
    void testStandardEntities() {
        TEntityResolver resolver;

        QCOMPARE(resolver.getResolution("&nbsp;"), " ");
        QCOMPARE(resolver.getResolution("&gt;"), ">");
        QCOMPARE(resolver.getResolution("&lt;"), "<");
        QCOMPARE(resolver.getResolution("&amp;"), "&");
    }
};

MUDLET_GROUPED_TEST_MAIN(TEntityResolverTest)
#include "TEntityResolverTest.moc"
```

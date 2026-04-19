---
name: tdd
description: "Use when planning tests before implementation, writing Catch2 C++ unit tests, writing pytest Python tests, or practicing test-driven development for Noddle features. Covers test planning, template generation, execution, and coverage reporting."
---

# TDD Workflow for Noddle

## When to Use
- Before implementing any new feature or node
- When the orchestrator delegates test planning
- When validating existing code after refactoring
- When adding regression tests for bug fixes

## TDD Cycle
1. **Red**: Write failing tests that define expected behavior
2. **Green**: Implement minimum code to pass tests
3. **Refactor**: Clean up while keeping tests green

## Procedure

### Step 1 — Test Plan
Read the feature spec or interface contract. Identify:
- **Happy path**: Normal operation with valid inputs
- **Edge cases**: Empty input, single element, maximum size
- **Error cases**: Wrong dtype, null pointer, dimension mismatch, missing file
- **Boundary values**: 0, 1, MAX_INT, negative numbers

Document as a checklist before writing any test code.

### Step 2 — Write C++ Tests (Catch2 v3)

Use this template for new test files:

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "noddle/core/<header>.hpp"

TEST_CASE("<Component> — <scenario>", "[<tag>]") {
    // Arrange
    <setup objects>

    // Act
    <call function>

    // Assert
    REQUIRE(<condition>);
}

TEST_CASE("<Component> — handles empty input", "[<tag>][edge]") {
    REQUIRE_THROWS_AS(<call with bad input>, std::exception);
}
```

**Naming convention**: `TEST_CASE("<Class> — <behavior description>", "[tag]")`
**Tags**: `[tensor]`, `[runtime]`, `[opencv]`, `[source]`, `[serialization]`, `[edge]`

**File location**: `tests/test_<component>.cpp`

**CMake integration**:
```cmake
add_executable(noddle_tests
    test_tensor.cpp
    test_source_nodes.cpp
)
target_link_libraries(noddle_tests PRIVATE Catch2::Catch2WithMain noddle_core)
include(CTest)
include(Catch)
catch_discover_tests(noddle_tests)
```

### Step 3 — Write Python Tests (pytest)

Use this template:

```python
import pytest

class TestComponentName:
    """Tests for <component>."""

    def test_happy_path(self):
        # Arrange
        ...
        # Act
        result = ...
        # Assert
        assert result == expected

    def test_edge_case_empty_input(self):
        with pytest.raises(ValueError):
            ...

    @pytest.fixture
    def sample_data(self):
        return ...
```

**Naming convention**: `test_<component>.py`, class `TestComponentName`, method `test_<behavior>`
**File location**: `tests/test_<component>.py`

### Step 4 — Execute Tests
```bash
# C++ tests
cd build && ctest --output-on-failure

# Python tests
python -m pytest tests/ -v --tb=short
```

### Step 5 — Report
Return:
1. **Test plan**: List of test cases with rationale
2. **Test code**: Complete, compilable/runnable files
3. **Results**: Pass/fail with output
4. **Coverage gaps**: Untested paths to address later

## Test Quality Checklist
- [ ] Each test tests ONE behavior
- [ ] Test names describe the expected behavior
- [ ] No test depends on another test's state
- [ ] No test depends on external files or network
- [ ] Edge cases covered (empty, null, overflow, wrong type)
- [ ] Both success and failure paths tested
- [ ] Assertions use REQUIRE (not CHECK) for critical invariants

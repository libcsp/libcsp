# Contributing to csp_es

Thank you for your interest in contributing to csp_es, the EnduroSat fork of libcsp!

## About This Fork

This repository is a fork of [libcsp](https://github.com/libcsp/libcsp) maintained by EnduroSat. We welcome contributions that:

- Fix bugs
- Improve portability
- Add support for new platforms
- Enhance documentation
- Improve test coverage

## Getting Started

### Prerequisites

- Git
- CMake ≥3.20
- GCC or Clang with C11 support
- Ninja build system

### Building

```bash
git clone https://github.com/endurosat/csp-es.git
cd csp_es
cmake -G Ninja -B build -DCSP_BUILD_SAMPLES=ON
cmake --build build
```

### Running Tests

```bash
./build/examples/csp_arch
./build/examples/csp_server_client -T 10
```

## How to Contribute

### Reporting Bugs

1. Check [existing issues](../../issues) to avoid duplicates
2. Use the bug report template
3. Include:
   - CSP version
   - Platform/OS
   - Build configuration
   - Steps to reproduce
   - Expected vs actual behavior

### Suggesting Features

1. Open an issue with the feature request template
2. Describe the use case
3. Explain why this benefits the project

### Submitting Code

1. **Fork** the repository
2. **Create a branch** from `main`:
   ```bash
   git checkout -b feature/my-feature
   ```
3. **Make your changes** following our coding standards
4. **Test** your changes
5. **Commit** with clear messages (see below)
6. **Push** to your fork
7. **Open a Pull Request**

## Coding Standards

Please follow the [libcsp coding standards](./doc/codestyle.md):

- Use tabs for indentation
- K&R brace style
- Maximum line length: 120 characters
- Document public APIs with Doxygen comments

### Example

```c
/**
 * Brief description of function.
 *
 * @param[in] param1 Description of param1
 * @param[out] param2 Description of param2
 * @return Description of return value
 */
int csp_example_function(int param1, int *param2)
{
	if (param1 < 0) {
		return CSP_ERR_INVAL;
	}

	*param2 = param1 * 2;
	return CSP_ERR_NONE;
}
```

## Commit Messages

Follow the [libcsp Git Commit Guidelines](./doc/git-commit.md):

```
component: Short description (max 50 chars)

Longer description if needed. Wrap at 72 characters.
Explain what and why, not how.

Fixes #123
```

### Components

- `core:` - Core library changes
- `interfaces:` - Interface implementations
- `drivers:` - Driver implementations
- `arch:` - Architecture-specific code
- `build:` - Build system changes
- `doc:` - Documentation
- `test:` - Test code
- `examples:` - Example code

## Pull Request Process

1. Update documentation if needed
2. Add tests for new functionality
3. Ensure all CI checks pass
4. Request review from maintainers
5. Address review feedback
6. Squash commits if requested

## Upstream Contributions

If your contribution would benefit the upstream libcsp project:

1. We encourage you to also submit it to [libcsp](https://github.com/libcsp/libcsp)
2. Mention in your PR if you've done so
3. We will help coordinate if needed

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

- Open an issue with the "Q&A" label
- Check existing documentation in `/doc`

Thank you for contributing! 🚀


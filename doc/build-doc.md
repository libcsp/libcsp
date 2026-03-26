# How to build documentation

```{contents}
:depth: 3
```

## With Docker (recommended)

If you have the CI image built (see `ci/Dockerfile`), docs build with no local setup:

```shell
docker run --rm -v $(pwd):/workspace -w /workspace --user $(id -u):$(id -g) csp-es-ci \
  bash -c "cmake -B build-docs -S doc && cmake --build build-docs"
```

The output is placed in `build-docs/html/`.

## Without Docker (local venv)

```shell
python3 -m venv venv
. venv/bin/activate
pip install -r doc/requirements.txt
cmake -S doc -B builddir
cmake --build builddir
```

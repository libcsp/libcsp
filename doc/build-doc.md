# How to build documentation

```{contents}
:depth: 3
```

To build the documentation, follow these steps:

```shell
python3 -m venv venv
. venv/bin/activate
pip install -r doc/requirements.txt
mkdir builddir
cmake -S doc -B builddir
cmake --build
```

Note that currently, there is a problem building it under `doc/`. See
https://github.com/libcsp/libcsp/issues/572 for more details.

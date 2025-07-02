# Binning Application

This project provides a small web server for 1D nesting / bin packing
algorithms written in C++. The repository layout has been simplified so
that all source code lives in `src/` and header files in `include/`.

## Building

You can build the command line application using `make` if the HiGHS
library is installed:

```bash
make
```

The resulting executable will be placed in `bin/tube-designer`.

Alternatively a Docker image can be built using the provided
`Dockerfile` or started with `docker-compose`:

```bash
docker build -t one-d-nesting .
```

## Running

For a local run via Docker Compose:

```bash
docker-compose up --build
```

This will expose the web server on port 8080 and serve the static
front-end from the `static/` directory.

# 1D Nesting

A lightweight tool that finds efficient cutting patterns for linear stock. It uses the HiGHS solver and includes a small web UI so you can try optimisations from any browser.

## Features

- Mixed integer solver for minimal waste
- Accepts feet, inches, and fractions
- Kerf support
- Simple web interface
- Ready-to-run Docker setup

## Quick start

```bash
# clone and run with docker compose
git clone https://github.com/heavybullets8/1d-nesting.git
cd 1d-nesting
docker-compose -f docker/docker-compose.yml up
```

Open `http://localhost:8080` to use the UI.

## Development

### NixOS / Nix users

Enter the development shell (provides all dependencies + LSP support):
```bash
nix-shell
```

This automatically:
- Installs all dependencies (HiGHS, build tools, clang-tools)
- Sets up environment variables for LSP
- Generates `compile_commands.json` for clangd

Then build with:
```bash
make web
```

### Other systems

Install dependencies and build:
```bash
./setup-deps.sh
make web
```

## Example input

```json
{
  "jobName": "Kitchen Cabinets",
  "stockLength": "8'",
  "kerf": "1/8",
  "cuts": [
    { "length": "24 3/4", "quantity": 4 },
    { "length": "18 1/2", "quantity": 6 }
  ]
}
```

## API

POST `/api/optimize` with the JSON above. The response lists each cutting pattern and overall waste.

## Configuration

The server listens on port 8080. To expose a different external port, change the mapping in `docker-compose.yml`.

## Acknowledgements

- [HiGHS](https://highs.dev/)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [nlohmann/json](https://github.com/nlohmann/json)

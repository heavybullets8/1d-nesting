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
git clone https://github.com/yourusername/1d-nesting-software.git
cd 1d-nesting-software
docker-compose -f docker/docker-compose.yml up
```

Open `http://localhost:8080` to use the UI.

To build locally, run `./setup-deps.sh` followed by `make web`.

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

# 1D Nesting Software

A high-performance cutting optimization tool that minimizes material waste using
advanced algorithms. Perfect for woodworking, metal fabrication, and any
industry requiring efficient linear material cutting.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Docker](https://img.shields.io/badge/docker-ready-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-linux%20%7C%20macos%20%7C%20windows-lightgrey.svg)

## Features

- 🚀 **Advanced Optimization**: Uses HiGHS mixed-integer programming solver for
  optimal cutting patterns
- 📏 **Flexible Input**: Supports feet, inches, feet+inches, decimals, and
  fractions
- 🔧 **Kerf Support**: Accounts for blade thickness/material removal in
  calculations
- 🌐 **Web Interface**: Clean, responsive UI for easy access from any device
- 📊 **Visual Results**: Color-coded cutting patterns with efficiency metrics
- 🖨️ **Print-Ready Reports**: Generate professional cutting lists for your
  workshop
- 🐳 **Docker Ready**: Easy deployment with Docker and Docker Compose

## Quick Start

### Using Docker (Recommended)

```bash
# Clone the repository
git clone https://github.com/yourusername/1d-nesting-software.git
cd 1d-nesting-software

# Start with Docker Compose
docker-compose -f docker/docker-compose.yml up

# Or use the start script
chmod +x start.sh
./start.sh
```

Open your browser to `http://localhost:8080`

For detailed setup instructions, see [SETUP.md](SETUP.md).

### Building from Source

Requirements:

- C++17 compatible compiler
- HiGHS optimization library
- Make

```bash
# Download dependencies
chmod +x setup-deps.sh
./setup-deps.sh

# Build the web server
make web

# Run the server
./bin/nesting-server
```

## Input Examples

The software accepts various length formats:

| Input       | Interpreted As    |
| ----------- | ----------------- |
| `7' 6"`     | 7 feet 6 inches   |
| `7' 6 1/2"` | 7 feet 6.5 inches |
| `90 1/4`    | 90.25 inches      |
| `24'`       | 24 feet           |
| `1/8`       | 0.125 inches      |
| `0.125`     | 0.125 inches      |

## API Documentation

### POST /api/optimize

Optimize cutting patterns for given requirements.

**Request Body:**

```json
{
  "jobName": "Kitchen Cabinets",
  "materialType": "2x4 Pine",
  "stockLength": "8'",
  "kerf": "1/8",
  "cuts": [
    { "length": "24 3/4", "quantity": 4 },
    { "length": "18 1/2", "quantity": 6 },
    { "length": "36", "quantity": 2 }
  ]
}
```

**Response:**

```json
{
  "jobName": "Kitchen Cabinets",
  "solution": {
    "num_sticks": 3,
    "total_waste": 5.25,
    "efficiency": 94.5,
    "patterns": [
      {
        "count": 2,
        "cuts": [
          { "length": 36, "pretty_length": "3'" },
          { "length": 24.75, "pretty_length": "24 3/4\"" }
        ],
        "waste_len": 2.125
      }
    ]
  }
}
```

## Configuration

### Environment Variables

- `PORT`: Server port (default: 8080)
- `HOST`: Server host (default: 0.0.0.0)

### Docker Compose Configuration

Edit `docker/docker-compose.yml` to modify:

- Port mappings
- Volume mounts
- Environment variables
- Health check settings

## Development

### Project Structure

```
├── src/              # C++ source files
│   ├── algorithm.cpp # Core optimization algorithm
│   ├── parse.cpp     # Input parsing utilities
│   ├── output.cpp    # Output formatting
│   └── web_server.cpp# HTTP server implementation
├── include/          # C++ header files
├── static/           # Web frontend files
│   └── index.html    # Single-page application
├── docker/           # Docker configuration
│   ├── Dockerfile
│   └── docker-compose.yml
└── scripts/          # Utility scripts
```

### Building for Development

```bash
# Install dependencies
./setup-deps.sh

# Build all targets
make clean && make all

# Run tests
make test

# Build web server only
make web
```

### Adding Features

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Performance

The optimization algorithm uses HiGHS, a high-performance linear programming
solver. Performance characteristics:

- Handles 100+ cuts in under 1 second
- Memory efficient - scales linearly with problem size
- Supports fractional measurements with 1/32" precision

## Troubleshooting

### Common Issues

**Optimization returns no solution:**

- Check that no cut length exceeds stock length
- Ensure all inputs are positive numbers
- Verify kerf width is reasonable for material

## Acknowledgments

- [HiGHS](https://highs.dev/) - High-performance linear optimization solver
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) - Single-header HTTP
  library
- [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++

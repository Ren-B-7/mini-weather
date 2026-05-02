# mini-weather

`mini-weather` is a simple command-line interface (CLI) tool for fetching current weather information for one or more cities. It uses the [Open-Meteo API](https://open-meteo.com/) for weather data and geocoding.

## Features

- **Multi-city Support**: Fetch weather data for multiple cities simultaneously in parallel.
- **Unit Selection**: Choose between metric (Celsius) and imperial (Fahrenheit) units.
- **Detailed Info**: Displays current temperature and precipitation probability.
- **Fast and Lightweight**: Written in C with minimal overhead.

## Prerequisites

Before building `mini-weather`, ensure you have the following dependencies installed:

- `libcurl`: For making HTTP requests.
- `libcjson`: For parsing JSON data.
- `gcc`: C compiler.
- `make`: Build system.

On Debian/Ubuntu-based systems, you can install the dependencies with:
```bash
sudo apt-get install libcurl4-openssl-dev libcjson-dev build-essential
```

## Building

To build the project, run:

```bash
make all
```

The compiled executable will be located in the `bin/` directory.

## Usage

Run the `weather` command with the `--city` (or `-c`) flag followed by a comma-separated list of cities.

```bash
./bin/weather --city "New York,London,Tokyo"
```

### Options

| Option | Shorthand | Description | Default |
| :--- | :--- | :--- | :--- |
| `--city` | `-c` | Comma-separated list of city names. | (Required) |
| `--units` | `-u` | Units to use: `metric` (Celsius) or `imperial` (Fahrenheit). | `metric` |
| `--help` | `-h` | Display help message. | - |

### Example

```bash
./bin/weather --city "San Francisco,Paris" --units imperial
```

Output:
```
San Francisco - 62.4 F - 0% precipitation
Paris - 54.2 F - 10% precipitation
```

## Installation

You can install the tool to your local bin directory (`~/.local/bin` by default) using:

```bash
make install
```

To uninstall:

```bash
make uninstall
```

## Development

- **Format code**: `make format` (requires `clang-format`)
- **Linting**: `make lint` (requires `clang-tidy`)
- **Clean build artifacts**: `make clean`

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

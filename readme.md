# Quadtree Image Codec

A high-performance image compression system using quadtree decomposition with both lossless and lossy compression capabilities. Version 1.0.0

## Overview

This codec implements an advanced quadtree-based image compression algorithm designed for grayscale PGM images. It employs recursive quadtree decomposition to identify and efficiently encode uniform regions while maintaining high image quality. The system features detailed logging, progress tracking, and optional segmentation grid generation for visualization.

### Key Features

- Lossless and lossy compression modes
- Adaptive variance-based filtering
- Real-time progress visualization
- Detailed performance statistics
- Segmentation grid visualization
- Beautiful terminal-based UI with color support
- Comprehensive error handling and validation

## Technical Architecture

The system is organized into several core components:

### Core Components

1. **Codec Engine** (`codec.c`)
   - High-level compression/decompression orchestration
   - Error handling and status management
   - Operation workflow coordination

2. **Quadtree Implementation** (`quadtree.c`)
   - Recursive tree construction
   - Node management and memory handling
   - Variance calculations and statistics

3. **Compression Module** (`compression.c`)
   - Bit-level data encoding
   - Metadata header generation
   - Node-by-node recursive compression
   - Adaptive variance-based filtering

4. **Decompression Module** (`decompression.c`)
   - Bit stream parsing and validation
   - Tree reconstruction
   - Pixel data extraction

### Supporting Modules

- **PGM Handler** (`pgm.c`): Image file I/O operations
- **CLI Interface** (`cli.c`): Command-line argument processing
- **Logger** (`logger_utils.c`): Beautiful progress visualization
- **Grid Generator** (`segmentation_grid.c`): Visualization tools

## Installation

### Prerequisites

- C99-compatible compiler
- Standard C library
- Make build system

### Building

```bash
make clean
make all
```

## Usage

### Basic Commands

```bash
# Lossless compression
./codec -c -i input.pgm -o compressed.qtc

# Decompression
./codec -u -i compressed.qtc -o output.pgm

# Lossy compression with custom alpha
./codec -c -i input.pgm -o compressed.qtc -a 2.0

# Generate segmentation grid
./codec -c -i input.pgm -o compressed.qtc -g grid.pgm
```

### Command-Line Options

| Option       | Description                        | Default                   |
| ------------ | ---------------------------------- | ------------------------- |
| `-c`         | Compress mode                      | -                         |
| `-u`         | Decompress mode                    | -                         |
| `-i <file>`  | Input file path                    | Required                  |
| `-o <file>`  | Output file path                   | `default_compress_output.qtc`/`default_compress_input.pgm` |
| `-a <value>` | Compression alpha (>1.0 for lossy) | 1.0                       |
| `-g <file>`  | Generate segmentation grid         | Disabled                  |
| `-h`         | Show help message                  | -                         |

## File Format Specification

### QTC Format Structure

1. **Header**
   - Magic bytes ("Q1")
   - Timestamp
   - Compression statistics
   - Tree depth (n_levels)

2. **Data Section**
   - Node mean values (8 bits)
   - Error terms (2 bits)
   - Uniformity flags (1 bit)

### Compression Algorithm

The compression process follows these steps:

1. **Image Analysis**
   - Quadtree decomposition
   - Variance calculation
   - Region uniformity detection

2. **Adaptive Filtering** (Lossy Mode)
   - Variance-based thresholding
   - Dynamic threshold scaling
   - Region merging optimization

3. **Data Encoding**
   - Bit-level stream generation
   - Node property compression
   - Metadata embedding

## Performance Characteristics

### Time Complexity

- Tree Construction: O(n log n)
- Compression: O(n)
- Decompression: O(n)

where n = image pixels

## Development

### Code Structure

```md
src/
├── cli/          # Command-line interface
├── codec/        # Core compression logic
├── common/       # Shared utilities
├── core/         # Quadtree implementation
├── grid/         # Segmentation grid
├── io/           # File handling
└── logger/       # Logging utility
```

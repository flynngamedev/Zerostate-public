# ZeroState

A low-level true random number generator written in C++ 
using RDSEED and chaos theory.

## Pipeline

RDSEED (hardware) → Physics simulation → SHA-3 → Output

## Components

- **zerostate** — core TRNG binary
- **password** — cryptographic password generator  
- **visualiser** — real-time DX11 entropy visualiser

## Why ZeroState

Most RNGs depend on observable entropy sources. ZeroState 
chains hardware-level CPU entropy through a physics 
simulation making outputs practically impossible to 
predict or replicate externally.

## Requirements

- Windows 10/11
- MSVC or MinGW
- DirectX 11 (visualiser only)
- CPU with RDSEED support (Intel Ivy Bridge+ / AMD Zen+)

## Build

```bat
build.bat

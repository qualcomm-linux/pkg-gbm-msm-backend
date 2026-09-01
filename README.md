# gbm-msm-backend

A GBM (Generic Buffer Manager) backend implementation for Qualcomm MSM (Mobile Station Modem) platforms, designed to work as an overlay on top of upstream Mesa GBM.

## Overview

`gbm-msm-backend` provides hardware-accelerated buffer allocation and management for Qualcomm Adreno GPUs on MSM platforms. It implements the GBM backend ABI to integrate seamlessly with Mesa's GBM loader, enabling efficient graphics buffer operations for display and rendering pipelines.

## Architecture

The backend consists of several key components:

- **Buffer Allocation** (`buffer_alloc.c`): Interfaces with MSM DRM for GEM buffer operations
- **Buffer Layout** (`buffer_layout.c`): Calculates buffer dimensions, strides, and offsets
- **Schema Parser** (`schema_parser.c`): Parses XML format alignment specifications
- **GBM Interface** (`gbm_msm.c`): Implements the GBM backend ABI

## Building

### Prerequisites

- Meson build system (>= 0.50)
- libdrm (>= 2.4.75)
- Mesa GBM (>= 21.2)
- libxml2
- C compiler with C99 support

### Build Instructions

```bash
# Configure the build
meson setup build

# Compile
meson compile -C build

# Install
sudo meson install -C build
```

The library will be installed to the system library directory (typically `/usr/lib/<arch>/`).

## Usage

The backend is automatically loaded by Mesa's GBM loader when available. Applications using GBM will transparently utilize this backend on supported Qualcomm MSM platforms.


## Supported Formats

The backend supports a wide range of pixel formats. Format-specific alignment and layout requirements are defined in `default_fmt_alignment.xml`.


## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:

- Code style and standards
- Submitting pull requests
- Development workflow
- Security analysis requirements

## Code of Conduct

This project adheres to a code of conduct. By participating, you are expected to uphold this code. Please see [CODE-OF-CONDUCT.md](CODE-OF-CONDUCT.md) for details.

## Security

For security vulnerabilities, please see our [Security Policy](SECURITY.md) for responsible disclosure procedures.

## Maintainers

For questions or support, contact: Maintainers.gbm-msm-backend@qualcomm.com

See [MAINTAINERS.md](MAINTAINERS.md) for more information.

## License

This project is licensed under the GNU General Public License v2.0. See the [LICENSE](LICENSE) file for details.

Some components may be licensed under BSD-3-Clause-Clear. See individual file headers for specific licensing information.


## Related Projects

- [Mesa](https://www.mesa3d.org/) - Open source graphics drivers
- [libdrm](https://gitlab.freedesktop.org/mesa/drm) - Direct Rendering Manager library
- [Wayland](https://wayland.freedesktop.org/) - Display server protocol


## Support

For issues, questions, or feature requests, please open an issue on the project repository or contact the maintainers.

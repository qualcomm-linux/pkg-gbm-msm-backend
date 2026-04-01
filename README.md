# pkg-gbm-msm-backend

This is the debian packaging side of gbm-msm-backend project.
Upstream project is hosted here : https://github.com/qualcomm-linux/gbm-msm-backend

## Branches

**main**: Contains the code-of-conduct, contributing, licence, etc. and dispatch_workflows in .github folder.
          This branch does not contain any upstream code or debian/ metadata

**upstream/latest** : 

**debian/latest**: 

## Package Repo Branch Structure

| Branch | Purpose |
|--------|---------|
| `main` | Workflows, docs, and boilerplate files |
| `debian/<version>` | Version-specific tags/branches (e.g., `debian/1.1.0-1`) |
| `upstream/latest` | Latest upstream source (non-native packages) |
| `upstream/<version>` | Tagged upstream versions |

## Typical Workflows

1. **Upstream promotion**: When the upstream project tags a new release, the promote workflow merges it into the packaging branch and opens a PR.
2. **Upstream PR validation**: PRs in the upstream repo are validated against the package build to catch breakages early.
3. **Release**: A manual dispatch finalizes the changelog, builds the package, uploads artifacts to S3, and notifies [qcom-distro-images](https://github.com/qualcomm-linux/qcom-distro-images).

## Development

How to develop new features/fixes for the software. Maybe different than "usage". Also provide details on how to contribute via a [CONTRIBUTING.md file](CONTRIBUTING.md).

## Usage

Build: To build the the package, go to *Actions* tab, select the *Build Debian Package* workflow, then 'Run workflow'

Upstream Version Promotion: ...

The workflows of this repo use the reusable workflows from qcom-build-utils in the background. To understand more how everything
connects together, see https://github.com/qualcomm-linux/qcom-build-utils

## Installation Instructions

sudo dpkg -i libgbm-msm_x_arm64.deb
where x holds the version number of the package.


## Getting in Contact

How to contact maintainers. E.g. GitHub Issues, GitHub Discussions could be indicated for many cases. However a mail list or list of Maintainer e-mails could be shared for other types of discussions. E.g.

* [Report an Issue on GitHub](../../issues)
* [Open a Discussion on GitHub](../../discussions)
* [E-mail us](mailto:Maintainers.pkg-gbm-msm-backend@qualcomm.com) for general questions

## License

pkg-gbm-msm-backend is licensed under the [BSD-3-clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.

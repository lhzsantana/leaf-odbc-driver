# Release Process

This document describes how to create a release using GitHub Actions.

## Automatic Release (Recommended)

### Creating a Release from a Tag

1. **Create and push a version tag:**
   ```bash
   git tag -a v0.1.0 -m "Release version 0.1.0"
   git push origin v0.1.0
   ```

2. **GitHub Actions will automatically:**
   - Build the driver for Linux and macOS
   - Create a GitHub Release with the tag name
   - Attach the compiled binaries as release assets

### Manual Release via Workflow Dispatch

1. Go to **Actions** tab in GitHub
2. Select **Build and Release** workflow
3. Click **Run workflow**
4. Enter the version tag (e.g., `v0.1.0`)
5. Click **Run workflow**

## Release Artifacts

Each release includes:

- **leafodbc-linux.tar.gz**: Linux build with `libleafodbc.so`
- **leafodbc-macos.tar.gz**: macOS build with `libleafodbc.dylib`

Each archive contains:
- The compiled driver library
- README.md and BUILD.md
- Documentation (`docs/` directory)
- Example configuration files (`examples/` directory)

## Version Numbering

Follow [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: Backward-compatible functionality additions
- **PATCH**: Backward-compatible bug fixes

Examples:
- `v0.1.0` - Initial release
- `v0.1.1` - Bug fix release
- `v0.2.0` - New features added
- `v1.0.0` - First stable release

## Release Checklist

Before creating a release:

- [ ] Update `CHANGELOG.md` with new changes
- [ ] Ensure all tests pass
- [ ] Verify documentation is up to date
- [ ] Test the build locally
- [ ] Create and push the version tag
- [ ] Verify the GitHub Release was created correctly
- [ ] Test downloading and installing from the release

## Post-Release

After a release is created:

1. Update the main branch with any release notes
2. Announce the release (if applicable)
3. Monitor for any issues reported by users

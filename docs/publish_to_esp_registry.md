# Publish repository to the ESP Registry

This document provides clear, step-by-step instructions to prepare and publish an ESP-IDF component to the ESP Registry.

## Summary

- **Goal**: prepare metadata, versioning, and releases so consumers can install your component via `idf_component.yml`.

## Prerequisites

- A public GitHub account/repository.
- `git` installed locally.
- ESP-IDF installed and configured (`idf.py` available).
- Minimum repository files: `CMakeLists.txt` and `idf_component.yml`.

## Minimum repository structure

- `component/` with `CMakeLists.txt` and `include/` containing headers.
- `idf_component.yml` at the component root (or repository root for single-component repos).
- (Optional) `examples/` folder with projects that consume the component.

## Example `idf_component.yml`

Create a metadata file with the basic fields. Minimal example:

```yaml
name: "cli-api"
version: "1.0.2"
license: "MIT"
description: "CLI API for ESP-IDF"
repository: "https://github.com/<user>/<repo>"
documentation: "https://github.com/<user>/<repo>/tree/main/README.md"
issues: "https://github.com/<user>/<repo>/issues"
targets:
  - esp32
  - esp32s3
tags:
  - cli
dependencies:
  idf: ">=6.0.0"
```

In this repository there is an example at `examples/basic/main/idf_component.yml` referencing `pedroluisdionisiofraga/cli-api`.

## Steps to publish to the ESP Registry

1. Prepare metadata
   - Ensure `idf_component.yml` is correct and `version` follows semantic versioning (`MAJOR.MINOR.PATCH`).

2. Test local build
   - In a sample project that consumes the component, run:

   ```bash
   idf.py build
   ```

3. Commit and tag a release
   - Create and push a semantic Git tag:

```bash
git add .
git commit -m "Release v1.0.2"
git tag -a v1.0.2 -m "v1.0.2"
git push origin --tags
git push
```

1. Create a GitHub release
   - Optionally publish a release (assets or notes) on GitHub UI or with the `gh` CLI:

```bash
gh release create v1.0.2 -t "v1.0.2" -n "Release notes"
```

1. Make the repository public
   - The ESP Registry indexes public repositories; confirm repo visibility.

2. (Optional) Automate with GitHub Actions
   - Use Actions to create tags/releases on `main` or on `refs/tags` pushes.
   - Store `GITHUB_TOKEN` as a repository secret when needed.

3. Wait for indexing
   - The Registry may take a few minutes to index the new version.

## How consumers install the component

- In the consuming project, add the dependency to `idf_component.yml`:

```yaml
dependencies:
  <user>/<repo>: "*"
  idf: ">=6.0.0"
```

`"*"` always resolves to the latest published version. Pin an explicit version
(for example `"1.0.4"`) only when a build must stay reproducible.

Or let the tooling write it for you:

```bash
idf.py add-dependency "<user>/<repo>"
```

Example in this workspace: `examples/basic/main/idf_component.yml` references `pedroluisdionisiofraga/cli-api`.

## Post-publish verification

- Try adding the dependency in a local project and run `idf.py build`.
- Verify the correct version was downloaded and the build succeeds.

## Important CLI commands

- `compote registry login` to authenticate against the registry.
- `compote registry logout` to remove the local token and revoke access.
- `compote registry sync` to sync components into a project.
- `compote component pack` to build a component archive.
- `compote component upload` to publish a component version.
- `compote component delete` to permanently remove a published version.
- `compote component yank` to hide a version from normal resolution.
- `compote config set`, `compote config unset`, `compote config list`, and `compote config path` to manage CLI profiles.
- `compote manifest schema` to inspect the manifest format.
- `compote manifest add-dependency` to add dependencies to a project manifest.
- `compote project remove-managed-components` to clean generated managed components.

Use `compote <group> --help` or `compote <command> --help` to see the exact flags for each command.

## Removing a published version

- To remove a version from the ESP Registry, use the registry CLI `delete` command for that specific component version.
- The command accepts `--namespace`, `--name`, and `--version` and requires registry access with permission to manage that component.
- Example:

```bash
compote component delete --profile <profile> --namespace <namespace> --name <component> --version <version>
```

- If you only want to mark a version as unavailable for normal resolution, use `yank` instead. Yanked versions are still available when an exact version is requested.
- Deleting is permanent for that version and it cannot be restored or re-uploaded.

## Best practices

- Use semantic tags: `vMAJOR.MINOR.PATCH`.
- Include `README.md`, `LICENSE`, and working examples.
- Document ESP-IDF compatibility.
- Maintain changelog / release notes.

## Common issues

- Registry not appearing: ensure the repository is public and tag/release exist.
- Invalid `idf_component.yml`: formatting errors prevent recognition.
- Dependency not found: verify the repository name and exact version.

---

If you want, I can also:

- Generate a `README.md` with publishing instructions.
- Add a GitHub Action template to automate releases.

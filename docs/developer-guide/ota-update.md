# OTA Update

If new firmware versions are available, it checks for updates from the GitHub releases of the `gogo-boot/mystation`
repository. It uses the GitHub API to get the latest release information. You can use the following `curl` command to
fetch the latest release information:

## Github Release API Request Example

### Request Example

```curl
curl -L  -H "Accept: application/vnd.github+json" \
-H "Authorization: Bearer <YOUR-TOKEN>" \
-H "X-GitHub-Api-Version: 2022-11-28" \
https://api.github.com/repos/gogo-boot/mystation/releases/latest
```

> Detail : https://docs.github.com/en/rest/releases/releases#get-the-latest-release

### Response Example

The full example API response is stored in `test/ota/github-release-api.json` file.
only the `tag_name` , `name` and the `browser_download_url` of the release information is required for ota update:

```json5
{
    ...
    "tag_name": "v0.3.0",
    "assets": [
        {
            ...
            "name": "firmware.bin",
            ...
            "browser_download_url": "https://github.com/gogo-boot/mystation/releases/download/v0.3.0/firmware.bin"
        },
        ...
    ]
}
```

## Version Comparison

Current Version is defined in the `firmware_version` field of the `platformio.ini` file.
By compile time git tag information is embedded into `include/build_config.h` file.
using the following configuration in the `platformio.ini` file:

```ini
version_flag = -D BUILD_TIME=$UNIX_TIME
               !echo '-D VERSION=\\"'$(git describe --tags)'\\"'
...
```

> Note: It is dynamic value. so compiler compile always new binary even there is no code change.

It gets the latest version from Github release API. It accepts semantic versioning format (semver) for version
comparison. The version strings should be in the format of `MAJOR.MINOR.PATCH`, optionally prefixed with a `v` (e.g.,
`v1.2.3` or `1.2.3`).

If the latest version obtained from the GitHub release is greater than the current firmware version, it indicates newer
firmware available. and executes the OTA update process.

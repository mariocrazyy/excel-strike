<h1>
  <img src="https://cdn.jsdelivr.net/gh/homarr-labs/dashboard-icons/svg/open-source-initiative.svg" width="28">
  Excel-Strike
</h1>

Excel-Strike is a Counter-Strike 2 external ImGui application originally developed by **IMXNOOBX**, redesigned and rebranded under the name Microsoft Excel.

## Features Added
- Original functionality preserved
- Updated application icon and branding
- Excel-inspired interface
- Opens with the "Right-Shift" key only! (End is still the exit key)
- Daily log files archived on application shutdown
- Searchable Settings and an About tab with release information

Logging is always enabled. While the app is running, it writes to `!latest.log`
beside `excel.exe` and keeps a flushed recovery copy in the adjacent `logs`
folder. If the app is force-closed, that dated `[n]` log remains available. On
normal shutdown, the completed `!latest.log` replaces the recovery copy. Choose
the date order in Settings: European (`DD-MM-YYYY`) or American (`MM-DD-YYYY`).
Each entry includes its timestamp, severity, source file, and line number.

## Updates

At startup, the app checks the latest stable GitHub Release for `vX.Y.Z`. If its
version differs from the installed version, the app asks before updating or
downgrading. The release must include `excel.exe` and `excel.exe.sha256`. The
checksum file may contain the SHA-256 digest alone or in standard `sha256sum`
format. The app verifies the download, copies itself to `OfficeCore/updater.exe`,
and uses that helper to replace and restart `excel.exe`; existing files such as
`OfficeCore/debugger.exe` are left untouched.

The current app version is set in `src/updater/Updater.hpp`. Bump it for each
release and publish a stable GitHub Release named `Excel-Strike vX.Y.Z`, with a
matching `vX.Y.Z` tag and the two exact asset names above. No update manifest
file is required.

## Donations

- **₿ Bitcoin (BTC) - bc1q7c95ee3h6ynwug7uxqvpvcz78dy6s8afkqml5q**
- **Ξ Ethereum (ETH) - 0xabac244a9030ed43e2392282a7b8feefb85f2288**
- **◎ Solana (SOL) - G22j5qLoSxdbEsZv4WyHGfPvMQA1s6nbn3GZAfe4Y483**

## Credits & License

Originally developed by [IMXNOOBX](https://github.com/IMXNOOBX).

Forked from [cs2-external-esp](https://github.com/IMXNOOBX/cs2-external-esp).

Modified and rebranded as Excel-Strike.

Licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/).

## Disclaimer

Not affiliated with Microsoft Corporation or Valve Corporation.

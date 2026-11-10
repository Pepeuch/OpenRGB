# Aqua Computer D5 NEXT: offline lighting prototype

This document records the **lot 1** snapshot. The authorized read-only hardware
campaign, diagnostic tool, updated tests and optional read-only discovery are
documented in [AquaComputerD5NextReadOnly.md](AquaComputerD5NextReadOnly.md).
Its results do not approve any configuration writes or firmware compatibility.

Status: 10 November 2026, branch `feat/d5next`, starting revision `b9d903b5`.
This is a preparation for device support, not validated hardware support. No real
device was opened, read, written, reset, or enumerated during development. The
modified OpenRGB application was not launched. No installed udev rules or system
permissions were changed. No commit, push, or pull request was made.

## Repository analysis and integration choices

OpenRGB uses C++17, Qt for the UI, and QMake. `OpenRGB.pro` recursively includes
controller headers and sources under `Controllers/`, so no main project change
is necessary. Tests live outside that tree to avoid adding a second `main()` to
the application. There was no first-party controller unit-test target in this
checkout; dependency tests and CI package smoke tests are not suitable for this
codec. A standalone QMake console target uses the existing compiler and headers.

`CONTRIBUTING.md` and `Documentation/RGBControllerAPI.md` were read before edits.
The user-specified [RGBController API](https://github.com/Pepeuch/OpenRGB/blob/master/Documentation/RGBControllerAPI.md)
matched the local file, Git blob `7fce17de23dec02e113965102ffa60435cdc56b4`, when
checked. The API documentation's ResourceManager registration description is
older than this checkout's detectors: current HID callbacks return
`DetectedControllers` to `DetectionManager`, which performs registration.
The implementation follows those current signatures.

Reference controllers examined:

| Reference | Relevant convention |
| --- | --- |
| NZXT Kraken | HID detector, cooler device type, controller ownership and `Shutdown()` |
| Corsair Lighting Node | Controller/detector/adapter separation, hardware effect representation |
| Realtek ARGB | Feature reports, support checks and synchronization |
| Elgato Key Light | Single logical lighting zone; existing HSV conversion usage |

The existing CRC++ dependency is reused with CRC-16/USB parameters, without
enabling its optional global algorithm definitions. The existing `qt/hsv.cpp`
converts hue to integer degrees and uses an undefined hue sentinel for grayscale.
The new pure codec computes hue directly in device units using integer sector
arithmetic, preserving finer precision and assigning grayscale a deterministic
zero hue. No Qt types, Go/Python runtime, service, or new library is used.

The native layers are:

1. `D5NextProtocol`: buffer validation, HSV conversion, decoding and constrained
   report construction. It has no HID dependency.
2. `AquaComputerD5NextController`: owns an existing HID handle; serializes complete
   read/modify/compare/write transactions and suppresses unchanged writes.
3. `RGBController_AquaComputerD5Next`: maps mode-specific colors, brightness and
   off to the controller. Its destructor stops the base worker before deleting
   the controller and closing the handle.
4. `AquaComputerD5NextControllerDetect`: prepares native HID discovery using
   `REGISTER_HID_DETECTOR_I`, VID `0x0C70`, PID `0xF00E`, interface `1`.

The adapter exposes one fixed `ZONE_TYPE_SINGLE` zone, one logical global light,
and Off, Static, Breathing and Rainbow. Static/Breathing have one mode-specific
color; Rainbow has none. There are no per-LED, Direct, Custom, speed, direction,
save or resize capabilities. This logical light does not claim individual access
to physical pump LEDs or external RGBpx strips. Unknown firmware/serial metadata
is left unset rather than fabricated.

## Activation barriers and discovery

**Normal OpenRGB builds do not register this detector.** Its production callback
returns an empty result without opening a device. Registration and its discovery
body are compiled only with `OPENRGB_D5NEXT_OFFLINE_TESTS`, defined solely in the
standalone tests. That executable provides HID functions in-process and links no
hidapi/libusb backend. Do not define this test macro in an application build.

The production controller constructor always selects `D5NEXT_LAYOUT_UNKNOWN`.
All lighting commands refuse that layout before any feature-report call, even if
the controller is instantiated manually. The test-only overload selects the
documented layout for synthetic buffers. **No real firmware is allowlisted.** A
valid size, CRC, VID/PID or USB release number is not firmware/layout approval.
The pure codec's `D5NEXT_LAYOUT_DOCUMENTED_809` argument is an explicit offline
format assumption, not an automatic recognizer or permission to write hardware.

In the simulated detector, the HID path comes exclusively from enumerated
`hid_device_info`; users do not configure a path. The callback checks identity,
interface and path, calls `hid_open_path`, ignores failed opens, and returns a
native adapter named `Aqua Computer D5 NEXT`. Construction sends **no GET_FEATURE,
SET_FEATURE or save report**. Even simulated registration/profile callbacks on
this unknown-layout object cannot access feature reports. Opening failure is
logged through LogManager. Destruction closes each successfully owned handle.

Successive full scans use OpenRGB's existing `ProcessPreDetection()` cleanup
lifecycle, which destroys previous controllers before rediscovery. No persistent
path cache or second device registry is added: such a cache could incorrectly
prevent reappearance after unplug/replug. Tests model that lifecycle, including
disappearance and rediscovery. They exercise the **actual registration macro and
types**, with manager startup/registration replaced at link time. They do not
start the real discovery background thread or GUI. Actual Scan Devices behavior,
hotplug, and native enumeration deduplication still require later integration
validation on hardware.

Non-root operation will depend on the normal hidapi backend and suitable existing
udev permissions. No root requirement, manual hidraw configuration, new rule or
permission workaround is added. Permission behavior is simulated by an open
failure; actual platform permissions have not been tested. The future GravaStar
K98 Pro detector (`372E:10E5`, interface `2`) is outside this change.

## Protocol and preservation contract

All offsets include the report ID. Feature report `0x03` is exactly 809 bytes.
CRC-16/USB has polynomial `0x8005`, initial value `0xFFFF`, reflected input/output,
and final XOR `0xFFFF`. It covers 806 bytes, `[1, 807)`, and is stored big-endian
at offsets 807 and 808. The standard `123456789` check is `0xB4C8`.

| Offset | Allowed field | Accepted values in this prototype |
| --- | --- | --- |
| 237 | Brightness / HSV value | 0–255 |
| 239 | Lighting flag | `0x00` on, `0x02` off |
| 663 | Hardware effect | `0x01` static, `0x02` breathing, `0x03` rainbow |
| 706–707 | Hue, big-endian | 0–1535 |
| 708 | Saturation | 0–255 |
| 807–808 | CRC | Recomputed from the candidate |

Report validation checks pointer, exact size, ID, then CRC. Decoding additionally
requires an explicitly selected layout and known lighting values; a CRC-valid
baseline with an unknown flag, hue or effect is refused rather than normalized.
Construction copies the baseline, patches the six lighting bytes, recalculates
CRC, validates both reports, then compares **every byte** against the allowlist.
Every other byte, including effect parameters at 674–689, remains unchanged.
An audit also rejects an unauthorized byte change when its CRC is recomputed.
Failure leaves output untouched. In-place and overlapping output are supported.

RGB input is interpreted as OpenRGB `0x00BBGGRR`. A full hue circle is assumed to
have 1536 steps, yielding red 0, green 512 and blue 1024; saturation and HSV value
are rounded to 8-bit units. Black and white have hue/saturation zero, with value
0/255 respectively. The separate mode brightness scales HSV value with rounding;
Rainbow changes brightness/effect/enable and preserves the stored hue/saturation.
Off changes only offset 239 and the CRC, preserving color, brightness and effect.
A repeated command reads a fresh baseline and skips SET if it is already equal.

The references give approximate captured primary hues of 511/1023, versus the
1536-step mathematical mapping's 512/1024. This discrepancy, the full brightness
range and the interpretation of `0x00` as on need hardware confirmation. Breathing
and Rainbow selectors are prepared, but retained effect parameters and interaction
with device palettes/lighting groups have not been demonstrated here. Other
selectors listed by aquapx are deliberately refused in this first subset.

## Transactions, persistence and failure behavior

One controller mutex covers the complete fresh GET / validation / patch / byte
audit / no-op comparison / SET sequence. No stale baseline is cached or replayed.
GET and SET must return exactly 809 bytes. Negative, short or oversized transfer
results latch the instance as unavailable: subsequent commands do not retry or
reset the USB device. Invalid reports are refused without any SET. Destruction
closes the handle without restoring or saving settings.

This mutex protects calls through **one controller instance only**. Other handles
to the same device, other OpenRGB instances, aquasuite, aquapx or other software
are not protected. Another process could update pump configuration between GET
and SET, which would then be overwritten despite a perfect byte audit against the
old baseline. Existing `interop/DeviceGuardManager` was examined; cooperative
locking cannot exclude programs that do not honor the same lock. A shared
per-physical-device lock and an external-ownership policy must be decided before
live writes. Do not allow concurrent controlling applications in hardware trials.

No report `0x02` is implemented or transmitted. The documented 11-byte sequence
has ID at 0, `0x02` at offset 4, other body bytes zero, and trailing `34 C6`.
Offline CRC calculation over offsets `[1, 9)` gives `0x34C6`, consistent with
CRC-16/USB in big-endian form. One constant packet does not establish the meanings
of its body, scope, completion or persistence guarantees; those remain unresolved.
Tests use only its numeric body as an offline CRC vector. There is no save mode
or automatic persistence. Whether report `0x03` alone is volatile, applies live,
or causes internal flash writes must be independently confirmed before repeated
hardware use. No high-frequency synchronization capability is advertised.

## References, licensing and provenance

Technical discoveries are attributed to [hirparak/aquapx](https://github.com/hirparak/aquapx),
particularly the locally supplied `re/PROTOCOL.md`, `internal/aqua/d5next.go`,
`internal/aqua/crc.go`, `internal/aqua/lighting.go`, `re/decode.py` and
`re/set_color.py`. Those files were inspected as references, **never executed**.
The project attributes its D5 NEXT work to the captures attached to
[OpenRGB issue 2111](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/2111).
The public issue was checked as a provenance reference; its captures were not
downloaded, redistributed or represented as test data. `aquapx-main/re/` in this
checkout contains no captures or real baseline binary. The reference directory
was not modified or removed.

OpenRGB's root license is GPLv2 and its controller headers use
`GPL-2.0-or-later`. aquapx supplies GPLv3 and describes itself as GPL-3.0; no
additional relicensing permission was identified. The [GNU license FAQ](https://www.gnu.org/licenses/gpl-faq.en.html#v2v3Compatibility)
explains that GPLv2-only and GPLv3 are incompatible, while GPLv2-or-later permits
combination under GPLv3. That does **not** establish upstream permission to import
GPLv3-only code while retaining OpenRGB's existing licensing options.

No aquapx source, translated function, decoder, fixture or license text is imported
into this implementation. It was written for this repository using protocol facts,
the native API and existing CRC++ code. Having inspected aquapx, this work is not
represented as a clean-room implementation or as a legal determination. Attribution
is not a relicensing grant. A human reviewer must verify provenance and upstream
licensing expectations before submission. New source files use the surrounding
OpenRGB SPDX convention. CRC++ keeps its existing BSD-style license and notices.

`CONTRIBUTING.md` also disallows generally AI-generated submissions and requires
the responsible human developer to understand and manually vet assisted code.
This is an assisted local prototype for review, not a claim that upstream will
accept it. No AI authorship/co-authorship commit tags or invented personal author
identity were added.

## Offline tests and results

From the repository root:

```sh
bash tests/AquaComputerD5Next/run.sh
bash tests/AquaComputerD5Next/run.sh --sanitize
bash tests/AquaComputerD5Next/run.sh --sanitize-known-baseline
```

The first command passes **44,201 checks** using GCC 13 / C++17. The report
fixtures are explicitly artificial: a deterministic byte pattern, valid lighting
fields, and CRC computed by an independently written non-reflected bit-stream
oracle. No fixture is claimed to represent a real configuration or safe hydraulic
settings. The CRC check value is also documented in the existing CRC++ header and
the [CRC RevEng catalogue](https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-usb).

Coverage includes exact/truncated/oversized framing, all individual report-byte
corruptions, wrong ID and CRC byte order, null arguments, decode fields, RGB
primaries/white/black/secondary colors/grays, a 4,096-color HSV grid, saturation
and brightness boundaries, hue wrap, isolated brightness changes, untouched input
and failure output, repeated updates, overlapping buffers, all unauthorized byte
positions with recomputed CRC, unknown layout/field values, mocked HID failures,
fresh-baseline preservation, no-op suppression, off and hardware modes, and 200
transactions from two threads without overlap. Adapter tests use native public
synchronous callbacks; no GUI or application discovery loop runs.

Discovery tests verify actual macro registration, exact identity/interface fields,
automatic naming and enumerated paths, wrong interfaces/devices, absent and
inaccessible devices, repeated scan cleanup, disappearance/reappearance, no
GET/SET/save during construction/scan callbacks, and handle closure.

The strict sanitizer command intentionally stops on a **pre-existing UBSan
diagnostic** in `RGBControllerInterface.h:287`: `matrix_map_type::Set(0, 0, NULL)`
calls `memset` on an empty vector's null `data()` with zero length. A standalone
program using only that existing class reproduces the same failure without any
D5 NEXT source. No shared API file was modified for this task.

The explicit `--sanitize-known-baseline` command passes all checks with ASan and
UBSan. It announces and applies `ubsan.supp`, which suppresses nonnull-attribute
diagnostics originating in the existing `RGBControllerInterface.h` only. No new
D5 NEXT source is excluded. This result must not be described as an unsuppressed
sanitizer pass. Other sanitizer findings remain fatal.

All new controller translation units also pass production-mode syntax checks
without the test macro, using `-Wall -Wextra -Wpedantic -Werror`. Dynamic dependency
inspection confirms that the test executable links no Qt, hidapi or libusb
backend. A syscall trace of the offline tests contains no access to hidraw or USB
device nodes and no HID/USB ioctls. Full OpenRGB packaging and Windows/macOS builds
were not performed; the test target compiles the actual native adapter, detector,
RGBController base, and required StringUtils code, rather than launching OpenRGB.

## File inventory

Only new files were added; existing tracked source files were not changed.

| Directory | Files |
| --- | --- |
| `Controllers/AquaComputerD5NextController/` | `D5NextProtocol.h`, `D5NextProtocol.cpp`, `AquaComputerD5NextController.h`, `AquaComputerD5NextController.cpp`, `AquaComputerD5NextControllerDetect.cpp`, `RGBController_AquaComputerD5Next.h`, `RGBController_AquaComputerD5Next.cpp` |
| `tests/AquaComputerD5Next/` | `D5NextTests.cpp`, `D5NextTests.pro`, `OfflineLog.cpp`, `OfflineDetection.h`, `OfflineDetection.cpp`, `run.sh`, `ubsan.supp` |
| `Documentation/` | `AquaComputerD5Next.md` |

Build products, logs and syscall traces are under ignored `.build/`. The
pre-existing untracked `aquapx-main/` remains outside the contribution.

## Future hardware validation, only after explicit authorization

1. Complete human code/provenance review and agree the upstream licensing and
   contribution approach. Obtain lawful, attributable captures with confirmed
   firmware identity, report framing and lighting-group scope. Independently
   compare their CRC and fields offline before enabling any device access.
2. In a dedicated authorized read-only build, enable native discovery separately
   from writes. Confirm VID/PID/interface selection on Linux and Windows; verify
   that only the enumerated interface is opened, no path input/root is required
   with suitable existing permissions, and inaccessible devices do not appear.
   Confirm with capture that Scan Devices sends no SET or save, including profile
   loading callbacks. Test repeated scans, unplug/replug, multiple pumps and
   clean shutdown for duplicate results and leaked handles.
3. Determine a trustworthy firmware/layout discriminator. Add an explicit
   reviewed allowlist; all unknown versions must remain unable to write. Do not
   use report CRC, report length or `release_number` alone as a substitute.
   Identify a safe GET procedure and prove its framing without resets. Adopt
   observed state in the adapter without sending a color or Off command during
   initialization. Keep write enablement separate from scan/profile loading.
4. Establish exclusive device ownership across processes and handles. Record an
   original report under the agreed read-only procedure, preserve provenance and
   privacy, and review every proposed byte delta before the first authorized SET.
   Confirm hydraulic configuration against the original and an independent
   observation method. An unchanged-byte comparison alone cannot prove runtime
   pump behavior or prevent another process's concurrent update from being lost.
5. With writes separately authorized, try one static-color change using report
   `0x03` only; verify exact transfer completion, visual output, firmware state
   and hydraulic operation. Stop on error or unexplained differences; no automatic
   retry, reset, save or restoration sequence. An authorized recovery procedure
   must be reviewed in advance rather than replaying a potentially stale report.
6. Verify red/green/blue, hue scale, grayscale/black, brightness extrema and off;
   then verify Breathing and Rainbow without modifying their retained parameters.
   Determine whether `0x03` is volatile and whether any internal persistence occurs
   without `0x02`. Do not run frequent updates before this is known. Explicit
   persistent saving remains a separate future scope requiring full semantics.
7. Validate full application scan/profile/shutdown behavior and supported OS
   builds. Add supported-device metadata and production detector registration
   only after review of the preceding evidence. Add legally redistributable real
   fixtures and firmware-specific tests when available. The acceptance test is
   that Scan Devices automatically lists `Aqua Computer D5 NEXT`, without any
   SET/save during discovery or changes to pump operation settings.

The codec and simulated integration are ready for local technical review. Native
hardware support, production activation and an upstream-ready device submission
remain pending these validations. The missing firmware approval and unresolved
configuration/persistence semantics are intentional barriers, not features that
can be safely enabled by merely changing a build flag.

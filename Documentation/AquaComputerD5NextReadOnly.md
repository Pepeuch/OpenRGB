# D5 NEXT lot 2: read-only validation report

This report is the historical lot 2 snapshot. Later permission changes made
outside the agent and the subsequent controlled read-only pink preparation are
recorded in [AquaComputerD5NextPinkPreparation.md](AquaComputerD5NextPinkPreparation.md).

Campaign date: 10 November 2026 (Europe/Paris). Branch: `feat/d5next`.
Scope: authorized metadata, descriptors, cached telemetry, and a single
GET_FEATURE only if permissions and safety checks permit it. **The campaign is
closed. No RGB/configuration write has been authorized or performed.**

## Result

The physical D5 NEXT was identified automatically through sysfs as USB
`0C70:F00E`, HID interface `1`, bound to `aquacomputer_d5next`. The cached HID
descriptor agrees with the documented **809-byte framing** for Feature Report
`0x03`, including its report ID. That is a descriptor observation, not an actual
received configuration length or firmware compatibility decision.

The HID node is inaccessible to the current user. The diagnostic therefore did
not open it and sent **zero GET_FEATURE requests**. Actual report ID, CRC and RGB
configuration values could not be validated. There was no fallback, privilege
escalation, permission change, USB reset, SET_FEATURE, Output or save command.

The existing 44,201 offline checks passed before changes. The codec and hardware
controller implementation from lot 1 were retained without edits. Changes are
limited to a separate Linux diagnostic, tests, and an explicitly read-only
adapter/detector option. No complete OpenRGB build was run against hardware.

## Effective identification and permissions

| Observation | Result |
| --- | --- |
| Automatic matching devices | 1 |
| Manufacturer/product | Aqua Computer GmbH & Co. KG / D5 NEXT |
| VID / PID | `0x0C70` / `0xF00E` |
| USB interface / class | `01` / `03` (HID) |
| Kernel driver / hwmon name | `aquacomputer_d5next` / `d5next` |
| USB `bcdDevice` | `0100`; not an identified firmware version |
| Observed node | `/dev/hidraw1`; discovered, never supplied manually |
| Owner / group / mode | `root:root`, `0600` |
| ACL | Owner `rw-`, group `---`, other `---`; no additional ACL |
| Effective user | UID 1000, member of `plugdev`; neither read nor write access |
| debugfs | `/sys/kernel/debug` is `0700 root:root`, inaccessible |

The HID node number and physical USB port are observations, not stable IDs or
configuration requirements. The tool does not print device serial numbers, read
the serial sysfs attribute, or accept a manually configured USB path. An OpenRGB
process was observed during preliminary inspection and was left untouched. The
diagnostic checks for known controller process names before a possible feature
request; it found none in its later snapshot. This is a best-effort check, not
proof that every process or alternate USB handle has been excluded.

The cause of denied HID access is the device node's ownership, mode and ACL,
not failure to locate the pump. No custom matching Aqua Computer udev rule was
identified in the installed rules inspected.

### Proposed rule, not installed

For a future reviewed **read-only diagnostic** grant, a targeted two-stage rule
could be placed in an appropriate late udev rules file:

```udev
ACTION=="add|change", SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0c70", ATTRS{idProduct}=="f00e", ENV{D5NEXT_READONLY_MATCH}="1"
ACTION=="add|change", SUBSYSTEM=="hidraw", ENV{D5NEXT_READONLY_MATCH}=="1", ATTRS{bInterfaceNumber}=="01", GROUP="plugdev", MODE="0440"
```

The user is already in `plugdev`. This proposal grants group read access, not
world access. The two rules intentionally match USB device attributes and USB
interface attributes separately: combining those `ATTRS` checks in one rule
would require them to occur on the same parent device. The proposal has **not**
been installed, loaded, triggered, or claimed to work on a live udev event.
Local ACL/logind policies and other later rules must also be reviewed.

The standalone diagnostic uses `O_RDONLY`. OpenRGB's hidapi hidraw backend
normally needs an `O_RDWR` handle even for a read-only adapter; this read-only
rule does not satisfy that separate backend requirement. A broader permission
grant for the application would require separate review. Neither an existing
`plugdev` membership nor an installed libhidapi library bypasses the present
node mode. No debugfs permission change is proposed or performed here.

## Descriptor observations

The cached `report_descriptor` is 70 bytes. Its SHA-256 is
`dec463e438b54fb8ed4bed987704f7106b45ac0e0a2c05478914feea375efeb2`.
The hash is an observation and must not become a firmware allowlist by itself.
Parsing the HID Report Size, Count and ID globals, including repeated main items,
gives:

| ID | Advertised type | Payload bits | Payload bytes | With report ID |
| --- | --- | ---: | ---: | ---: |
| `0x01` | Input | 1288 | 161 | 162 |
| `0x02` | **Output** | 80 | 10 | 11 |
| `0x03` | Feature | 6464 | 808 | **809** |
| `0x08` | Feature | 6176 | 772 | 773 |
| `0x0C` | Feature | 8192 | 1024 | 1025 |

The `0x03` framing agrees with `aquapx-main/re/PROTOCOL.md` and the existing C++
codec. The descriptor does not identify RGB offsets or CRC semantics.

**Observed discrepancy:** the descriptor advertises `0x02` as an Output report,
whereas aquapx describes its 11-byte save packet as a Feature Report. Some vendor
devices accept reports outside their descriptors, so this observation cannot
prove which transfer type the device accepts or what its persistence semantics
are. Neither form was tested. The current implementation still has no active
save implementation, and no Output/SET probe should be inferred from this result.

## Read results and RGB comparison

One invocation of the separate diagnostic was made with `--get-feature`. It
completed cached metadata inspection and stopped at the pre-open permission
check, with exit status `4` and `Permission denied`.

| Check | Actual result |
| --- | --- |
| Device node opened | No |
| HID identity ioctl / feature ioctl | 0 / 0 |
| Received configuration size | Not available; not zero-byte USB response |
| Returned report ID | Not available |
| Stored / calculated CRC | Not available |
| Brightness 237, flag 239, effect 663 | Not available |
| Hue 706–707, saturation 708 | Not available |
| Firmware/layout compatibility | Unconfirmed; no firmware approved |

Thus no real RGB color or effect can be compared against the existing codec.
The documented CRC coverage `[1,807)`, big-endian checksum, RGB offsets and hue
scale remain assumptions pending a permitted actual read. In particular, the
lot 1 discrepancy between captured approximate primary hues 511/1023 and the
mathematical 1536-step mapping 512/1024 remains unresolved. The conservative
codec still refuses effect selectors outside its initial three-mode subset;
this must not be confused with a device fault if a later actual read uses another
documented hardware effect.

A syscall trace of the diagnostic contains no open of `/dev/hidraw*` or
`/dev/bus/usb/*` and no ioctl calls. Only ordinary sysfs/proc/library files were
opened. The inaccessible debugfs firmware file returned `EACCES`. No USB error
occurred because no USB transfer was attempted; there was no retry campaign.

## Cached hwmon snapshot

The diagnostic reads only `name`, sensor labels and allowlisted `*_input`
attributes below the matched HID device's `hwmon/` tree, once each. It does not
read PWM, temperature offsets, targets or other control attributes. Inspection
of the installed driver's source matters here: some control attribute **reads**
can issue configuration GETs, while D5 NEXT sensor inputs use the cached periodic
Input Report `0x01` data.

| Sensor | Observed value |
| --- | ---: |
| Coolant temperature | 29.50 °C |
| Pump speed | 3865 RPM |
| Pump voltage | 12.100 V |
| Pump current | 0.855 A |
| Pump power | 10.340 W |
| +5 V / +12 V rails | 5.020 V / 12.100 V |
| Fan speed/current/power | 0 RPM / 0 A / 0 W |
| Eight virtual temperature sensors | No data |

These are one cached snapshot, not a stability test, hydraulic safety assessment
or a reference configuration. No sensor, curve, PWM or hydraulic setting was
written. No hidden configuration bytes are included in this report.

## Firmware identification research

The local running driver is `aquacomputer_d5next` on kernel `7.0.0-42-generic`.
The installed module and loaded module expose matching source-version strings.
The examined source is
`/usr/src/aquacomputer_d5next-main-dkms.20250330/aquacomputer_d5next.c`.
It identifies the firmware using a big-endian 16-bit field at offset `0x0D` of
the periodic Input Report `0x01`, then exposes the cached decimal value in a
device-specific debugfs `firmware_version` file. Reading that debugfs value does
not itself request a new Feature Report according to the examined implementation.

This agrees with the [kernel driver's documented debugfs interface](https://docs.kernel.org/hwmon/aquacomputer_d5next.html).
The diagnostic attempted to open that cached firmware file and was denied by
debugfs directory permissions. It did not read an Input Report directly or request
other Feature Reports as a workaround. The sysfs ACPI `firmware_node` link is not
treated as pump firmware information. USB `bcdDevice=0100` is not substituted for
the unavailable firmware value.

A future authorized read could obtain the cached debugfs value through an agreed
access method, or parse one passive Input Report once hidraw is accessible.
Firmware identity alone would still need independent evidence linking that
version to the documented configuration fields. A correct descriptor, length,
CRC or release number cannot establish that compatibility. No such mapping or
allowlist has been added.

## Diagnostic and automatic discovery preparation

Build the standalone Linux diagnostic without starting it:

```sh
bash tools/AquaComputerD5Next/build.sh
```

The generated `.build/d5next-readonly/d5next-readonly` accepts only an optional
`--get-feature` flag; it has no manual device-path or write command. Default
execution inspects cached metadata only. The feature path performs, in order:

1. Automatic sysfs VID/PID/HID-interface matching. More than one matching device
   prevents a feature request instead of choosing one arbitrarily.
2. Node type and effective read-permission checks, before opening.
3. A best-effort known-controller process check and descriptor shape check.
4. `open(O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW)` and a nonblocking
   cooperative `flock` on the handle.
5. File identity and `HIDIOCGRAWINFO` confirmation of USB VID/PID, protecting
   against node replacement between preflight and open.
6. Exactly one `HIDIOCGFEATURE(809)` for `0x03`, then close, validate framing/CRC,
   and summarize only documented fields. Invalid reads leave caller output
   untouched. No raw dump is saved.

`O_NONBLOCK` is not an application-level timeout for a synchronous feature ioctl;
transport completion depends on the kernel driver. There is no retry loop.
The tool contains no SET_FEATURE, Output, save or reset call and links no hidapi,
libusb, Qt or external service. It uses the [documented Linux hidraw GET interface](https://docs.kernel.org/hid/hidraw.html)
and the existing C++ report codec.

The process-name check and `flock` protect only cooperative cases. They do not
exclude hidden processes, a second handle using another backend, kernel control
operations or a client ignoring the lock. They are not sufficient proof of
exclusive control before writes. The permission denial in this campaign stopped
execution earlier than all of these request-path operations.

The native detector now has a separate, **disabled-by-default** compile option:
`OPENRGB_D5NEXT_READ_ONLY_DETECTION`. It uses the existing
`REGISTER_HID_DETECTOR_I` entry for `0C70:F00E`, interface `1`. On Linux it checks
hidapi's read/write opening permissions before calling `hid_open_path`.
It constructs the controller with an unknown layout and an adapter whose sole
mode is **Read only**, with no color, brightness, Direct or save capabilities.
All mode/color/profile callbacks return before controller I/O; shutdown closes
the owned handle without restoring or saving a configuration.

The existing unknown-layout barrier remains an independent safeguard. A test
also constructs the read-only adapter with a *known synthetic layout*, proving
that the adapter's no-I/O behavior does not merely rely on that second barrier.
Neither scan nor construction performs GET_FEATURE; the explicit GET operation
is confined to the separate diagnostic tool.

No main QMake setting enables this option, and no modified full application was
launched. Real Scan Devices integration remains unvalidated because node access
is denied and full-application scans can operate unrelated controllers/load
profiles. The real device therefore has **not** been demonstrated in the OpenRGB
list. The prepared callback can create a read-only RGBController automatically
when enabled in a reviewed build with suitable permissions. Existing scan cleanup
is retained; simulated repeated scans verify one result, proper handle closure,
inaccessible-device skipping and reappearance without a static path cache.

## Tests, data and modified files

```sh
bash tests/AquaComputerD5Next/run.sh
bash tests/AquaComputerD5Next/run.sh --sanitize-known-baseline
bash tests/AquaComputerD5Next/run-readonly.sh
bash tests/AquaComputerD5Next/run-readonly.sh --sanitize
```

Results:

- Native codec/controller/adapter/detector suite: **44,218 checks pass**.
- New diagnostic suite: **8,264 checks pass**, including synthetic descriptor
  parsing, automatic identity/interface discovery, permission refusal before open,
  controller-process detection, read-only flags, locking failure, identity changes,
  exact single GET, transfer/CRC errors and handle closure. Acquisition syscalls
  are replaced at link time, so those tests never reach hardware.
- New diagnostic suite under ASan/UBSan: passes **without suppressions**.
- Native suite under ASan/UBSan: passes with the lot 1 explicit suppression for
  the existing empty-matrix nonnull diagnostic in `RGBControllerInterface.h`.
  This is not an unsuppressed sanitizer pass. Tests avoid depending on the base
  worker's existing startup/notification timing; direct public callbacks and
  shutdown ownership are exercised without changing that unrelated component.
- Strict `-Wall -Wextra -Wpedantic -Werror` build of the Linux diagnostic and
  syntax checks of the native read-only detection option pass.
- Dynamic dependency inspection confirms the diagnostic needs only the standard
  C/C++ runtime libraries. Both scripts pass shell syntax checks.

Local evidence is under ignored `.build/d5next-readonly/`, created with mode
`0700`; the actual hardware summary and syscall trace are mode `0600`. No raw
configuration was read or saved, and no serial number is included in the summary.
No private hardware data was added to test fixtures or tracked source. Tests use
only synthetic buffers/files. The following paths are local evidence, not files
for publication:

- `.build/d5next-readonly/hardware-summary.txt`
- `.build/d5next-readonly/hardware-access.trace`
- `.build/d5next-readonly/tests-sanitized.log`
- `.build/d5next-tests/lot2-baseline.log`, `lot2-final.log`, `lot2-sanitize.log`

Added in lot 2:

- `tools/AquaComputerD5Next/D5NextReadOnly.h`, `D5NextReadOnly.cpp`,
  `D5NextDiagnostic.cpp`, `build.sh`.
- `tests/AquaComputerD5Next/ReadOnlyTests.cpp`, `run-readonly.sh`.
- This report.

Updated from lot 1: the detector, RGBController adapter header/implementation,
native test file, and a historical-snapshot link in the lot 1 report. The report
codec, hardware controller, project file and installed permissions are unchanged.
`aquapx-main/` remains untouched. No commit, push or PR was made.

## Remaining work before any write authorization

1. Review and resolve HID access through a separately approved permission change;
   close competing clients before a controlled read. Do not bypass it with root,
   resets or repeated attempts. The current campaign ends without a real GET.
2. Obtain one valid actual report, validate its received length/ID/CRC, inspect
   RGB fields privately and compare them to the documentation without changing
   configuration. Establish actual firmware identity through a permitted source.
3. Independently verify firmware-to-layout compatibility and RGB group/offset
   semantics. A descriptor hash, USB release or CRC alone is insufficient.
4. Resolve the observed Output-versus-Feature ambiguity for report `0x02` from
   lawful existing captures/documentation; no save probe is authorized.
5. Validate the full application's read-only detection/profile/shutdown lifecycle
   in an isolated reviewed build, including permission failures and repeated scans.
6. Before a separate first-write decision, establish exclusive ownership, recovery
   strategy, exact allowed byte differences, and evidence about volatile versus
   persistent effects of `0x03`. The present cached telemetry and descriptor do
   not validate any write or guarantee unchanged hydraulic operation afterward.

No further hardware commands or background polling are scheduled by this work.

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Validate CP3NV1 panel source and compile its isolated data overlay."""
import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DTS = ROOT / "arch/arm64/boot/dts/sprd"
PANEL = DTS / "lcd/lcd_icnl9922c_mipi_sharp.dtsi"
OVERLAY = DTS / "cp3nv1-icnl9922c-panel-overlay.dts"
INIT_SHA256 = "4801fb3c20c5d27160c4d2e3d1a736d412155b7527e65d3da214d03ba4c1b9c5"
UBOOT_SHA256 = "bf776cfe56116951dbcea4c9096586b22ee40386f1d229ca9458e23bf9f325ac"


def require(ok, message):
    if not ok:
        raise ValueError(message)


def records(blob):
    result = []
    pos = 0
    while pos < len(blob):
        require(len(blob) - pos >= 4, "truncated command header")
        packet_type, delay, length = struct.unpack_from(">BBH", blob, pos)
        require(length > 0, "zero-length command/sentinel in property")
        end = pos + 4 + length
        require(end <= len(blob), "command payload overruns property")
        result.append((packet_type, delay, blob[pos + 4:end]))
        pos = end
    return result


def validate():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--uboot", type=Path, help="optional original U-Boot image")
    args = parser.parse_args()
    source = re.sub(r"/\*.*?\*/|//[^\n]*", "", PANEL.read_text(), flags=re.S)
    match = re.search(r"sprd,initial-command\s*=\s*\[([^]]*)\]", source)
    require(match is not None, "initial command property missing")
    blob = bytes.fromhex(match.group(1))
    commands = records(blob)
    require(len(commands) == 39, "expected 39 initialization commands")
    require(len(blob) == 1166, "unexpected initialization byte count")
    require(hashlib.sha256(blob).hexdigest() == INIT_SHA256,
            "initialization stream differs from recovered U-Boot bytes")
    require(all(r[0] == 0x39 for r in commands), "expected DCS long writes")
    require(commands[0][2] == bytes.fromhex("f0 99 22 0c"), "unlock mismatch")
    require(commands[-3:] == [(0x39, 120, b"\x11\x00\x00"),
                              (0x39, 20, b"\x29\x00\x00"),
                              (0x39, 0, b"\xac\x05\x00")], "wake tail mismatch")
    require("sprd,use-dcs-write;" in source, "DCS transport required")
    require('sprd,dsi-color-format = "dsc";' in source, "DSC transport required")
    expected = {
        "sprd,dsi-lane-number": 4, "sprd,phy-bit-clock": 1100000,
        "sprd,dsc-enable": 1, "sprd,slice-width": 540,
        "sprd,slice-height": 8, "sprd,output-bpc": 8,
        "sprd,esd-check-enable": 0, "clock-frequency": 384000000,
        "hactive": 1080, "vactive": 2408, "hfront-porch": 102,
        "hback-porch": 102, "hsync-len": 4, "vfront-porch": 2520,
        "vback-porch": 32, "vsync-len": 4,
    }
    for name, value in expected.items():
        found = re.search(re.escape(name) + r"\s*=\s*<(\d+)>;", source)
        require(found is not None and int(found.group(1)) == value,
                "unexpected value for " + name)
    require(1080 % 540 == 0 and 2408 % 8 == 0, "invalid DSC slice geometry")
    if args.uboot:
        image = args.uboot.read_bytes()
        require(hashlib.sha256(image).hexdigest() == UBOOT_SHA256,
                "not the analyzed U-Boot image")
        require(image[0x1c7438:0x1c78c6] == blob, "U-Boot byte comparison failed")
        print("PASS: original U-Boot hash and initialization bytes")
    with tempfile.TemporaryDirectory(prefix="cp3nv1-panel-") as tmp:
        preprocessed = Path(tmp) / "panel.dts"
        binary = Path(tmp) / "panel.dtbo"
        text = subprocess.run(["cpp", "-nostdinc", "-undef", "-x",
                               "assembler-with-cpp", "-P", str(OVERLAY)],
                              check=True, capture_output=True).stdout
        preprocessed.write_bytes(text)
        subprocess.run(["dtc", "-@", "-I", "dts", "-O", "dtb", "-o",
                        str(binary), str(preprocessed)], check=True)
        subprocess.run(["dtc", "-I", "dtb", "-O", "dts", "-o",
                        str(Path(tmp) / "roundtrip.dts"), str(binary)], check=True)
        require(binary.stat().st_size > len(blob), "invalid compiled overlay")
    print("PASS: 39 commands, 1166 bytes, exact payload hash and wake delays")
    print("PASS: DSC geometry, native panel properties and dtc round trip")
    print("NOTE: static checks only; bias wiring and DSI VFP remain unverified")


if __name__ == "__main__":
    try:
        validate()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        raise SystemExit("FAIL: " + str(error))

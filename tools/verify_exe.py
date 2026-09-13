#!/usr/bin/env python3
"""Validate the resources in the linked PE, not just the input manifest.

Uses only the Python standard library and runs on Windows and Linux.
"""
from pathlib import Path
import re
import struct
import sys
import xml.etree.ElementTree as ET


def resources(data):
    if data[:2] != b'MZ':
        raise ValueError('missing DOS header')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe+4] != b'PE\0\0':
        raise ValueError('missing PE signature')
    machine, count = struct.unpack_from('<HH', data, pe + 4)
    opt_len = struct.unpack_from('<H', data, pe + 20)[0]
    opt = pe + 24
    if machine != 0x8664 or struct.unpack_from('<H', data, opt)[0] != 0x20b:
        raise ValueError('expected a Windows x64 PE32+ executable')
    if struct.unpack_from('<H', data, opt + 68)[0] != 2:
        raise ValueError('expected Windows GUI subsystem')
    resource_rva, resource_size = struct.unpack_from('<II', data, opt + 112 + 2 * 8)
    if not resource_rva or not resource_size:
        raise ValueError('missing resource directory')

    sections = []
    for index in range(count):
        offset = opt + opt_len + index * 40
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', data, offset + 8)
        sections.append((rva, max(virtual_size, raw_size), raw_offset))

    def address(rva):
        for start, length, raw in sections:
            if start <= rva < start + length:
                return raw + rva - start
        raise ValueError('unmapped resource RVA')

    base = address(resource_rva)
    result = {}

    def visit(relative, path=()):
        if len(path) > 3:
            raise ValueError('invalid resource nesting')
        named, ids = struct.unpack_from('<HH', data, base + relative + 12)
        for index in range(named + ids):
            key, target = struct.unpack_from('<II', data, base + relative + 16 + index * 8)
            if key & 0x80000000:
                raise ValueError('unexpected named OzAmp resource')
            current = path + (key,)
            if target & 0x80000000:
                visit(target & 0x7fffffff, current)
            else:
                rva, length = struct.unpack_from('<II', data, base + target)
                start = address(rva)
                result[current] = data[start:start + length]

    visit(0)
    return result


def verify(path, expected=None):
    data = Path(path).read_bytes()
    if expected is None:
        expected = re.search(r'VER\s*=\s*L"(\d+\.\d+\.\d+)"',
                             (Path(__file__).resolve().parents[1] / 'main.cpp').read_text()).group(1)
    found = resources(data)
    manifests = [value for (kind, rid, lang), value in found.items() if kind == 24 and rid == 1]
    if len(manifests) != 1:
        raise ValueError('exactly one embedded application manifest at RT_MANIFEST/1 is required')
    root = ET.fromstring(manifests[0])
    ns = {'asm': 'urn:schemas-microsoft-com:asm.v1',
          'compat': 'urn:schemas-microsoft-com:compatibility.v1',
          'trust': 'urn:schemas-microsoft-com:asm.v3',
          'dpi': 'http://schemas.microsoft.com/SMI/2016/WindowsSettings'}
    identity = root.find('asm:assemblyIdentity', ns)
    assert identity is not None and identity.get('version') == expected + '.0', 'assembly version mismatch'
    assert identity.get('processorArchitecture') == 'amd64', 'assembly architecture mismatch'
    supported = root.findall('compat:compatibility/compat:application/compat:supportedOS', ns)
    assert any(node.get('Id') == '{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}' for node in supported), 'Windows 10/11 declaration missing'
    level = root.find('trust:trustInfo/trust:security/trust:requestedPrivileges/trust:requestedExecutionLevel', ns)
    assert level is not None and level.get('level') == 'asInvoker' and level.get('uiAccess') == 'false', 'unexpected execution level'
    assert root.find('trust:application/trust:windowsSettings/dpi:dpiAwareness', ns).text == 'system', 'DPI default changed'

    versions = [value for (kind, rid, lang), value in found.items() if kind == 16 and rid == 1]
    assert len(versions) == 1, 'version resource missing'
    info = versions[0]
    length, value_len, value_type = struct.unpack_from('<HHH', info)
    assert length == len(info) and value_len == 52 and value_type == 0, 'invalid version root'
    key_end = 6
    while info[key_end:key_end+2] != b'\0\0':
        key_end += 2
    assert info[6:key_end].decode('utf-16le') == 'VS_VERSION_INFO'
    fixed_offset = (key_end + 2 + 3) & ~3
    fixed = struct.unpack_from('<13I', info, fixed_offset)
    major, minor, patch = map(int, expected.split('.'))
    assert fixed[0] == 0xFEEF04BD
    assert fixed[2:6] == ((major << 16) | minor, patch << 16) * 2, 'file/product version mismatch'
    assert (f'OzAmp-{expected}.exe\0').encode('utf-16le') in info, 'original filename mismatch'
    assert any(kind == 3 for kind, rid, lang in found), 'icon images missing'
    assert any(kind == 14 and rid == 101 for kind, rid, lang in found), 'icon group missing'
    for encoding in ('ascii', 'utf-16le'):
        assert not re.search(r'\bTEST\d+[A-Za-z]?\b', data.decode(encoding, errors='ignore'), re.I), 'development version in linked binary'
    print(f'PASS: Windows x64 PE, embedded Windows 10/11 manifest, asInvoker, preserved DPI, file/product {expected}, icon resources and stable binary labels')


if __name__ == '__main__':
    if len(sys.argv) not in (2, 3):
        raise SystemExit('usage: verify_exe.py OzAmp-X.Y.Z.exe [expected-version]')
    verify(sys.argv[1], sys.argv[2] if len(sys.argv) == 3 else None)

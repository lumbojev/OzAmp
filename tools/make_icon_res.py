#!/usr/bin/env python3
"""Create icon, application manifest and version-info resources for OzAmp.
No third-party modules required. The resulting .res can be passed directly to lld-link.
"""
import re, struct, sys
from pathlib import Path
import xml.etree.ElementTree as ET

def align4(data: bytes) -> bytes:
    return data + b"\0" * ((-len(data)) % 4)

def ordinal(value: int) -> bytes:
    return struct.pack("<HH", 0xFFFF, value)

def record(rtype: int, rname: int, data: bytes, flags=0x1030, lang=0x0409) -> bytes:
    head = struct.pack("<II", len(data), 0) + ordinal(rtype) + ordinal(rname)
    head = align4(head)
    head += struct.pack("<IHHII", 0, flags, lang, 0, 0)
    head = struct.pack("<II", len(data), len(head)) + head[8:]
    return align4(head + data)

def version_block(key, value=b"", children=(), text=True):
    key_bytes = (key + "\0").encode("utf-16le")
    length = len(value) // 2 if text else len(value)
    block = align4(struct.pack("<HHH", 0, length, int(text)) + key_bytes) + value
    for child in children:
        block = align4(block) + child
    return struct.pack("<H", len(block)) + block[2:]

def version_resource(version):
    major, minor, patch = map(int, version.split('.'))
    if any(part > 65535 for part in (major, minor, patch)):
        raise ValueError('version component exceeds Windows resource limit')
    ms, ls = (major << 16) | minor, patch << 16
    fixed = struct.pack('<13I', 0xFEEF04BD, 0x10000, ms, ls, ms, ls,
                        0x3F, 0, 0x40004, 1, 0, 0, 0)
    values = {
        'CompanyName': 'Oskar Lumbojev',
        'FileDescription': 'OzAmp native Windows audio player',
        'FileVersion': version,
        'InternalName': 'OzAmp',
        'LegalCopyright': 'Copyright (c) 2026 Oskar Lumbojev',
        'OriginalFilename': 'OzAmp-' + version + '.exe',
        'ProductName': 'OzAmp',
        'ProductVersion': version,
    }
    table = version_block('040904B0', children=[
        version_block(key, (value + '\0').encode('utf-16le'))
        for key, value in values.items()
    ])
    strings = version_block('StringFileInfo', children=[table])
    translation = version_block('Translation', struct.pack('<HH', 0x0409, 1200), text=False)
    variables = version_block('VarFileInfo', children=[translation])
    return version_block('VS_VERSION_INFO', fixed, [strings, variables], text=False)

def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: make_icon_res.py input.ico output.res app.manifest main.cpp")
    source = Path(sys.argv[4]).read_text(encoding='utf-8-sig')
    match = re.search(r'VER\s*=\s*L"(\d+\.\d+\.\d+)"', source)
    if not match:
        raise SystemExit('main.cpp must specify a plain stable version')
    version = match.group(1)
    manifest = Path(sys.argv[3]).read_bytes()
    root = ET.fromstring(manifest)
    identity = root.find('{urn:schemas-microsoft-com:asm.v1}assemblyIdentity')
    if identity is None or identity.get('version') != version + '.0':
        raise SystemExit('application manifest and main.cpp versions do not match')
    ico = Path(sys.argv[1]).read_bytes()
    reserved, kind, count = struct.unpack_from("<HHH", ico, 0)
    if reserved != 0 or kind != 1 or count < 1:
        raise SystemExit("not a valid Windows icon file")
    entries=[]
    for i in range(count):
        off=6+i*16
        w,h,colors,reserved2,planes,bpp,size,data_off=struct.unpack_from("<BBBBHHII",ico,off)
        entries.append((w,h,colors,reserved2,planes,bpp,size,ico[data_off:data_off+size]))
    result=bytearray(record(0,0,b"",flags=0,lang=0))
    for rid,e in enumerate(entries,1):
        result += record(3,rid,e[7])  # RT_ICON
    group=bytearray(struct.pack("<HHH",0,1,count))
    for rid,e in enumerate(entries,1):
        w,h,colors,reserved2,planes,bpp,size,_=e
        group += struct.pack("<BBBBHHIH",w,h,colors,reserved2,planes,bpp,size,rid)
    result += record(14,101,bytes(group))  # RT_GROUP_ICON / IDI_OZAMP
    result += record(16,1,version_resource(version))  # RT_VERSION
    result += record(24,1,manifest,lang=0)  # RT_MANIFEST / CREATEPROCESS_MANIFEST_RESOURCE_ID
    Path(sys.argv[2]).write_bytes(result)

if __name__ == "__main__":
    main()

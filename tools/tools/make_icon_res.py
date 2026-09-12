#!/usr/bin/env python3
"""Create a Microsoft .res file containing RT_ICON + RT_GROUP_ICON from an .ico.
No third-party modules required. The resulting .res can be passed directly to lld-link.
"""
import struct, sys
from pathlib import Path

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

def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: make_icon_res.py input.ico output.res")
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
    Path(sys.argv[2]).write_bytes(result)

if __name__ == "__main__":
    main()

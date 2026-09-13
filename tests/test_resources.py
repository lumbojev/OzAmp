#!/usr/bin/env python3
"""Build resource fixtures and reject absent/mismatched application metadata."""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


class ApplicationResources(unittest.TestCase):
    def test_manifest_version_and_icon_resources(self):
        with tempfile.TemporaryDirectory(prefix='ozamp-resources-') as directory:
            result = Path(directory) / 'app.res'
            subprocess.run([sys.executable, str(ROOT / 'tools/make_icon_res.py'),
                            str(ROOT / 'ozamp.ico'), str(result),
                            str(ROOT / 'ozamp.manifest'), str(ROOT / 'main.cpp')], check=True)
            data = result.read_bytes()
            entries = {}
            offset = 0
            while offset < len(data):
                length, header = struct.unpack_from('<II', data, offset)
                type_marker, kind, id_marker, rid = struct.unpack_from('<4H', data, offset + 8)
                self.assertEqual((type_marker, id_marker), (65535, 65535))
                entries[kind, rid] = data[offset+header:offset+header+length]
                offset = (offset + header + length + 3) & ~3
            self.assertEqual(entries[24, 1], (ROOT / 'ozamp.manifest').read_bytes())
            xml = ET.fromstring(entries[24, 1])
            version = re.search(r'VER=L"([0-9.]+)"', (ROOT / 'main.cpp').read_text()).group(1)
            self.assertEqual(xml.find('{urn:schemas-microsoft-com:asm.v1}assemblyIdentity').get('version'), version + '.0')
            self.assertIn((14, 101), entries)
            self.assertIn((16, 1), entries)
            self.assertGreater(sum(kind == 3 for kind, rid in entries), 1)

    def test_bad_manifest_never_uses_a_stale_resource(self):
        with tempfile.TemporaryDirectory(prefix='ozamp-resources-') as directory:
            manifest = Path(directory) / 'mismatch.manifest'
            text = (ROOT / 'ozamp.manifest').read_text()
            text = re.sub(r'version="\d+\.\d+\.\d+\.\d+"', 'version="9.8.7.0"', text)
            manifest.write_text(text)
            resource = Path(directory) / 'must-not-exist.res'
            run = subprocess.run([sys.executable, str(ROOT / 'tools/make_icon_res.py'),
                                  str(ROOT / 'ozamp.ico'), str(resource), str(manifest),
                                  str(ROOT / 'main.cpp')], capture_output=True, text=True)
            self.assertNotEqual(run.returncode, 0)
            self.assertIn('versions do not match', run.stderr)
            self.assertFalse(resource.exists())


if __name__ == '__main__':
    unittest.main()

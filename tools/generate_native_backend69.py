"""Generate private native69 C from an owner's ROM and the reviewed SDK.

The committed inputs are addresses/modes and the existing camera patch contract,
not a ROM or generated game code. Unsupported instructions fail generation.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys

PROJECT = Path(__file__).resolve().parents[1]
METADATA = PROJECT / 'config/native69'
TEXT_PIN_POLICY = 'utf8-crlf-to-lf-only-v1'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def text_digest(data):
    """Ignore Git checkout CRLF conversion, preserving every other source byte."""
    data.decode('utf-8')  # Invalid text must fail instead of being repaired.
    return digest(data.replace(b'\r\n', b'\n'))


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8'))


def c_array(text, name):
    match = re.search(r'\b' + re.escape(name) + r'\s*\[[^\]]*\]\s*=\s*\{([^}]+)\}', text, re.S)
    if not match:
        raise ValueError('Missing camera source array: ' + name)
    return bytes(int(v, 0) for v in re.findall(r'0[xX][0-9a-fA-F]+|\b\d+\b', match[1]))


def check_camera_contract(patches, project=PROJECT):
    """Bind metadata to both current patch writer and follower source guards."""
    qol = (project / 'src/lagoon_qol.c').read_text(encoding='utf-8')
    follower = (project / 'src/lagoon_follower.c').read_text(encoding='utf-8')
    changes = re.search(r'\bchanges\[\]\s*=\s*\{(.*?)\n\s*\};', qol, re.S)
    if not changes:
        raise ValueError('Missing camera threshold source contract')
    expected = [dict(rom_offset=0x809d, original=c_array(qol, 'early_call').hex(), runtime='eaeaea'),
                dict(rom_offset=0x80ca, original=c_array(follower, 'stock').hex(), runtime=c_array(follower, 'centered').hex()),
                dict(rom_offset=0x83d8, original=c_array(qol, 'dispatcher_stock').hex(), runtime=c_array(qol, 'dispatcher_center').hex())]
    if c_array(follower, 'dispatcher') != bytes.fromhex(expected[2]['runtime']):
        raise ValueError('Follower and QoL camera dispatchers disagree')
    for row in re.findall(r'\{([^{}]+)\}', changes[1]):
        offset, opcode, stock, center = (int(v.strip(), 0) for v in row.split(','))
        expected.append(dict(rom_offset=offset, original=bytes((opcode, stock, 0)).hex(), runtime=bytes((opcode, center, 0)).hex()))
    if len(expected) != 8 or patches != expected:
        raise ValueError('Native camera profile differs from existing source patch contract')
    # Check the actual writer sites too; array equality alone is insufficient.
    for pattern in (r'memset\(r\+0x809D,0xEA,3\)', r'r\[0x80CB\]=0xD8;\s*r\[0x80CC\]=0x83',
                    r'memcpy\(r\+0x83D8,dispatcher_center,sizeof\(dispatcher_center\)\)',
                    r'r\[changes\[j\]\.offset\+1\]=changes\[j\]\.center'):
        if not re.search(pattern, qol):
            raise ValueError('Camera patch writer changed; review native profile')


def checked_inputs(rom_path, sdk, metadata=METADATA):
    manifest = read_json(metadata / 'manifest.json')
    if manifest.get('schema') != 1 or manifest.get('abi') != 'native-blocks69':
        raise ValueError('Unknown native69 metadata schema/ABI')
    if manifest.get('text_pin_policy') != TEXT_PIN_POLICY:
        raise ValueError('Unknown native69 source text normalization policy')
    rom = rom_path.read_bytes()
    if len(rom) != manifest['original_rom_size'] or digest(rom) != manifest['original_rom_sha256']:
        raise ValueError('ROM is not the reviewed unmodified Lagoon USA image')
    for filename, expected in manifest['metadata_files'].items():
        if text_digest((metadata / filename).read_bytes()) != expected:
            raise ValueError('Native metadata hash differs: ' + filename)
    for filename, expected in manifest['sdk_files'].items():
        if text_digest((sdk / filename).read_bytes()) != expected:
            raise ValueError('SDK native compiler/ABI changed; review before regenerating: ' + filename)
    profiles = []
    for profile in manifest['profiles']:
        keys = read_json(metadata / profile['keys'])
        if len(keys) != profile['count'] or len({tuple(k) for k in keys}) != len(keys):
            raise ValueError('Wrong or duplicate key count')
        for key in keys:
            if (not isinstance(key, list) or len(key) != 4 or any(type(v) is not int for v in key)
                    or not 0 <= key[0] <= 0xffffff or any(v not in (0, 1) for v in key[1:])
                    or key[3] and key[1:3] != [1, 1]):
                raise ValueError('Invalid architectural PC/M/X/E key')
        patches = read_json(metadata / profile['patches']) if profile['patches'] else []
        profiles.append((profile, keys, patches))
    check_camera_contract(profiles[1][2])
    return manifest, rom, profiles


def generate(rom_path, sdk, outdir, metadata=METADATA):
    manifest, rom, profiles = checked_inputs(rom_path, sdk, metadata)
    # Import exactly the selected, hash-checked SDK. No SDK bootstrap or download.
    recompiler = sdk / 'recompiler'
    sys.path.insert(0, str(recompiler))
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('native69_build_emitter', recompiler / 'v2/emit_native_blocks.py')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    generated = []
    for profile, keys, patches in profiles:
        text = module.emit_native_blocks(rom, symbol=profile['symbol'], profile=profile['id'],
                                         keys=keys, include_reset=True, patches=patches)
        generated.append((profile, text))
    # Both profiles must compile successfully before publishing either output.
    outdir.mkdir(parents=True, exist_ok=True)
    outputs = []
    for profile, text in generated:
        output = outdir / profile['output']
        if not output.exists() or output.read_text(encoding='utf-8') != text:
            output.write_text(text, encoding='utf-8')
        outputs.append(dict(file=output.name, profile=profile['id'], keys=profile['count'],
                            sha256=digest(output.read_bytes()), canonical_lf_sha256=digest(text.encode('utf-8'))))
    receipt = dict(schema=1, original_rom_sha256=manifest['original_rom_sha256'],
                   text_pin_policy=TEXT_PIN_POLICY,
                   metadata_manifest_sha256=text_digest((metadata / 'manifest.json').read_bytes()),
                   sdk_files=manifest['sdk_files'], outputs=outputs, scope=manifest['scope'])
    (outdir / 'generation69.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--sdk', required=True, type=Path)
    parser.add_argument('--out-dir', required=True, type=Path)
    args = parser.parse_args()
    try:
        receipt = generate(args.rom.resolve(), args.sdk.resolve(), args.out_dir.resolve())
    except (ValueError, OSError, ImportError) as error:
        parser.exit(1, 'Native69 generation refused: ' + str(error) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()

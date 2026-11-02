"""Rebuild the immutable ASCII UI atlas. Requires Pillow; never needed to play.

Original Atkinson Hyperlegible Next font and OFL are retained beside this script.
The generated glyph subset remains under the SIL Open Font License 1.1.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import hashlib

ROOT = Path(__file__).resolve().parent
FONT = ROOT / 'AtkinsonHyperlegibleNext.ttf'
DEST = ROOT.parents[2] / 'src' / 'lagoon_ui_font.generated.h'

def atlas(name, size, width, height, baseline):
    font = ImageFont.truetype(str(FONT), size)
    font.set_variation_by_axes([500])
    advances = []
    rows = []
    for code in range(32, 127):
        c = chr(code)
        image = Image.new('L', (width, height))
        ImageDraw.Draw(image).text((2, baseline), c, font=font, fill=255, anchor='ls')
        advances.append(max(1, round(font.getlength(c))))
        pixels = image.tobytes()
        packed = []
        for y in range(height):
            for x in range(0, width, 2):
                a = (pixels[y * width + x] + 8) // 17
                b = (pixels[y * width + x + 1] + 8) // 17
                packed.append((a << 4) | b)
        rows.append(' {' + ','.join(str(v) for v in packed) + '}')
    return (f'static const unsigned char {name}_advance[95]={{' + ','.join(map(str, advances)) + '};\n' +
            f'static const unsigned char {name}_alpha[95][{width * height // 2}]={{\n' + ',\n'.join(rows) + '\n};\n')

if __name__ == '__main__':
    # Exact source hash is also recorded in provenance.json.
    digest = hashlib.sha256(FONT.read_bytes()).hexdigest()
    import json
    record = json.loads((ROOT / 'provenance.json').read_text())
    assert digest == record['files'][FONT.name]['sha256']
    notice = record['files']['OFL.txt']
    assert hashlib.sha256((ROOT / 'OFL.txt').read_bytes().replace(b'\r\n', b'\n')).hexdigest() == notice.get('canonical_sha256', notice['sha256'])
    text = ('/* Generated ASCII glyph subset of Atkinson Hyperlegible Next.\n'
            ' * SIL Open Font License 1.1; see assets/fonts/atkinson-hyperlegible/OFL.txt.\n'
            ' * Rebuild with the adjacent generate.py; no platform font/runtime dependency. */\n')
    text += atlas('ui_body', 14, 20, 20, 14)
    text += atlas('ui_small', 9, 16, 12, 8)
    DEST.write_text(text, encoding='utf-8')
    record['text_hash_policy'] = 'utf8-crlf-to-lf-only-v1'
    record['files']['OFL.txt']['canonical_sha256'] = hashlib.sha256((ROOT / 'OFL.txt').read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    record['generated_header_sha256'] = hashlib.sha256(DEST.read_bytes()).hexdigest()
    record['canonical_generated_header_sha256'] = hashlib.sha256(DEST.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
    (ROOT / 'provenance.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    print(DEST)

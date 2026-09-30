#!/usr/bin/env python3
"""Strict original pixel-art gates; validation only, no resizing/quantization."""
import argparse
import json
from pathlib import Path

LABEL = 'fan-made, AI-generated replacement sprite art created for this project; not from the game; characters © Square Enix'

def check_pixels(width, height, rgba, grid, palette_min=16):
    if (width, height) != (grid[0]*2, grid[1]*2):
        raise ValueError(f'expected EXACT 2x grid {grid[0]*2}x{grid[1]*2}; got {width}x{height}')
    if len(rgba) != width*height*4:
        raise ValueError('bad RGBA byte count')
    colors = set()
    opaque = 0
    for i in range(0, len(rgba), 4):
        r,g,b,a = rgba[i:i+4]
        if a not in (0,255):
            raise ValueError('nonbinary alpha / antialiasing')
        if a:
            opaque += 1
            colors.add((r,g,b))
    if not opaque:
        raise ValueError('empty art')
    if not palette_min <= len(colors) <= 32:
        raise ValueError(f'expected {palette_min}..32 opaque colors; got {len(colors)}')
    return dict(width=width,height=height,opaque_pixels=opaque,opaque_colors=len(colors),binary_alpha=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest',type=Path)
    args=parser.parse_args()
    data=json.loads(args.manifest.read_text())
    if data.get('label') != LABEL or data.get('kind') != 'original_pixel_parts':
        raise ValueError('explicit original pixel-part provenance required')
    from PIL import Image
    results=[]
    aggregate_colors=set()
    for part in data['parts']:
        path=args.manifest.parent/part['png']
        if path.suffix.lower()!='.png': raise ValueError('PNG required')
        with Image.open(path) as im:
            if im.mode!='RGBA': raise ValueError('RGBA PNG required')
            rgba=im.tobytes()
            result=check_pixels(*im.size,rgba,part['retail_texel_grid'],palette_min=1)
            aggregate_colors.update(tuple(rgba[i:i+3]) for i in range(0,len(rgba),4) if rgba[i+3])
        results.append(dict(png=part['png'],**result))
    if not 16 <= len(aggregate_colors) <= 32:
        raise ValueError(f'pack palette must contain16..32 colors; got {len(aggregate_colors)}')
    print(json.dumps(dict(mechanical_gates='PASS',parts=results,pack_opaque_colors=len(aggregate_colors),
                         pending=['Visual hard 1px outlines/no gradients',
                                  '16 pose identity and phase proof',
                                  'Renderer strict original footprint/feet probe']),indent=2))

if __name__=='__main__': main()

import importlib.util
from pathlib import Path
import unittest
spec=importlib.util.spec_from_file_location('pixel_parts',Path(__file__).parents[1]/'tools/validate_hd2d_pixel_parts.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class PixelGates(unittest.TestCase):
    def setUp(self):
        self.rgba=bytes(sum(([i*8,0,0,255] for i in range(16)),[]))
    def test_exact_grid_and_palette(self):
        self.assertEqual(m.check_pixels(4,4,self.rgba,[2,2])['opaque_colors'],16)
    def test_wrong_size(self):
        with self.assertRaisesRegex(ValueError,'EXACT'):m.check_pixels(4,4,self.rgba,[3,2])
    def test_antialias_rejected(self):
        with self.assertRaisesRegex(ValueError,'nonbinary'):m.check_pixels(4,4,self.rgba[:-1]+b'\x80',[2,2])
    def test_painted_palette_rejected(self):
        rgba=bytes(sum(([i,0,0,255] for i in range(36)),[]))
        with self.assertRaisesRegex(ValueError,'colors'):m.check_pixels(6,6,rgba,[3,3])
    def test_empty_and_malformed(self):
        for raw in (b'\x00'*64,self.rgba[:-1]):
            with self.assertRaises(ValueError):m.check_pixels(4,4,raw,[2,2])
if __name__=='__main__':unittest.main()

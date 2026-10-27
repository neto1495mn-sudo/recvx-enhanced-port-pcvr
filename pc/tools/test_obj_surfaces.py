"""Synthetic-only checks; no proprietary fixtures."""
import tempfile
from pathlib import Path
import unittest
from analyze_obj_surfaces import read_obj, measure, draft_mapping, opacity_materials, scratch_path


class SurfaceTests(unittest.TestCase):
    def test_negative_indices_and_fan(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)/'mesh.obj'
            p.write_text('g shell#one\nv 0 0 0\nv 2 0 0\nv 2 0 2\nv 0 0 2\nusemtl floor\nf -4 -3 -2 -1\n')
            faces = read_obj(p)
            self.assertEqual(len(faces), 2)
            self.assertEqual(faces[0][0], 'shell#one')
            report = measure(faces, 10)
            self.assertEqual(report['materials']['floor']['area'], 4)
            self.assertEqual(report['materials']['floor']['role_area_fraction']['ground'], 1)

    def test_area_weighted_roles_and_degenerate(self):
        faces = [('shell', 'mixed', ((0,0,0), (10,0,0), (0,0,10))),
                 ('shell', 'mixed', ((0,0,0), (1,0,0), (0,1,0))),
                 ('shell', 'mixed', ((0,0,0),)*3)]
        report = measure(faces, 10)
        self.assertEqual(report['degenerate_triangles'], 1)
        self.assertAlmostEqual(report['materials']['mixed']['orientation_area_fraction']['horizontal'], 50/50.5)
        self.assertEqual(draft_mapping(report, set())['materials']['mixed']['class'], 'ground')
        entry = draft_mapping(report, {'mixed'})['materials']['mixed']
        self.assertTrue(entry['review_required'])
        self.assertIsNone(entry['retail_texture'])

    def test_roof_and_wall_context(self):
        report = measure([
            ('house', 'wall', ((0,0,0), (10,0,0), (0,10,0))),
            ('house', 'roof', ((0,10,0), (10,10,0), (0,15,10)))], 100)
        self.assertEqual(report['materials']['wall']['role_area_fraction']['wall'], 1)
        self.assertEqual(report['materials']['roof']['role_area_fraction']['roof'], 1)
        self.assertEqual(report['materials']['roof']['local_horizontal_anchor_coverage'], 0)

    def test_opacity_and_output_confinement(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)/'mesh.mtl'
            p.write_text('newmtl leaves\nmap_d alpha.tga\nnewmtl glass\nd 0.5\nnewmtl solid\nd 1\n')
            self.assertEqual(opacity_materials(p), {'leaves', 'glass'})
            with self.assertRaises(ValueError):
                scratch_path(Path(d)/'report.json')


if __name__ == '__main__':
    unittest.main()

import unittest
import datetime as dt

from opm.io.parser import Parser
from opm.io.ecl_state import EclipseState
from opm.io.schedule import Schedule


class TestSchedule(unittest.TestCase):


    @classmethod
    def setUpClass(cls):
        deck  = Parser().parse('tests/spe3/SPE3CASE1.DATA')
        state = EclipseState(deck)
        cls.sch = Schedule( deck, state )

    def testWells(self):
        self.assertTrue( isinstance( self.sch.get_wells(0), list) )
        self.assertEqual(2, len(self.sch.get_wells(0)))

        with self.assertRaises(KeyError):
            self.sch.get_well('foo', 0)

    def testContains(self):
        self.assertTrue('PROD' in self.sch)
        self.assertTrue('INJ'  in self.sch)
        self.assertTrue('NOT'  not in self.sch)
        self.assertFalse('NOT' in self.sch)

    def testStartEnd(self):
        self.assertEqual(dt.datetime(2015, 1, 1),   self.sch.start)
        self.assertEqual(dt.datetime(2029, 12, 28), self.sch.end)

    def testTimesteps(self):
        timesteps = self.sch.timesteps
        self.assertEqual(176, len(timesteps))
        self.assertEqual(dt.datetime(2016, 1, 1), timesteps[7])

    def testGroups(self):

        G1 = self.sch.group( 'G1', 0 )
        self.assertTrue(G1.name == 'G1')
        self.assertTrue(G1.num_wells == 2)

        names = G1.well_names

        self.assertEqual(2, len(names))

        self.assertTrue(self.sch.get_well('INJ', 0).isinjector())
        self.assertTrue(self.sch.get_well('PROD', 0).isproducer())

        with self.assertRaises(ValueError):
            self.sch.group('foo', 0)


if __name__ == "__main__":
    unittest.main()


from tests.utils.runtest import makesuite, run
from tests.utils.testcase import TestCase

from tools.utils.dllreader import DllReader


class DllReaderTest(TestCase):

    def testInit(self):
        sm = DllReader(self.testDataBuildDir + '/exportsymbols.dll')
        self.assertSequenceEqual(sm.functions, ['Func', 'Funk', 'Jazz'])
        self.assertSequenceEqual(sm.data, ['Alphabetised', 'AnotherExportedSymbol', 'ExportedSymbol'])



suite = makesuite(
    DllReaderTest,
)

if __name__ == '__main__':
    run(suite)

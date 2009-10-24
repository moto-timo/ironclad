
import os
import shutil
import tempfile

from tests.utils.process import spawn
from tests.utils.runtest import automakesuite, run
from tests.utils.testcase import TestCase

from tools.utils import popen, read_interesting_lines

def GetPexportsLines(path):
    stream = popen("pexports", path.replace('/cygdrive/c', 'c:'))
    try:
        return set(map(lambda s: s.strip(), stream.readlines()))
    finally:
        stream.close()


class BuildStubTest(TestCase):

    def testBuildStubWithBadParams(self):
        retVal = spawn("ipy", "tools/buildstub.py")
        self.assertEquals(retVal, 1, "buildstub didn't bail for 0 param")
        retVal = spawn("ipy", "tools/buildstub.py", "one")
        self.assertEquals(retVal, 1, "buildstub didn't bail for 1 param")
        retVal = spawn("ipy", "tools/buildstub.py", "one", "two", "three", "four")
        self.assertEquals(retVal, 1, "buildstub didn't bail for 4 param")


    def testBuildStubCreatesOutputFiles(self):
        inputPath = "tests/data/exportsymbols.dll"
        overridePath = "tests/data/stub"
        tempDir = tempfile.mkdtemp()
        ourTempDir = os.path.join(tempDir, 'buildstubtest')

        def testGenerates(*extraArgs):
            if os.path.exists(ourTempDir):
                shutil.rmtree(ourTempDir)

            retVal = spawn("ipy", "tools/buildstub.py", *extraArgs)
            self.assertEquals(retVal, 0, "process ended badly")
            cPath = os.path.join(ourTempDir, "stubinit.generated.c")
            self.assertTrue(os.path.exists(cPath))
            asmPath = os.path.join(ourTempDir, "jumps.generated.asm")
            self.assertTrue(os.path.exists(asmPath))
            headerPath = os.path.join(ourTempDir, 'Include', '_mgd_function_prototypes.generated.h')
            self.assertTrue(os.path.exists(headerPath))

        testGenerates(inputPath, ourTempDir)
        testGenerates(inputPath, ourTempDir, overridePath)
        shutil.rmtree(tempDir)

class PythonStubTest(TestCase):

    def testPythonStub(self):
        path = os.path.join('tests', 'data', 'python26-pexports')
        python26exports = set(read_interesting_lines(path))
        generatedExports = GetPexportsLines("build/ironclad/python26.dll")
        self.assertEquals(python26exports - generatedExports, set())

suite = automakesuite(locals())
if __name__ == '__main__':
    run(suite)

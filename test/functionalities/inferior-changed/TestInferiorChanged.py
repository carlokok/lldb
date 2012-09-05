"""Test lldb reloads the inferior after it was changed during the session."""

import os, time
import unittest2
import lldb
from lldbtest import *

class ChangedInferiorTestCase(TestBase):

    mydir = os.path.join("functionalities", "inferior-changed")

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    def test_inferior_crashing_dsym(self):
        """Test lldb reloads the inferior after it was changed during the session."""
        self.buildDsym()
        self.inferior_crashing()
        self.cleanup()
        d = {'C_SOURCES': 'main2.c'}
        self.buildDsym(dictionary=d)
        self.setTearDownCleanup(dictionary=d)
        self.inferior_not_crashing()

    def test_inferior_crashing_dwarf(self):
        """Test lldb reloads the inferior after it was changed during the session."""
        self.buildDwarf()
        self.inferior_crashing()
        self.cleanup()
        d = {'C_SOURCES': 'main2.c'}
        self.buildDwarf(dictionary=d)
        self.setTearDownCleanup(dictionary=d)
        self.inferior_not_crashing()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number of the crash.
        self.line1 = line_number('main.c', '// Crash here.')
        self.line2 = line_number('main2.c', '// Not crash here.')

    def inferior_crashing(self):
        """Inferior crashes upon launching; lldb should catch the event and stop."""
        exe = os.path.join(os.getcwd(), "a.out")
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be a bad access exception.
        self.expect("thread list", STOPPED_DUE_TO_EXC_BAD_ACCESS,
            substrs = ['stopped',
                       'stop reason = EXC_BAD_ACCESS'])

        # And it should report the correct line number.
        self.expect("thread backtrace all",
            substrs = ['stop reason = EXC_BAD_ACCESS',
                       'main.c:%d' % self.line1])

    def inferior_not_crashing(self):
        """Test lldb reloads the inferior after it was changed during the session."""
        self.runCmd("process kill")
        self.runCmd("run", RUN_SUCCEEDED)
        self.runCmd("process status")

        if 'EXC_BAD_ACCESS' in self.res.GetOutput():
            self.fail("Inferior changed, but lldb did not perform a reload")

        # Break inside the main.
        self.expect("breakpoint set -f main2.c -l %d" % self.line2,
                    BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 1: file ='main2.c', line = %d, locations = 1" %
                        self.line2)

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['stopped',
                       'stop reason = breakpoint'])

        self.runCmd("frame variable int_ptr")
        self.expect("frame variable *int_ptr",
            substrs = ['= 7'])
        self.expect("expression *int_ptr",
            substrs = ['= 7'])


if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()

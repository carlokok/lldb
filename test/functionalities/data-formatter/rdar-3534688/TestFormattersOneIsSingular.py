"""
Test lldb data formatter subsystem.
"""

import os, time
import unittest2
import lldb
from lldbtest import *
import datetime

class DataFormatterOneIsSingularTestCase(TestBase):

    mydir = os.path.join("functionalities", "data-formatter", "rdar-3534688")

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @dsym_test
    def test_one_is_singular_with_dsym_and_run_command(self):
        """Test that 1 item is not as reported as 1 items."""
        self.buildDsym()
        self.oneness_data_formatter_commands()

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @dwarf_test
    def test_one_is_singular_with_dwarf_and_run_command(self):
        """Test that 1 item is not as reported as 1 items."""
        self.buildDwarf()
        self.oneness_data_formatter_commands()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break at.
        self.line = line_number('main.m', '// Set break point at this line.')

    def oneness_data_formatter_commands(self):
        """Test that 1 item is not as reported as 1 items."""
        self.runCmd("file a.out", CURRENT_EXECUTABLE_SET)

        self.expect("breakpoint set -f main.m -l %d" % self.line,
                    BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 1: file ='main.m', line = %d, locations = 1" %
                        self.line)

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['stopped',
                       'stop reason = breakpoint'])

        # This is the function to remove the custom formats in order to have a
        # clean slate for the next test case.
        def cleanup():
            self.runCmd('type format clear', check=False)
            self.runCmd('type summary clear', check=False)
            self.runCmd('type synth clear', check=False)

        # Execute the cleanup function during test case tear down.
        self.addTearDownHook(cleanup)

        # Now check that we are displaying Cocoa classes correctly
        self.expect('frame variable key',
                    substrs = ['@"1 object"'])
        self.expect('frame variable key', matching=False,
                    substrs = ['1 objects'])
        self.expect('frame variable value',
                    substrs = ['@"1 object"'])
        self.expect('frame variable value', matching=False,
                    substrs = ['1 objects'])
        self.expect('frame variable dict',
                    substrs = ['1 key/value pair'])
        self.expect('frame variable dict', matching=False,
                    substrs = ['1 key/value pairs'])
        self.expect('frame variable mutable_bag_ref',
                    substrs = ['@"1 value"'])
        self.expect('frame variable mutable_bag_ref', matching=False,
                    substrs = ['1 values'])
        self.expect('frame variable nscounted_set',
                    substrs = ['1 object'])
        self.expect('frame variable nscounted_set', matching=False,
                    substrs = ['1 objects'])
        self.expect('frame variable imset',
                    substrs = ['1 object'])
        self.expect('frame variable imset', matching=False,
                    substrs = ['1 objects'])
        self.expect('frame variable binheap_ref',
                    substrs = ['@"1 item"'])
        self.expect('frame variable binheap_ref', matching=False,
                    substrs = ['1 items'])
        self.expect('frame variable nsset',
                    substrs = ['1 object'])
        self.expect('frame variable nsset', matching=False,
                    substrs = ['1 objects'])
        self.expect('frame variable immutableData',
                    substrs = ['1 byte'])
        self.expect('frame variable immutableData', matching=False,
                    substrs = ['1 bytes'])


if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()

"""
Test that we do not attempt to make a dynamic type for a 'const char*'
"""

import os, time
import unittest2
import lldb
from lldbtest import *

@unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
class Rdar10967107TestCase(TestBase):

    mydir = os.path.join("lang", "objc", "rdar-11355592")

    @dsym_test
    def test_charstar_dyntype_with_dsym(self):
        """Test that we do not attempt to make a dynamic type for a 'const char*'"""
        d = {'EXE': self.exe_name}
        self.buildDsym(dictionary=d)
        self.setTearDownCleanup(dictionary=d)
        self.charstar_dyntype(self.exe_name)

    @dwarf_test
    def test_charstar_dyntype_with_dwarf(self):
        """Test that we do not attempt to make a dynamic type for a 'const char*'"""
        d = {'EXE': self.exe_name}
        self.buildDwarf(dictionary=d)
        self.setTearDownCleanup(dictionary=d)
        self.charstar_dyntype(self.exe_name)

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # We'll use the test method name as the exe_name.
        self.exe_name = self.testMethodName
        # Find the line number to break inside main().
        self.main_source = "main.m"
        self.line = line_number(self.main_source, '// Set breakpoint here.')

    def charstar_dyntype(self, exe_name):
        """Test that we do not attempt to make a dynamic type for a 'const char*'"""
        exe = os.path.join(os.getcwd(), exe_name)
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        self.expect("breakpoint set -f %s -l %d" % (self.main_source, self.line),
                    BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 1: file ='%s', line = %d, locations = 1" %
                        (self.main_source, self.line))

        self.runCmd("run", RUN_SUCCEEDED)
        # check that we correctly see the const char*, even with dynamic types on
        self.expect("frame variable my_string", substrs = ['const char *'])
        self.expect("frame variable my_string -d run-target", substrs = ['const char *'])
        # check that expr also gets it right
        self.expect("expr my_string", substrs = ['const char *'])
        self.expect("expr -d true -- my_string", substrs = ['const char *'])
        # but check that we get the real Foolie as such
        self.expect("frame variable my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("frame variable my_foolie -d run-target", substrs = ['FoolMeOnce *'])
        # check that expr also gets it right
        self.expect("expr my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("expr -d true -- my_foolie", substrs = ['FoolMeOnce *'])
        # now check that assigning a true string does not break anything
        self.runCmd("next")
        # check that we correctly see the const char*, even with dynamic types on
        self.expect("frame variable my_string", substrs = ['const char *'])
        self.expect("frame variable my_string -d run-target", substrs = ['const char *'])
        # check that expr also gets it right
        self.expect("expr my_string", substrs = ['const char *'])
        self.expect("expr -d true -- my_string", substrs = ['const char *'])
        # but check that we get the real Foolie as such
        self.expect("frame variable my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("frame variable my_foolie -d run-target", substrs = ['FoolMeOnce *'])
        # check that expr also gets it right
        self.expect("expr my_foolie", substrs = ['FoolMeOnce *'])
        self.expect("expr -d true -- my_foolie", substrs = ['FoolMeOnce *'])

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()

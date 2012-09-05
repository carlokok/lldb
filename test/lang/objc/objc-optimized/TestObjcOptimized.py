"""
Test that objective-c expression parser continues to work for optimized build.

http://llvm.org/viewvc/llvm-project?rev=126973&view=rev
Fixed a bug in the expression parser where the 'this'
or 'self' variable was not properly read if the compiler
optimized it into a register.
"""

import os, time
import unittest2
import lldb
from lldbtest import *

# rdar://problem/9087739
# test failure: objc_optimized does not work for "-C clang -A i386"
@unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
class ObjcOptimizedTestCase(TestBase):

    mydir = os.path.join("lang", "objc", "objc-optimized")
    myclass = "MyClass"
    mymethod = "description"
    method_spec = "-[%s %s]" % (myclass, mymethod)

    @dsym_test
    def test_break_with_dsym(self):
        """Test 'expr member' continues to work for optimized build."""
        self.buildDsym()
        self.objc_optimized()

    @dwarf_test
    def test_break_with_dwarf(self):
        """Test 'expr member' continues to work for optimized build."""
        self.buildDwarf()
        self.objc_optimized()

    def objc_optimized(self):
        """Test 'expr member' continues to work for optimized build."""
        exe = os.path.join(os.getcwd(), "a.out")
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        self.expect("breakpoint set -n '%s'" % self.method_spec,
                    BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 1: name = '%s', locations = 1" % self.method_spec)

        self.runCmd("run", RUN_SUCCEEDED)
        self.expect("thread backtrace", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ["stop reason = breakpoint"],
            patterns = ["frame.*0:.*%s %s" % (self.myclass, self.mymethod)])

        self.expect('expression member',
            startstr = "(int) $0 = 5")

        self.expect('expression self',
            startstr = "(%s *) $1 = " % self.myclass)

        self.expect('expression self->non_member', error=True,
            substrs = ["does not have a member named 'non_member'"])

        
if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()

"""Test variable with function ptr type and that break on the function works."""

import os, time
import unittest2
import lldb
from lldbtest import *

class FunctionTypesTestCase(TestBase):

    mydir = os.path.join("lang", "c", "function_types")

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @dsym_test
    def test_with_dsym(self):
        """Test 'callback' has function ptr type, then break on the function."""
        self.buildDsym()
        self.function_types()

    @dwarf_test
    def test_with_dwarf(self):
        """Test 'callback' has function ptr type, then break on the function."""
        self.buildDwarf()
        self.function_types()
    
    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @dsym_test
    def test_pointers_with_dsym(self):
        """Test that a function pointer to 'printf' works and can be called."""
        self.buildDsym()
        self.function_pointers()
    
    @dwarf_test
    def test_pointers_with_dwarf(self):
        """Test that a function pointer to 'printf' works and can be called."""
        self.buildDwarf()
        self.function_pointers()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break inside main().
        self.line = line_number('main.c', '// Set break point at this line.')

    def runToBreakpoint(self):
        exe = os.path.join(os.getcwd(), "a.out")
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)
        
        # Break inside the main.
        self.expect("breakpoint set -f main.c -l %d" % self.line,
                    BREAKPOINT_CREATED,
                    startstr = "Breakpoint created: 1: file ='main.c', line = %d, locations = 1" %
                    self.line)
        
        self.runCmd("run", RUN_SUCCEEDED)
        
        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
                    substrs = ['stopped',
                               'stop reason = breakpoint'])
        
        # The breakpoint should have a hit count of 1.
        self.expect("breakpoint list -f", BREAKPOINT_HIT_ONCE,
                    substrs = [' resolved, hit count = 1'])
    
    def function_types(self):
        """Test 'callback' has function ptr type, then break on the function."""
        
        self.runToBreakpoint()

        # Check that the 'callback' variable display properly.
        self.expect("frame variable -T callback", VARIABLES_DISPLAYED_CORRECTLY,
            startstr = '(int (*)(const char *)) callback =')

        # And that we can break on the callback function.
        self.runCmd("breakpoint set -n string_not_empty", BREAKPOINT_CREATED)
        self.runCmd("continue")

        # Check that we do indeed stop on the string_not_empty function.
        self.expect("process status", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['a.out`string_not_empty',
                       'stop reason = breakpoint'])

    def function_pointers(self):
        """Test that a function pointer to 'printf' works and can be called."""
        
        self.runToBreakpoint()

        self.expect("expr string_not_empty",
                    substrs = ['(int (*)(const char *)) $0 = ', '(a.out`'])

        if sys.platform.startswith("darwin"):
            regexps = ['lib.*\.dylib`printf']
        else:
            regexps = ['printf']
        self.expect("expr (int (*)(const char*, ...))printf",
                    substrs = ['(int (*)(const char *, ...)) $1 = '],
                    patterns = regexps)

        self.expect("expr $1(\"Hello world\\n\")",
                    startstr = '(int) $2 = 12')

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()

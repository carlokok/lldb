"""
Tests that C strings work as expected in expressions
"""

from lldbtest import *

class CStringsTestCase(TestBase):
    
    mydir = os.path.join("lang", "c", "strings")
    
    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @dsym_test
    def test_with_dsym_and_run_command(self):
        """Tests that C strings work as expected in expressions"""
        self.buildDsym()
        self.static_method_commands()

    @dwarf_test
    def test_with_dwarf_and_run_command(self):
        """Tests that C strings work as expected in expressions"""
        self.buildDwarf()
        self.static_method_commands()

    def setUp(self):
        TestBase.setUp(self)
    
    def set_breakpoint(self, line):
        self.expect("breakpoint set -f main.c -l %d" % line,
                    BREAKPOINT_CREATED,
                    startstr = "Breakpoint created")
    
    def static_method_commands(self):
        """Tests that C strings work as expected in expressions"""
        self.runCmd("file a.out", CURRENT_EXECUTABLE_SET)

        self.set_breakpoint(line_number('main.c', '// breakpoint 1'))

        self.runCmd("process launch", RUN_SUCCEEDED)

        self.expect("expression -- a[2]",
                    patterns = ["\((const )?char\) \$0 = 'c'"])

        self.expect("expression -- z[2]",
                    startstr = "(const char) $1 = 'x'")

        self.expect("expression -- (int)strlen(\"hello\")",
                    startstr = "(int) $2 = 5")

        self.expect("expression -- \"world\"[2]",
                    startstr = "(const char) $3 = 'r'")

        self.expect("expression -- \"\"[0]",
                    startstr = "(const char) $4 = '\\0'")

        self.expect("p \"hello\"",
            substrs = ['(const char [6]) $', 'hello',
                       '(const char) [0] = \'h\'',
                       '(const char) [5] = \'\\0\''])

        self.expect("p (char*)\"hello\"",
                    substrs = ['(char *) $', ' = 0x',
                               'hello'])

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()

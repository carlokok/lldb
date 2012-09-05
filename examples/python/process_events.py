#!/usr/bin/python

#----------------------------------------------------------------------
# Be sure to add the python path that points to the LLDB shared library.
# On MacOSX csh, tcsh:
#   setenv PYTHONPATH /Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Resources/Python
# On MacOSX sh, bash:
#   export PYTHONPATH=/Applications/Xcode.app/Contents/SharedFrameworks/LLDB.framework/Resources/Python
#----------------------------------------------------------------------

import commands
import optparse
import os
import platform
import sys

#----------------------------------------------------------------------
# Code that auto imports LLDB
#----------------------------------------------------------------------
try: 
    # Just try for LLDB in case PYTHONPATH is already correctly setup
    import lldb
except ImportError:
    lldb_python_dirs = list()
    # lldb is not in the PYTHONPATH, try some defaults for the current platform
    platform_system = platform.system()
    if platform_system == 'Darwin':
        # On Darwin, try the currently selected Xcode directory
        xcode_dir = commands.getoutput("xcode-select --print-path")
        if xcode_dir:
            lldb_python_dirs.append(os.path.realpath(xcode_dir + '/../SharedFrameworks/LLDB.framework/Resources/Python'))
            lldb_python_dirs.append(xcode_dir + '/Library/PrivateFrameworks/LLDB.framework/Resources/Python')
        lldb_python_dirs.append('/System/Library/PrivateFrameworks/LLDB.framework/Resources/Python')
    success = False
    for lldb_python_dir in lldb_python_dirs:
        if os.path.exists(lldb_python_dir):
            if not (sys.path.__contains__(lldb_python_dir)):
                sys.path.append(lldb_python_dir)
                try: 
                    import lldb
                except ImportError:
                    pass
                else:
                    print 'imported lldb from: "%s"' % (lldb_python_dir)
                    success = True
                    break
    if not success:
        print "error: couldn't locate the 'lldb' module, please set PYTHONPATH correctly"
        sys.exit(1)








def print_threads(process, options):
    if options.show_threads:
        for thread in process:
            print '%s %s' % (thread, thread.GetFrameAtIndex(0))

def run_commands(command_interpreter, commands):
    return_obj = lldb.SBCommandReturnObject()
    for command in commands:
        command_interpreter.HandleCommand( command, return_obj )
        if return_obj.Succeeded():
            print return_obj.GetOutput()
        else:
            print return_obj
            if options.stop_on_error:
                break
    
def main(argv):
    description='''Debugs a program using the LLDB python API and uses asynchronous broadcast events to watch for process state changes.'''
    epilog='''Examples:

#----------------------------------------------------------------------
# Run "/bin/ls" with the arguments "-lAF /tmp/", and set a breakpoint 
# at "malloc" and backtrace and read all registers each time we stop
#----------------------------------------------------------------------
% ./process_events.py --breakpoint malloc --stop-command bt --stop-command 'register read' -- /bin/ls -lAF /tmp/

'''
    optparse.OptionParser.format_epilog = lambda self, formatter: self.epilog
    parser = optparse.OptionParser(description=description, prog='process_events',usage='usage: process_events [options] program [arg1 arg2]', epilog=epilog)
    parser.add_option('-v', '--verbose', action='store_true', dest='verbose', help="Enable verbose logging.", default=False)
    parser.add_option('-b', '--breakpoint', action='append', type='string', metavar='BPEXPR', dest='breakpoints', help='Breakpoint commands to create after the target has been created, the values will be sent to the "_regexp-break" command which supports breakpoints by name, file:line, and address.')
    parser.add_option('-a', '--arch', type='string', dest='arch', help='The architecture to use when creating the debug target.', default=None)
    parser.add_option('-l', '--launch-command', action='append', type='string', metavar='CMD', dest='launch_commands', help='LLDB command interpreter commands to run once after the process has launched. This option can be specified more than once.', default=[])
    parser.add_option('-s', '--stop-command', action='append', type='string', metavar='CMD', dest='stop_commands', help='LLDB command interpreter commands to run each time the process stops. This option can be specified more than once.', default=[])
    parser.add_option('-c', '--crash-command', action='append', type='string', metavar='CMD', dest='crash_commands', help='LLDB command interpreter commands to run in case the process crashes. This option can be specified more than once.', default=[])
    parser.add_option('-x', '--exit-command', action='append', type='string', metavar='CMD', dest='exit_commands', help='LLDB command interpreter commands to run once after the process has exited. This option can be specified more than once.', default=[])
    parser.add_option('-T', '--no-threads', action='store_false', dest='show_threads', help="Don't show threads when process stops.", default=True)
    parser.add_option('-e', '--ignore-errors', action='store_false', dest='stop_on_error', help="Don't stop executing LLDB commands if the command returns an error. This applies to all of the LLDB command interpreter commands that get run for launch, stop, crash and exit.", default=True)
    parser.add_option('-n', '--run-count', type='int', dest='run_count', metavar='N', help='How many times to run the process in case the process exits.', default=1)
    parser.add_option('-t', '--event-timeout', type='int', dest='event_timeout', metavar='SEC', help='Specify the timeout in seconds to wait for process state change events.', default=5)
    try:
        (options, args) = parser.parse_args(argv)
    except:
        return
    if not args:
        print 'error: a program path for a program to debug and its arguments are required'
        sys.exit(1)
    
    exe = args.pop(0)

    # Create a new debugger instance
    debugger = lldb.SBDebugger.Create()
    command_interpreter = debugger.GetCommandInterpreter()
    # Create a target from a file and arch
    print "Creating a target for '%s'" % exe
    
    target = debugger.CreateTargetWithFileAndArch (exe, options.arch)
    
    if target:
        
        # Set any breakpoints that were specified in the args
        if options.breakpoints:
            for bp in options.breakpoints:
                debugger.HandleCommand( "_regexp-break %s" % (bp))
            run_commands(command_interpreter, ['breakpoint list'])
        
        for run_idx in range(options.run_count):
            # Launch the process. Since we specified synchronous mode, we won't return
            # from this function until we hit the breakpoint at main
            if options.run_count == 1:
                print 'Launching "%s"...' % (exe)
            else:
                print 'Launching "%s"... (launch %u of %u)' % (exe, run_idx + 1, options.run_count)
            
            process = target.LaunchSimple (args, None, os.getcwd())
            
            # Make sure the launch went ok
            if process:
                pid = process.GetProcessID()
                listener = lldb.SBListener("event_listener")
                # sign up for process state change events
                process.GetBroadcaster().AddListener(listener, lldb.SBProcess.eBroadcastBitStateChanged)
                stop_idx = 0
                done = False
                while not done:
                    event = lldb.SBEvent()
                    if listener.WaitForEvent (options.event_timeout, event):
                        state = lldb.SBProcess.GetStateFromEvent (event)
                        if state == lldb.eStateStopped:
                            if stop_idx == 0:
                                print "process %u launched" % (pid)
                                run_commands (command_interpreter, options.launch_commands)
                            else:
                                if options.verbose:
                                    print "process %u stopped" % (pid)
                                run_commands (command_interpreter, options.stop_commands)
                            stop_idx += 1
                            print_threads (process, options)
                            process.Continue()
                        elif state == lldb.eStateExited:
                            exit_desc = process.GetExitDescription()
                            if exit_desc:
                                print "process %u exited with status %u: %s" % (pid, process.GetExitStatus (), exit_desc)
                            else:
                                print "process %u exited with status %u" % (pid, process.GetExitStatus ())
                            run_commands (command_interpreter, options.exit_commands)
                            done = True
                        elif state == lldb.eStateCrashed:
                            print "process %u crashed" % (pid)
                            print_threads (process, options)
                            run_commands (command_interpreter, options.crash_commands)
                            done = True
                        elif state == lldb.eStateDetached:
                            print "process %u detached" % (pid)
                            done = True
                        elif state == lldb.eStateRunning:
                            # process is running, don't say anything, we will always get one of these after resuming
                            if options.verbose:
                                print "process %u resumed" % (pid)
                        elif state == lldb.eStateUnloaded:
                            print "process %u unloaded, this shouldn't happen" % (pid)
                            done = True
                        elif state == lldb.eStateConnected:
                            print "process connected"
                        elif state == lldb.eStateAttaching:
                            print "process attaching"
                        elif state == lldb.eStateLaunching:
                            print "process launching"
                    else:
                        # timeout waiting for an event
                        print "no process event for %u seconds, killing the process..." % (options.event_timeout)
                        done = True
                process.Kill() # kill the process
    
    lldb.SBDebugger.Terminate()

if __name__ == '__main__':
    main(sys.argv[1:])
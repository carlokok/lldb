set(LLVM_NO_RTTI 1)

add_lldb_library(lldbBreakpoint
  Breakpoint.cpp
  BreakpointID.cpp
  BreakpointIDList.cpp
  BreakpointList.cpp
  BreakpointLocation.cpp
  BreakpointLocationCollection.cpp
  BreakpointLocationList.cpp
  BreakpointOptions.cpp
  BreakpointResolver.cpp
  BreakpointResolverAddress.cpp
  BreakpointResolverFileLine.cpp
  BreakpointResolverFileRegex.cpp
  BreakpointResolverName.cpp
  BreakpointSite.cpp
  BreakpointSiteList.cpp
  Stoppoint.cpp
  StoppointCallbackContext.cpp
  StoppointLocation.cpp
  Watchpoint.cpp
  WatchpointList.cpp
  WatchpointOptions.cpp
  )
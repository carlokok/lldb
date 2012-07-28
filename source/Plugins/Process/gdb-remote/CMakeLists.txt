set(LLVM_NO_RTTI 1)

add_lldb_library(lldbPluginProcessGDBRemote
  GDBRemoteCommunication.cpp
  GDBRemoteRegisterContext.cpp
  GDBServer.cpp
  GDBServerLog.cpp
  ProcessGDBRemote.cpp
  ProcessGDBRemoteLog.cpp
  ThreadGDBRemote.cpp
  )
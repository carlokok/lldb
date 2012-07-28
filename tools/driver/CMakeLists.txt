set(LLVM_NO_RTTI 1)

set( LLVM_USED_LIBS
  lldbAPI
  lldbInterpreter
  lldbInitAndLog
  lldbAPI
  lldbPluginDisassemblerLLVM
  lldbPluginObjectFileELF
  lldbPluginObjectContainerBSDArchive
  lldbPluginSymbolFileDWARF
  lldbPluginSymbolFileSymtab
#  lldbPluginSymbolVendorMacOSX
  lldbTarget
  lldbCommands
  lldbBreakpoint
  lldbCore
  lldbSymbol
  lldbExpression
  lldbUtility
  lldbHostLinux
  clangFrontend
  clangDriver
  clangCodeGen
  clangSema
  clangChecker
  clangAnalysis
  clangRewrite
  clangAST
  clangParse
  clangLex
  clangBasic
  EnhancedDisassembly
  edit
  python2.6
)

set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  jit
  interpreter
  nativecodegen
  asmparser
  bitreader
  bitwriter
  codegen
  ipo
  selectiondag
  bitreader
  mc
  core
  )

add_lldb_executable(lldb
  Driver.cpp
  IOChannel.cpp
  )

install(TARGETS lldb
  RUNTIME DESTINATION bin)

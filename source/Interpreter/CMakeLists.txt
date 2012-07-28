set(LLVM_NO_RTTI 1)

add_lldb_library(lldbInterpreter
  Args.cpp
  CommandInterpreter.cpp
  CommandObject.cpp
  CommandObjectRegexCommand.cpp
  CommandObjectScript.cpp
  CommandReturnObject.cpp
  NamedOptionValue.cpp
  OptionGroupArchitecture.cpp
  OptionGroupBoolean.cpp
  OptionGroupFile.cpp
  OptionGroupFormat.cpp
  OptionGroupOutputFile.cpp
  OptionGroupPlatform.cpp
  OptionGroupUInt64.cpp
  OptionGroupUUID.cpp
  OptionGroupValueObjectDisplay.cpp
  OptionGroupVariable.cpp
  OptionGroupWatchpoint.cpp
  Options.cpp
  ScriptInterpreter.cpp
  ScriptInterpreterNone.cpp
  ScriptInterpreterPython.cpp
  )
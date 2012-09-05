//===-- ClangUserExpression.cpp -------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C Includes
#include <stdio.h>
#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

// C++ Includes

#include "lldb/Core/ConstString.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Stream.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Expression/ClangExpressionDeclMap.h"
#include "lldb/Expression/ClangExpressionParser.h"
#include "lldb/Expression/ClangUtilityFunction.h"
#include "lldb/Host/Host.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/Target.h"

using namespace lldb_private;

//------------------------------------------------------------------
/// Constructor
///
/// @param[in] text
///     The text of the function.  Must be a full translation unit.
///
/// @param[in] name
///     The name of the function, as used in the text.
//------------------------------------------------------------------
ClangUtilityFunction::ClangUtilityFunction (const char *text, 
                                            const char *name) :
    ClangExpression (),
    m_function_text (text),
    m_function_name (name)
{
}

ClangUtilityFunction::~ClangUtilityFunction ()
{
}

//------------------------------------------------------------------
/// Install the utility function into a process
///
/// @param[in] error_stream
///     A stream to print parse errors and warnings to.
///
/// @param[in] exe_ctx
///     The execution context to install the utility function to.
///
/// @return
///     True on success (no errors); false otherwise.
//------------------------------------------------------------------
bool
ClangUtilityFunction::Install (Stream &error_stream,
                               ExecutionContext &exe_ctx)
{
    lldb::LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_EXPRESSIONS));
    
    if (m_jit_start_addr != LLDB_INVALID_ADDRESS)
    {
        error_stream.PutCString("error: already installed\n");
        return false;
    }
    
    ////////////////////////////////////
    // Set up the target and compiler
    //
    
    Target *target = exe_ctx.GetTargetPtr();
    
    if (!target)
    {
        error_stream.PutCString ("error: invalid target\n");
        return false;
    }
    
    Process *process = exe_ctx.GetProcessPtr();
    
    if (!process)
    {
        error_stream.PutCString ("error: invalid process\n");
        return false;
    }
        
    //////////////////////////
    // Parse the expression
    //
    
    bool keep_result_in_memory = false;
    
    m_expr_decl_map.reset(new ClangExpressionDeclMap(keep_result_in_memory, exe_ctx));
    
    m_data_allocator.reset(new ProcessDataAllocator(*process));
    
    if (!m_expr_decl_map->WillParse(exe_ctx))
    {
        error_stream.PutCString ("error: current process state is unsuitable for expression parsing\n");
        return false;
    }
        
    ClangExpressionParser parser(exe_ctx.GetBestExecutionContextScope(), *this);
    
    unsigned num_errors = parser.Parse (error_stream);
    
    if (num_errors)
    {
        error_stream.Printf ("error: %d errors parsing expression\n", num_errors);
        
        m_expr_decl_map.reset();
        
        return false;
    }
    
    //////////////////////////////////
    // JIT the output of the parser
    //
    
    lldb::ClangExpressionVariableSP const_result;
    
    bool evaluated_statically = false; // should stay that way
    
    Error jit_error = parser.PrepareForExecution (m_jit_alloc, 
                                                  m_jit_start_addr, 
                                                  m_jit_end_addr, 
                                                  exe_ctx,
                                                  m_data_allocator.get(),
                                                  evaluated_statically,
                                                  const_result,
                                                  eExecutionPolicyAlways);
    
    if (log)
    {
        StreamString dump_string;
        m_data_allocator->Dump(dump_string);
        
        log->Printf("Data buffer contents:\n%s", dump_string.GetString().c_str());
    }
    
    if (m_jit_start_addr != LLDB_INVALID_ADDRESS)
        m_jit_process_sp = process->shared_from_this();
    
#if 0
	// jingham: look here
    StreamFile logfile ("/tmp/exprs.txt", "a");
    logfile.Printf ("0x%16.16llx: func = %s, source =\n%s\n", 
                    m_jit_start_addr, 
                    m_function_name.c_str(), 
                    m_function_text.c_str());
#endif

    m_expr_decl_map->DidParse();
    
    m_expr_decl_map.reset();
    
    if (jit_error.Success())
    {
        return true;
    }
    else
    {
        const char *error_cstr = jit_error.AsCString();
        if (error_cstr && error_cstr[0])
            error_stream.Printf ("error: %s\n", error_cstr);
        else
            error_stream.Printf ("error: expression can't be interpreted or run\n");
        return false;
    }
}



//===-- IRForTarget.h ---------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_IRForTarget_h_
#define liblldb_IRForTarget_h_

#include "lldb/lldb-public.h"
#include "lldb/Core/ConstString.h"
#include "lldb/Core/Error.h"
#include "lldb/Core/Stream.h"
#include "lldb/Symbol/TaggedASTType.h"
#include "llvm/Pass.h"

namespace llvm {
    class BasicBlock;
    class CallInst;
    class Constant;
    class ConstantInt;
    class Function;
    class GlobalValue;
    class GlobalVariable;
    class Instruction;
    class Module;
    class StoreInst;
    class TargetData;
    class Type;
    class Value;
}

namespace lldb_private {
    class ClangExpressionDeclMap;
}

//----------------------------------------------------------------------
/// @class IRForTarget IRForTarget.h "lldb/Expression/IRForTarget.h"
/// @brief Transforms the IR for a function to run in the target
///
/// Once an expression has been parsed and converted to IR, it can run
/// in two contexts: interpreted by LLDB as a DWARF location expression,
/// or compiled by the JIT and inserted into the target process for
/// execution.
///
/// IRForTarget makes the second possible, by applying a series of
/// transformations to the IR which make it relocatable.  These
/// transformations are discussed in more detail next to their relevant
/// functions.
//----------------------------------------------------------------------
class IRForTarget : public llvm::ModulePass
{
public:
    class StaticDataAllocator {
    public:
        StaticDataAllocator();
        virtual ~StaticDataAllocator();
        virtual lldb_private::StreamString &GetStream() = 0;
        virtual lldb::addr_t Allocate() = 0;
    };
    
    //------------------------------------------------------------------
    /// Constructor
    ///
    /// @param[in] decl_map
    ///     The list of externally-referenced variables for the expression,
    ///     for use in looking up globals and allocating the argument
    ///     struct.  See the documentation for ClangExpressionDeclMap.
    ///
    /// @param[in] resolve_vars
    ///     True if the external variable references (including persistent
    ///     variables) should be resolved.  If not, only external functions
    ///     are resolved.
    ///
    /// @param[in] execution_policy
    ///     Determines whether an IR interpreter can be used to statically
    ///     evaluate the expression.
    ///
    /// @param[in] const_result
    ///     This variable is populated with the statically-computed result
    ///     of the function, if it has no side-effects and the result can
    ///     be computed statically.
    ///
    /// @param[in] data_allocator
    ///     If non-NULL, the static data allocator to use for literal strings.
    ///
    /// @param[in] error_stream
    ///     If non-NULL, a stream on which errors can be printed.
    ///
    /// @param[in] func_name
    ///     The name of the function to prepare for execution in the target.
    //------------------------------------------------------------------
    IRForTarget(lldb_private::ClangExpressionDeclMap *decl_map,
                bool resolve_vars,
                lldb_private::ExecutionPolicy execution_policy,
                lldb::ClangExpressionVariableSP &const_result,
                StaticDataAllocator *data_allocator,
                lldb_private::Stream *error_stream,
                const char* func_name = "$__lldb_expr");
    
    //------------------------------------------------------------------
    /// Destructor
    //------------------------------------------------------------------
    virtual ~IRForTarget();
    
    //------------------------------------------------------------------
    /// Run this IR transformer on a single module
    ///
    /// Implementation of the llvm::ModulePass::runOnModule() function.
    ///
    /// @param[in] llvm_module
    ///     The module to run on.  This module is searched for the function
    ///     $__lldb_expr, and that function is passed to the passes one by 
    ///     one.
    ///
    /// @param[in] interpreter_error
    ///     An error.  If the expression fails to be interpreted, this error
    ///     is set to a reason why.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    virtual bool 
    runOnModule (llvm::Module &llvm_module);
    
    //------------------------------------------------------------------
    /// Interface stub
    ///
    /// Implementation of the llvm::ModulePass::assignPassManager() 
    /// function.
    //------------------------------------------------------------------
    virtual void
    assignPassManager (llvm::PMStack &pass_mgr_stack,
                       llvm::PassManagerType pass_mgr_type = llvm::PMT_ModulePassManager);
    
    //------------------------------------------------------------------
    /// Returns PMT_ModulePassManager
    ///
    /// Implementation of the llvm::ModulePass::getPotentialPassManagerType() 
    /// function.
    //------------------------------------------------------------------
    virtual llvm::PassManagerType 
    getPotentialPassManagerType() const;
    
    //------------------------------------------------------------------
    /// Checks whether the IR interpreter successfully interpreted the
    /// expression.
    ///
    /// Returns true if it did; false otherwise.
    //------------------------------------------------------------------
    lldb_private::Error &
    getInterpreterError ()
    {
        return m_interpreter_error;
    }

private:
    //------------------------------------------------------------------
    /// Ensures that the current function's linkage is set to external.
    /// Otherwise the JIT may not return an address for it.
    ///
    /// @param[in] llvm_function
    ///     The function whose linkage is to be fixed.
    ///
    /// @return
    ///     True on success; false otherwise.
    //------------------------------------------------------------------
    bool 
    FixFunctionLinkage (llvm::Function &llvm_function);
    
    //------------------------------------------------------------------
    /// A module-level pass to replace all function pointers with their
    /// integer equivalents.
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] llvm_module
    ///     The module currently being processed.
    ///
    /// @param[in] llvm_function
    ///     The function currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise.
    //------------------------------------------------------------------
    bool 
    HasSideEffects (llvm::Function &llvm_function);
    
    //------------------------------------------------------------------
    /// A function-level pass to check whether the function has side
    /// effects.
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// Get the address of a fuction, and a location to put the complete
    /// Value of the function if one is available.
    ///
    /// @param[in] function
    ///     The function to find the location of.
    ///
    /// @param[out] ptr
    ///     The location of the function in the target.
    ///
    /// @param[out] name
    ///     The resolved name of the function (matters for intrinsics).
    ///
    /// @param[out] value_ptr
    ///     A variable to put the function's completed Value* in, or NULL
    ///     if the Value* shouldn't be stored anywhere.
    ///
    /// @return
    ///     The pointer.
    //------------------------------------------------------------------ 
    bool 
    GetFunctionAddress (llvm::Function *function,
                        uint64_t &ptr,
                        lldb_private::ConstString &name,
                        llvm::Constant **&value_ptr);
    
    //------------------------------------------------------------------
    /// Build a function pointer given a type and a raw pointer.
    ///
    /// @param[in] type
    ///     The type of the function pointer to be built.
    ///
    /// @param[in] ptr
    ///     The value of the pointer.
    ///
    /// @return
    ///     The pointer.
    //------------------------------------------------------------------ 
    llvm::Constant *
    BuildFunctionPointer (llvm::Type *type,
                          uint64_t ptr);
    
    void
    RegisterFunctionMetadata (llvm::LLVMContext &context,
                              llvm::Value *function_ptr,
                              const char *name);
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] llvm_function
    ///     The function currently being processed.
    ///
    /// @return
    ///     True if the function has side effects (or if this cannot
    ///     be determined); false otherwise.
    //------------------------------------------------------------------
    bool 
    ResolveFunctionPointers (llvm::Module &llvm_module,
                             llvm::Function &llvm_function);
    
    //------------------------------------------------------------------
    /// A function-level pass to take the generated global value
    /// $__lldb_expr_result and make it into a persistent variable.
    /// Also see ASTResultSynthesizer.
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// Find the NamedDecl corresponding to a Value.  This interface is
    /// exposed for the IR interpreter.
    ///
    /// @param[in] module
    ///     The module containing metadata to search
    ///
    /// @param[in] global
    ///     The global entity to search for
    ///
    /// @return
    ///     The corresponding variable declaration
    //------------------------------------------------------------------
public:
    static clang::NamedDecl *
    DeclForGlobal (const llvm::GlobalValue *global_val, llvm::Module *module);
private:
    clang::NamedDecl *
    DeclForGlobal (llvm::GlobalValue *global);
    
    //------------------------------------------------------------------
    /// Set the constant result variable m_const_result to the provided
    /// constant, assuming it can be evaluated.  The result variable
    /// will be reset to NULL later if the expression has side effects.
    ///
    /// @param[in] initializer
    ///     The constant initializer for the variable.
    ///
    /// @param[in] name
    ///     The name of the result variable.
    ///
    /// @param[in] type
    ///     The Clang type of the result variable.
    //------------------------------------------------------------------    
    void 
    MaybeSetConstantResult (llvm::Constant *initializer,
                            const lldb_private::ConstString &name,
                            lldb_private::TypeFromParser type);
    
    //------------------------------------------------------------------
    /// If the IR represents a cast of a variable, set m_const_result
    /// to the result of the cast.  The result variable will be reset to
    /// NULL latger if the expression has side effects.
    ///
    /// @param[in] type
    ///     The Clang type of the result variable.
    //------------------------------------------------------------------  
    void
    MaybeSetCastResult (lldb_private::TypeFromParser type);
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] llvm_function
    ///     The function currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    CreateResultVariable (llvm::Function &llvm_function);
    
    //------------------------------------------------------------------
    /// A function-level pass to find Objective-C constant strings and
    /// transform them to calls to CFStringCreateWithBytes.
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    /// Rewrite a single Objective-C constant string.
    ///
    /// @param[in] NSStr
    ///     The constant NSString to be transformed
    ///
    /// @param[in] CStr
    ///     The constant C string inside the NSString.  This will be
    ///     passed as the bytes argument to CFStringCreateWithBytes.
    ///
    /// @param[in] FirstEntryInstruction
    ///     An instruction early in the execution of the function.
    ///     When this function synthesizes a call to 
    ///     CFStringCreateWithBytes, it places the call before this
    ///     instruction.  The instruction should come before all 
    ///     uses of the NSString.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    RewriteObjCConstString (llvm::GlobalVariable *NSStr,
                            llvm::GlobalVariable *CStr,
                            llvm::Instruction *FirstEntryInstruction);    
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] llvm_function
    ///     The function currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    RewriteObjCConstStrings (llvm::Function &llvm_function);

    //------------------------------------------------------------------
    /// A basic block-level pass to find all Objective-C method calls and
    /// rewrite them to use sel_registerName instead of statically allocated
    /// selectors.  The reason is that the selectors are created on the
    /// assumption that the Objective-C runtime will scan the appropriate
    /// section and prepare them.  This doesn't happen when code is copied
    /// into the target, though, and there's no easy way to induce the
    /// runtime to scan them.  So instead we get our selectors from
    /// sel_registerName.
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    /// Replace a single selector reference
    ///
    /// @param[in] selector_load
    ///     The load of the statically-allocated selector.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    RewriteObjCSelector (llvm::Instruction* selector_load);
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] basic_block
    ///     The basic block currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    RewriteObjCSelectors (llvm::BasicBlock &basic_block);
    
    //------------------------------------------------------------------
    /// A basic block-level pass to find all newly-declared persistent
    /// variables and register them with the ClangExprDeclMap.  This 
    /// allows them to be materialized and dematerialized like normal
    /// external variables.  Before transformation, these persistent
    /// variables look like normal locals, so they have an allocation.
    /// This pass excises these allocations and makes references look
    /// like external references where they will be resolved -- like all
    /// other external references -- by ResolveExternals().
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    /// Handle a single allocation of a persistent variable
    ///
    /// @param[in] persistent_alloc
    ///     The allocation of the persistent variable.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    RewritePersistentAlloc (llvm::Instruction *persistent_alloc);
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] basic_block
    ///     The basic block currently being processed.
    //------------------------------------------------------------------
    bool 
    RewritePersistentAllocs (llvm::BasicBlock &basic_block);
    
    //------------------------------------------------------------------
    /// A function-level pass to find all external variables and functions 
    /// used in the IR.  Each found external variable is added to the 
    /// struct, and each external function is resolved in place, its call
    /// replaced with a call to a function pointer whose value is the 
    /// address of the function in the target process.
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// Write an initializer to a memory array of assumed sufficient
    /// size.
    ///
    /// @param[in] data
    ///     A pointer to the data to write to.
    ///
    /// @param[in] initializer
    ///     The initializer itself.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool
    MaterializeInitializer (uint8_t *data, llvm::Constant *initializer);
    
    //------------------------------------------------------------------
    /// Move an internal variable into the static allocation section.
    ///
    /// @param[in] global_variable
    ///     The variable.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool
    MaterializeInternalVariable (llvm::GlobalVariable *global_variable);
    
    //------------------------------------------------------------------
    /// Handle a single externally-defined variable
    ///
    /// @param[in] value
    ///     The variable.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    MaybeHandleVariable (llvm::Value *value);
    
    //------------------------------------------------------------------
    /// Handle a single externally-defined symbol
    ///
    /// @param[in] symbol
    ///     The symbol.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool
    HandleSymbol (llvm::Value *symbol);
    
    //------------------------------------------------------------------
    /// Handle a single externally-defined Objective-C class
    ///
    /// @param[in] classlist_reference
    ///     The reference, usually "01L_OBJC_CLASSLIST_REFERENCES_$_n"
    ///     where n (if present) is an index.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool
    HandleObjCClass(llvm::Value *classlist_reference);

    //------------------------------------------------------------------
    /// Handle all the arguments to a function call
    ///
    /// @param[in] C
    ///     The call instruction.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    MaybeHandleCallArguments (llvm::CallInst *call_inst);
    
    //------------------------------------------------------------------
    /// Resolve variable references in calls to external functions
    ///
    /// @param[in] basic_block
    ///     The basic block currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    ResolveCalls (llvm::BasicBlock &basic_block);
    
    //------------------------------------------------------------------
    /// Remove calls to __cxa_atexit, which should never be generated by
    /// expressions.
    ///
    /// @param[in] call_inst
    ///     The call instruction.
    ///
    /// @return
    ///     True if the scan was successful; false if some operation
    ///     failed
    //------------------------------------------------------------------
    bool
    RemoveCXAAtExit (llvm::BasicBlock &basic_block);
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] basic_block
    ///     The function currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    ResolveExternals (llvm::Function &llvm_function);
    
    //------------------------------------------------------------------
    /// A basic block-level pass to excise guard variables from the code.
    /// The result for the function is passed through Clang as a static
    /// variable.  Static variables normally have guard variables to
    /// ensure that they are only initialized once.  
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// Rewrite a load to a guard variable to return constant 0.
    ///
    /// @param[in] guard_load
    ///     The load instruction to zero out.
    //------------------------------------------------------------------
    void
    TurnGuardLoadIntoZero(llvm::Instruction* guard_load);
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] basic_block
    ///     The basic block currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    RemoveGuards (llvm::BasicBlock &basic_block);
    
    //------------------------------------------------------------------
    /// A module-level pass to allocate all string literals in a separate
    /// allocation and redirect references to them.
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    ReplaceStrings ();
    
    //------------------------------------------------------------------
    /// A basick block-level pass to find all literals that will be 
    /// allocated as statics by the JIT (in contrast to the Strings, 
    /// which already are statics) and synthesize loads for them.
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] basic_block
    ///     The basic block currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    ReplaceStaticLiterals (llvm::BasicBlock &basic_block);
    
    //------------------------------------------------------------------
    /// A function-level pass to make all external variable references
    /// point at the correct offsets from the void* passed into the
    /// function.  ClangExpressionDeclMap::DoStructLayout() must be called
    /// beforehand, so that the offsets are valid.
    //------------------------------------------------------------------
    
    //------------------------------------------------------------------
    /// The top-level pass implementation
    ///
    /// @param[in] llvm_function
    ///     The function currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    ReplaceVariables (llvm::Function &llvm_function);
    
    /// Flags
    bool                                    m_resolve_vars;             ///< True if external variable references and persistent variable references should be resolved
    lldb_private::ExecutionPolicy           m_execution_policy;         ///< True if the interpreter should be used to attempt to get a static result
    bool                                    m_interpret_success;        ///< True if the interpreter successfully handled the whole expression
    std::string                             m_func_name;                ///< The name of the function to translate
    lldb_private::ConstString               m_result_name;              ///< The name of the result variable ($0, $1, ...)
    lldb_private::TypeFromParser            m_result_type;              ///< The type of the result variable.
    llvm::Module                           *m_module;                   ///< The module being processed, or NULL if that has not been determined yet.
    std::auto_ptr<llvm::TargetData>         m_target_data;              ///< The target data for the module being processed, or NULL if there is no module.
    lldb_private::ClangExpressionDeclMap   *m_decl_map;                 ///< The DeclMap containing the Decls 
    StaticDataAllocator                    *m_data_allocator;           ///< If non-NULL, the allocator to use for constant strings
    llvm::Constant                         *m_CFStringCreateWithBytes;  ///< The address of the function CFStringCreateWithBytes, cast to the appropriate function pointer type
    llvm::Constant                         *m_sel_registerName;         ///< The address of the function sel_registerName, cast to the appropriate function pointer type
    lldb::ClangExpressionVariableSP        &m_const_result;             ///< This value should be set to the return value of the expression if it is constant and the expression has no side effects
    lldb_private::Stream                   *m_error_stream;             ///< If non-NULL, the stream on which errors should be printed
    lldb_private::Error                     m_interpreter_error;        ///< The error result from the IR interpreter
    
    bool                                    m_has_side_effects;         ///< True if the function's result cannot be simply determined statically
    llvm::StoreInst                        *m_result_store;             ///< If non-NULL, the store instruction that writes to the result variable.  If m_has_side_effects is true, this is NULL.
    bool                                    m_result_is_pointer;        ///< True if the function's result in the AST is a pointer (see comments in ASTResultSynthesizer::SynthesizeBodyResult)
    
    llvm::GlobalVariable                   *m_reloc_placeholder;        ///< A placeholder that will be replaced by a pointer to the final location of the static allocation.
    
    //------------------------------------------------------------------
    /// UnfoldConstant operates on a constant [Old] which has just been 
    /// replaced with a value [New].  We assume that new_value has 
    /// been properly placed early in the function, in front of the 
    /// first instruction in the entry basic block 
    /// [FirstEntryInstruction].  
    ///
    /// UnfoldConstant reads through the uses of Old and replaces Old 
    /// in those uses with New.  Where those uses are constants, the 
    /// function generates new instructions to compute the result of the 
    /// new, non-constant expression and places them before 
    /// FirstEntryInstruction.  These instructions replace the constant
    /// uses, so UnfoldConstant calls itself recursively for those.
    ///
    /// @param[in] llvm_function
    ///     The function currently being processed.
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    static bool 
    UnfoldConstant (llvm::Constant *old_constant, 
                    llvm::Value *new_constant, 
                    llvm::Instruction *first_entry_inst);
    
    //------------------------------------------------------------------
    /// Construct a reference to m_reloc_placeholder with a given type
    /// and offset.  This typically happens after inserting data into
    /// m_data_allocator.
    ///
    /// @param[in] type
    ///     The type of the value being loaded.
    ///
    /// @param[in] offset
    ///     The offset of the value from the base of m_data_allocator.
    ///
    /// @return
    ///     The Constant for the reference, usually a ConstantExpr.
    //------------------------------------------------------------------
    llvm::Constant *
    BuildRelocation(llvm::Type *type, 
                    uint64_t offset);
    
    //------------------------------------------------------------------
    /// Commit the allocation in m_data_allocator and use its final
    /// location to replace m_reloc_placeholder.
    ///
    /// @param[in] module
    ///     The module that m_data_allocator resides in
    ///
    /// @return
    ///     True on success; false otherwise
    //------------------------------------------------------------------
    bool 
    CompleteDataAllocation ();

};

#endif

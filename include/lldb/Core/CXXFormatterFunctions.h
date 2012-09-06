//===-- CXXFormatterFunctions.h------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_CXXFormatterFunctions_h_
#define liblldb_CXXFormatterFunctions_h_

#include "lldb/lldb-forward.h"
#ifdef _MSC_VER
typedef unsigned __int64 uint64_t;
#endif

namespace lldb_private {
    namespace formatters
    {
        
        bool
        CodeRunning_Fetcher (ValueObject &valobj,
                             const char* target_type,
                             const char* selector,
                             uint64_t &value);
        
        template<bool name_entries>
        bool
        NSDictionary_SummaryProvider (ValueObject& valobj, Stream& stream);
        
        bool
        NSArray_SummaryProvider (ValueObject& valobj, Stream& stream);
        
        template<bool needs_at>
        bool
        NSData_SummaryProvider (ValueObject& valobj, Stream& stream);
        
        bool
        NSNumber_SummaryProvider (ValueObject& valobj, Stream& stream);

        bool
        NSString_SummaryProvider (ValueObject& valobj, Stream& stream);
        
        template bool
        NSDictionary_SummaryProvider<true> (ValueObject&, Stream&) ;
        
        template bool
        NSDictionary_SummaryProvider<false> (ValueObject&, Stream&) ;
        
        template bool
        NSData_SummaryProvider<true> (ValueObject&, Stream&) ;
        
        template bool
        NSData_SummaryProvider<false> (ValueObject&, Stream&) ;
        
    }
}

#endif

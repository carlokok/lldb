//===-- FormatNavigator.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef lldb_FormatNavigator_h_
#define lldb_FormatNavigator_h_

// C Includes
// C++ Includes

// Other libraries and framework includes
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"
#include "clang/AST/DeclObjC.h"

// Project includes
#include "lldb/lldb-public.h"

#include "lldb/Core/FormatClasses.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/RegularExpression.h"
#include "lldb/Core/ValueObject.h"
#include "lldb/Target/ObjCLanguageRuntime.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/TargetList.h"

using lldb::LogSP;

namespace lldb_private {
    
// this file (and its. cpp) contain the low-level implementation of LLDB Data Visualization
// class DataVisualization is the high-level front-end of this feature
// clients should refer to that class as the entry-point into the data formatters
// unless they have a good reason to bypass it and prefer to use this file's objects directly
class IFormatChangeListener
{
public:
    virtual void
    Changed () = 0;
    
    virtual
    ~IFormatChangeListener () {}
    
    virtual uint32_t
    GetCurrentRevision () = 0;
    
};
    
static inline bool
IsWhitespace (char c)
{
    return ( (c == ' ') || (c == '\t') || (c == '\v') || (c == '\f') );
}

static inline bool
HasPrefix (const char* str1, const char* str2)
{
    return ( ::strstr(str1, str2) == str1 );
}
    
// if the user tries to add formatters for, say, "struct Foo"
// those will not match any type because of the way we strip qualifiers from typenames
// this method looks for the case where the user is adding a "class","struct","enum" or "union" Foo
// and strips the unnecessary qualifier
static ConstString
GetValidTypeName_Impl (const ConstString& type)
{
    int strip_len = 0;
    
    if (type == false)
        return type;
    
    const char* type_cstr = type.AsCString();
    
    if ( HasPrefix(type_cstr, "class ") )
        strip_len = 6;
    else if ( HasPrefix(type_cstr, "enum ") )
        strip_len = 5;
    else if ( HasPrefix(type_cstr, "struct ") )
        strip_len = 7;
    else if ( HasPrefix(type_cstr, "union ") )
        strip_len = 6;
    
    if (strip_len == 0)
        return type;
    
    type_cstr += strip_len;
    while (IsWhitespace(*type_cstr) && ++type_cstr)
        ;
    
    return ConstString(type_cstr);
}
    
template<typename KeyType, typename ValueType>
class FormatNavigator;

template<typename KeyType, typename ValueType>
class FormatMap
{
public:

    typedef typename ValueType::SharedPointer ValueSP;
    typedef std::map<KeyType, ValueSP> MapType;
    typedef typename MapType::iterator MapIterator;
    typedef bool(*CallbackType)(void*, KeyType, const ValueSP&);
    
    FormatMap(IFormatChangeListener* lst) :
    m_map(),
    m_map_mutex(Mutex::eMutexTypeRecursive),
    listener(lst)
    {
    }
    
    void
    Add(KeyType name,
        const ValueSP& entry)
    {
        if (listener)
            entry->GetRevision() = listener->GetCurrentRevision();
        else
            entry->GetRevision() = 0;

        Mutex::Locker(m_map_mutex);
        m_map[name] = entry;
        if (listener)
            listener->Changed();
    }
    
    bool
    Delete (KeyType name)
    {
        Mutex::Locker(m_map_mutex);
        MapIterator iter = m_map.find(name);
        if (iter == m_map.end())
            return false;
        m_map.erase(name);
        if (listener)
            listener->Changed();
        return true;
    }
    
    void
    Clear ()
    {
        Mutex::Locker(m_map_mutex);
        m_map.clear();
        if (listener)
            listener->Changed();
    }
    
    bool
    Get(KeyType name,
        ValueSP& entry)
    {
        Mutex::Locker(m_map_mutex);
        MapIterator iter = m_map.find(name);
        if (iter == m_map.end())
            return false;
        entry = iter->second;
        return true;
    }
    
    void
    LoopThrough (CallbackType callback, void* param)
    {
        if (callback)
        {
            Mutex::Locker(m_map_mutex);
            MapIterator pos, end = m_map.end();
            for (pos = m_map.begin(); pos != end; pos++)
            {
                KeyType type = pos->first;
                if (!callback(param, type, pos->second))
                    break;
            }
        }
    }
    
    uint32_t
    GetCount ()
    {
        return m_map.size();
    }
    
    ValueSP
    GetValueAtIndex (uint32_t index)
    {
        Mutex::Locker(m_map_mutex);
        MapIterator iter = m_map.begin();
        MapIterator end = m_map.end();
        while (index > 0)
        {
            iter++;
            index--;
            if (end == iter)
                return ValueSP();
        }
        return iter->second;
    }
    
    KeyType
    GetKeyAtIndex (uint32_t index)
    {
        Mutex::Locker(m_map_mutex);
        MapIterator iter = m_map.begin();
        MapIterator end = m_map.end();
        while (index > 0)
        {
            iter++;
            index--;
            if (end == iter)
                return KeyType();
        }
        return iter->first;
    }
    
protected:
    MapType m_map;    
    Mutex m_map_mutex;
    IFormatChangeListener* listener;
    
    MapType&
    map ()
    {
        return m_map;
    }
    
    Mutex&
    mutex ()
    {
        return m_map_mutex;
    }
    
    friend class FormatNavigator<KeyType, ValueType>;
    friend class FormatManager;
    
};
    
template<typename KeyType, typename ValueType>
class FormatNavigator
{
protected:
    typedef FormatMap<KeyType,ValueType> BackEndType;
    
public:
    typedef typename BackEndType::MapType MapType;
    typedef typename MapType::iterator MapIterator;
    typedef typename MapType::key_type MapKeyType;
    typedef typename MapType::mapped_type MapValueType;
    typedef typename BackEndType::CallbackType CallbackType;
#ifdef _LIBCPP_VERSION
    typedef typename std::shared_ptr<FormatNavigator<KeyType, ValueType> > SharedPointer;
#else
    typedef typename std::tr1::shared_ptr<FormatNavigator<KeyType, ValueType> > SharedPointer;
#endif
    
    friend class TypeCategoryImpl;

    FormatNavigator(std::string name,
                    IFormatChangeListener* lst) :
    m_format_map(lst),
    m_name(name),
    m_id_cs(ConstString("id"))
    {
    }
    
    void
    Add (const MapKeyType &type, const MapValueType& entry)
    {
        Add_Impl(type, entry, (KeyType*)NULL);
    }
    
    bool
    Delete (ConstString type)
    {
        return Delete_Impl(type, (KeyType*)NULL);
    }
        
    bool
    Get(ValueObject& valobj,
        MapValueType& entry,
        lldb::DynamicValueType use_dynamic,
        uint32_t* why = NULL)
    {
        uint32_t value = lldb_private::eFormatterChoiceCriterionDirectChoice;
        clang::QualType type = clang::QualType::getFromOpaquePtr(valobj.GetClangType());
        bool ret = Get(valobj, type, entry, use_dynamic, value);
        if (ret)
            entry = MapValueType(entry);
        else
            entry = MapValueType();        
        if (why)
            *why = value;
        return ret;
    }
    
    bool
    Get (ConstString type, MapValueType& entry)
    {
        return Get_Impl(type, entry, (KeyType*)NULL);
    }
    
    bool
    GetExact (ConstString type, MapValueType& entry)
    {
        return GetExact_Impl(type, entry, (KeyType*)NULL);
    }
    
    MapValueType
    GetAtIndex (uint32_t index)
    {
        return m_format_map.GetValueAtIndex(index);
    }
    
    lldb::TypeNameSpecifierImplSP
    GetTypeNameSpecifierAtIndex (uint32_t index)
    {
        return GetTypeNameSpecifierAtIndex_Impl(index, (KeyType*)NULL);
    }
    
    void
    Clear ()
    {
        m_format_map.Clear();
    }
    
    void
    LoopThrough (CallbackType callback, void* param)
    {
        m_format_map.LoopThrough(callback,param);
    }
    
    uint32_t
    GetCount ()
    {
        return m_format_map.GetCount();
    }
    
protected:
        
    BackEndType m_format_map;
    
    std::string m_name;
    
    DISALLOW_COPY_AND_ASSIGN(FormatNavigator);
    
    ConstString m_id_cs;
                           
    void
    Add_Impl (const MapKeyType &type, const MapValueType& entry, lldb::RegularExpressionSP *dummy)
    {
       m_format_map.Add(type,entry);
    }

    void Add_Impl (const ConstString &type, const MapValueType& entry, ConstString *dummy)
    {
       m_format_map.Add(GetValidTypeName_Impl(type), entry);
    }

    bool
    Delete_Impl (ConstString type, ConstString *dummy)
    {
       return m_format_map.Delete(type);
    }

    bool
    Delete_Impl (ConstString type, lldb::RegularExpressionSP *dummy)
    {
       Mutex& x_mutex = m_format_map.mutex();
        lldb_private::Mutex::Locker locker(x_mutex);
       MapIterator pos, end = m_format_map.map().end();
       for (pos = m_format_map.map().begin(); pos != end; pos++)
       {
           lldb::RegularExpressionSP regex = pos->first;
           if ( ::strcmp(type.AsCString(),regex->GetText()) == 0)
           {
               m_format_map.map().erase(pos);
               if (m_format_map.listener)
                   m_format_map.listener->Changed();
               return true;
           }
       }
       return false;
    }    

    bool
    Get_Impl (ConstString type, MapValueType& entry, ConstString *dummy)
    {
       return m_format_map.Get(type, entry);
    }

    bool
    GetExact_Impl (ConstString type, MapValueType& entry, ConstString *dummy)
    {
        return Get_Impl(type,entry, (KeyType*)0);
    }
    
    lldb::TypeNameSpecifierImplSP
    GetTypeNameSpecifierAtIndex_Impl (uint32_t index, ConstString *dummy)
    {
        ConstString key = m_format_map.GetKeyAtIndex(index);
        if (key)
            return lldb::TypeNameSpecifierImplSP(new TypeNameSpecifierImpl(key.AsCString(),
                                                                           false));
        else
            return lldb::TypeNameSpecifierImplSP();
    }
    
    lldb::TypeNameSpecifierImplSP
    GetTypeNameSpecifierAtIndex_Impl (uint32_t index, lldb::RegularExpressionSP *dummy)
    {
        lldb::RegularExpressionSP regex = m_format_map.GetKeyAtIndex(index);
        if (regex.get() == NULL)
            return lldb::TypeNameSpecifierImplSP();
        return lldb::TypeNameSpecifierImplSP(new TypeNameSpecifierImpl(regex->GetText(),
                                                                       true));
    }

    bool
    Get_Impl (ConstString key, MapValueType& value, lldb::RegularExpressionSP *dummy)
    {
       Mutex& x_mutex = m_format_map.mutex();
       lldb_private::Mutex::Locker locker(x_mutex);
       MapIterator pos, end = m_format_map.map().end();
       for (pos = m_format_map.map().begin(); pos != end; pos++)
       {
           lldb::RegularExpressionSP regex = pos->first;
           if (regex->Execute(key.AsCString()))
           {
               value = pos->second;
               return true;
           }
       }
       return false;
    }
    
    bool
    GetExact_Impl (ConstString key, MapValueType& value, lldb::RegularExpressionSP *dummy)
    {
        Mutex& x_mutex = m_format_map.mutex();
        lldb_private::Mutex::Locker locker(x_mutex);
        MapIterator pos, end = m_format_map.map().end();
        for (pos = m_format_map.map().begin(); pos != end; pos++)
        {
            lldb::RegularExpressionSP regex = pos->first;
            if (strcmp(regex->GetText(),key.AsCString()) == 0)
            {
                value = pos->second;
                return true;
            }
        }
        return false;
    }
    
    bool
    Get_BitfieldMatch (ValueObject& valobj,
                       ConstString typeName,
                       MapValueType& entry,
                       uint32_t& reason)
    {
        LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_TYPES));
        // for bitfields, append size to the typename so one can custom format them
        StreamString sstring;
        sstring.Printf("%s:%d",typeName.AsCString(),valobj.GetBitfieldBitSize());
        ConstString bitfieldname = ConstString(sstring.GetData());
        if (log)
            log->Printf("appended bitfield info, final result is %s", bitfieldname.GetCString());
        if (Get(bitfieldname, entry))
        {
            if (log)
                log->Printf("bitfield direct match found, returning");
            return true;
        }
        else
        {
            reason |= lldb_private::eFormatterChoiceCriterionStrippedBitField;
            if (log)
                log->Printf("no bitfield direct match");
            return false;
        }
    }
    
    bool Get_ObjC (ValueObject& valobj,
                   MapValueType& entry)
    {
        LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_TYPES));
        lldb::ProcessSP process_sp = valobj.GetProcessSP();
        ObjCLanguageRuntime* runtime = process_sp->GetObjCLanguageRuntime();
        if (runtime == NULL)
        {
            if (log)
                log->Printf("no valid ObjC runtime, skipping dynamic");
            return false;
        }
        ObjCLanguageRuntime::ObjCISA isa = runtime->GetISA(valobj);
        if (runtime->IsValidISA(isa) == false)
        {
            if (log)
                log->Printf("invalid ISA, skipping dynamic");
            return false;
        }
        ConstString name = runtime->GetActualTypeName(isa);
        if (log)
            log->Printf("dynamic type inferred is %s - looking for direct dynamic match", name.GetCString());
        if (Get(name, entry))
        {
            if (log)
                log->Printf("direct dynamic match found, returning");
            return true;
        }
        if (log)
            log->Printf("no dynamic match");
        return false;
    }
    
    bool Get (ValueObject& valobj,
              clang::QualType type, // TODO: find out why "type" is passed in the type when it belongs to valobj? Can it ever differ?
              MapValueType& entry,
              lldb::DynamicValueType use_dynamic,
              uint32_t& reason)
    {
        LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_TYPES));
        if (type.isNull())
        {
            if (log)
                log->Printf("type is NULL, returning");
            return false;
        }

        type.removeLocalConst(); type.removeLocalVolatile(); type.removeLocalRestrict();
        const clang::Type* typePtr = type.getTypePtrOrNull();
        if (!typePtr)
        {
            if (log)
                log->Printf("type is NULL, returning");
            return false;
        }
        ConstString typeName(ClangASTType::GetTypeNameForQualType(valobj.GetClangAST(), type).c_str());
        
        if (valobj.GetBitfieldBitSize() > 0)
        {
            if (Get_BitfieldMatch(valobj, typeName, entry, reason))
                return true;
        }
        
        if (log)
            log->Printf("trying to get %s for VO name %s of type %s",
                        m_name.c_str(),
                        valobj.GetName().AsCString(),
                        typeName.AsCString());
        
        if (Get(typeName, entry))
        {
            if (log)
                log->Printf("direct match found, returning");
            return true;
        }
        if (log)
            log->Printf("no direct match");

        // strip pointers and references and see if that helps
        if (typePtr->isReferenceType())
        {
            if (log)
                log->Printf("stripping reference");
            if (Get(valobj,type.getNonReferenceType(),entry, use_dynamic, reason) && !entry->SkipsReferences())
            {
                reason |= lldb_private::eFormatterChoiceCriterionStrippedPointerReference;
                return true;
            }
        }
        
        if (typePtr->isPointerType())
        {
            if (log)
                log->Printf("stripping pointer");
            clang::QualType pointee = typePtr->getPointeeType();
            if (Get(valobj, pointee, entry, use_dynamic, reason) && !entry->SkipsPointers())
            {
                reason |= lldb_private::eFormatterChoiceCriterionStrippedPointerReference;
                return true;
            }
        }

        if (typePtr->isObjCObjectPointerType())
        {
            if (use_dynamic != lldb::eNoDynamicValues)
            {
                if (log)
                    log->Printf("allowed to figure out dynamic ObjC type");
                if (Get_ObjC(valobj,entry))
                {
                    reason |= lldb_private::eFormatterChoiceCriterionDynamicObjCDiscovery;
                    return true;
                }
            }
            if (log)
                log->Printf("dynamic disabled or failed - stripping ObjC pointer");
            clang::QualType pointee = typePtr->getPointeeType();
            if (Get(valobj, pointee, entry, use_dynamic, reason) && !entry->SkipsPointers())
            {
                reason |= lldb_private::eFormatterChoiceCriterionStrippedPointerReference;
                return true;
            }
        }
    
        // try to strip typedef chains
        const clang::TypedefType* type_tdef = type->getAs<clang::TypedefType>();
        if (type_tdef)
        {
            if (log)
                log->Printf("stripping typedef");
            if ((Get(valobj, type_tdef->getDecl()->getUnderlyingType(), entry, use_dynamic, reason)) && entry->Cascades())
            {
                reason |= lldb_private::eFormatterChoiceCriterionNavigatedTypedefs;
                return true;
            }
        }
        return false;
    }
};

} // namespace lldb_private

#endif	// lldb_FormatNavigator_h_

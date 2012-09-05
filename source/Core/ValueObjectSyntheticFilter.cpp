//===-- ValueObjectSyntheticFilter.cpp -----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#include "lldb/Core/ValueObjectSyntheticFilter.h"

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Core/FormatClasses.h"
#include "lldb/Core/ValueObject.h"

using namespace lldb_private;

ValueObjectSynthetic::ValueObjectSynthetic (ValueObject &parent, lldb::SyntheticChildrenSP filter) :
    ValueObject(parent),
    m_synth_sp(filter),
    m_synth_filter_ap(filter->GetFrontEnd(parent)),
    m_children_byindex(),
    m_name_toindex(),
    m_synthetic_children_count(UINT32_MAX)
{
#ifdef LLDB_CONFIGURATION_DEBUG
    std::string new_name(parent.GetName().AsCString());
    new_name += "$$__synth__";
    SetName (ConstString(new_name.c_str()));
#else
    SetName(parent.GetName());
#endif
}

ValueObjectSynthetic::~ValueObjectSynthetic()
{
}

lldb::clang_type_t
ValueObjectSynthetic::GetClangTypeImpl ()
{
    return m_parent->GetClangType();
}

ConstString
ValueObjectSynthetic::GetTypeName()
{
    return m_parent->GetTypeName();
}

uint32_t
ValueObjectSynthetic::CalculateNumChildren()
{
    UpdateValueIfNeeded();
    if (m_synthetic_children_count < UINT32_MAX)
        return m_synthetic_children_count;
    return (m_synthetic_children_count = m_synth_filter_ap->CalculateNumChildren());
}

clang::ASTContext *
ValueObjectSynthetic::GetClangASTImpl ()
{
    return m_parent->GetClangAST ();
}

size_t
ValueObjectSynthetic::GetByteSize()
{
    return m_parent->GetByteSize();
}

lldb::ValueType
ValueObjectSynthetic::GetValueType() const
{
    return m_parent->GetValueType();
}

bool
ValueObjectSynthetic::UpdateValue ()
{
    SetValueIsValid (false);
    m_error.Clear();

    if (!m_parent->UpdateValueIfNeeded(false))
    {
        // our parent could not update.. as we are meaningless without a parent, just stop
        if (m_parent->GetError().Fail())
            m_error = m_parent->GetError();
        return false;
    }

    // let our backend do its update
    if (m_synth_filter_ap->Update() == false)
    {
        // filter said that cached values are stale
        m_children_byindex.clear();
        m_name_toindex.clear();
        // usually, an object's value can change but this does not alter its children count
        // for a synthetic VO that might indeed happen, so we need to tell the upper echelons
        // that they need to come back to us asking for children
        m_children_count_valid = false;
        m_synthetic_children_count = UINT32_MAX;
    }
    
    SetValueIsValid(true);
    return true;
}

lldb::ValueObjectSP
ValueObjectSynthetic::GetChildAtIndex (uint32_t idx, bool can_create)
{
    UpdateValueIfNeeded();
    
    ByIndexIterator iter = m_children_byindex.find(idx);
    
    if (iter == m_children_byindex.end())
    {
        if (can_create && m_synth_filter_ap.get() != NULL)
        {
            lldb::ValueObjectSP synth_guy = m_synth_filter_ap->GetChildAtIndex (idx, can_create);
            if (!synth_guy)
                return synth_guy;
            m_children_byindex[idx]= synth_guy.get();
            return synth_guy;
        }
        else
            return lldb::ValueObjectSP();
    }
    else
        return iter->second->GetSP();
}

lldb::ValueObjectSP
ValueObjectSynthetic::GetChildMemberWithName (const ConstString &name, bool can_create)
{
    UpdateValueIfNeeded();

    uint32_t index = GetIndexOfChildWithName(name);
    
    if (index == UINT32_MAX)
        return lldb::ValueObjectSP();
    
    return GetChildAtIndex(index, can_create);
}

uint32_t
ValueObjectSynthetic::GetIndexOfChildWithName (const ConstString &name)
{
    UpdateValueIfNeeded();
    
    NameToIndexIterator iter = m_name_toindex.find(name.GetCString());
    
    if (iter == m_name_toindex.end() && m_synth_filter_ap.get() != NULL)
    {
        uint32_t index = m_synth_filter_ap->GetIndexOfChildWithName (name);
        if (index == UINT32_MAX)
            return index;
        m_name_toindex[name.GetCString()] = index;
        return index;
    }
    else if (iter == m_name_toindex.end() && m_synth_filter_ap.get() == NULL)
        return UINT32_MAX;
    else /*if (iter != m_name_toindex.end())*/
        return iter->second;
}

bool
ValueObjectSynthetic::IsInScope ()
{
    return m_parent->IsInScope();
}

lldb::ValueObjectSP
ValueObjectSynthetic::GetNonSyntheticValue ()
{
    return m_parent->GetSP();
}

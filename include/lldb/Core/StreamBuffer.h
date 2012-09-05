//===-- StreamBuffer.h ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_StreamBuffer_h_
#define liblldb_StreamBuffer_h_

#include <stdio.h>
#include <string>
#include "llvm/ADT/SmallVector.h"
#include "lldb/Core/Stream.h"

namespace lldb_private {

template <unsigned N>
class StreamBuffer : public Stream
{
public:
    StreamBuffer () :
        Stream (0, 4, lldb::eByteOrderBig),
        m_packet ()
    {
    }


    StreamBuffer (uint32_t flags,
                  uint32_t addr_size,
                  lldb::ByteOrder byte_order) :
        Stream (flags, addr_size, byte_order),
        m_packet ()
    {
    }

    virtual
    ~StreamBuffer ()
    {
    }

    virtual void
    Flush ()
    {
        // Nothing to do when flushing a buffer based stream...
    }

    virtual int
    Write (const void *s, size_t length)
    {
        if (s && length)
            m_packet.append ((const char *)s, ((const char *)s) + length);
        return length;
    }

    void
    Clear()
    {
        m_packet.clear();
    }

    const char *
    GetData () const
    {
        return m_packet.data();
    }

    size_t
    GetSize() const
    {
        return m_packet.size();
    }

protected:
    llvm::SmallVector<char, N> m_packet;

};

} // namespace lldb_private

#endif  // #ifndef liblldb_StreamBuffer_h_

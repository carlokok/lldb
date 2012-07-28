//===-- Stream.h ------------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_Stream_h_
#define liblldb_Stream_h_
#if defined(__cplusplus)

#include "lldb/lldb-private.h"
#include "lldb/Core/Flags.h"
#include <stdarg.h>

namespace lldb_private {

//----------------------------------------------------------------------
/// @class Stream Stream.h "lldb/Core/Stream.h"
/// @brief A stream class that can stream formatted output to a file.
//----------------------------------------------------------------------
class Stream
{
public:
    //------------------------------------------------------------------
    /// \a m_flags bit values.
    //------------------------------------------------------------------
    enum
    {
        eVerbose    = (1 << 0), ///< If set, verbose logging is enabled
        eDebug      = (1 << 1), ///< If set, debug logging is enabled
        eAddPrefix  = (1 << 2), ///< Add number prefixes for binary, octal and hex when eBinary is clear
        eBinary     = (1 << 3)  ///< Get and put data as binary instead of as the default string mode.
    };

    //------------------------------------------------------------------
    /// Construct with flags and address size and byte order.
    ///
    /// Construct with dump flags \a flags and the default address
    /// size. \a flags can be any of the above enumeration logical OR'ed
    /// together.
    //------------------------------------------------------------------
    Stream (uint32_t flags,
            uint32_t addr_size,
            lldb::ByteOrder byte_order);

    //------------------------------------------------------------------
    /// Construct a default Stream, not binary, host byte order and
    /// host addr size.
    ///
    //------------------------------------------------------------------
    Stream ();

    //------------------------------------------------------------------
    /// Destructor
    //------------------------------------------------------------------
    virtual
    ~Stream ();

    //------------------------------------------------------------------
    // Subclasses must override these methods
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    /// Flush the stream.
    ///
    /// Subclasses should flush the stream to make any output appear
    /// if the stream has any buffering.
    //------------------------------------------------------------------
    virtual void
    Flush () = 0;

    //------------------------------------------------------------------
    /// Output character bytes to the stream.
    ///
    /// Appends \a src_len characters from the buffer \a src to the
    /// stream.
    ///
    /// @param[in] src
    ///     A buffer containing at least \a src_len bytes of data.
    ///
    /// @param[in] src_len
    ///     A number of bytes to append to the stream.
    ///
    /// @return
    ///     The number of bytes that were appended to the stream.
    //------------------------------------------------------------------
    virtual int
    Write (const void *src, size_t src_len) = 0;

    //------------------------------------------------------------------
    // Member functions
    //------------------------------------------------------------------
    int
    PutChar (char ch);

    //------------------------------------------------------------------
    /// Set the byte_order value.
    ///
    /// Sets the byte order of the data to extract. Extracted values
    /// will be swapped if necessary when decoding.
    ///
    /// @param[in] byte_order
    ///     The byte order value to use when extracting data.
    ///
    /// @return
    ///     The old byte order value.
    //------------------------------------------------------------------
    lldb::ByteOrder
    SetByteOrder (lldb::ByteOrder byte_order);

    //------------------------------------------------------------------
    /// Format a C string from a printf style format and variable
    /// arguments and encode and append the resulting C string as hex
    /// bytes.
    ///
    /// @param[in] format
    ///     A printf style format string.
    ///
    /// @param[in] ...
    ///     Any additional arguments needed for the printf format string.
    ///
    /// @return
    ///     The number of bytes that were appended to the stream.
    //------------------------------------------------------------------
    int
    PrintfAsRawHex8 (const char *format, ...)  LLDB_ATTR(__attribute__ ((format (printf, 2, 3))));

    //------------------------------------------------------------------
    /// Format a C string from a printf style format and variable
    /// arguments and encode and append the resulting C string as hex
    /// bytes.
    ///
    /// @param[in] format
    ///     A printf style format string.
    ///
    /// @param[in] ...
    ///     Any additional arguments needed for the printf format string.
    ///
    /// @return
    ///     The number of bytes that were appended to the stream.
    //------------------------------------------------------------------
    int
    PutHex8 (uint8_t uvalue);

    int
    PutNHex8 (size_t n, uint8_t uvalue);

    int
    PutHex16 (uint16_t uvalue,
              lldb::ByteOrder byte_order = lldb::eByteOrderInvalid);

    int
    PutHex32 (uint32_t uvalue,
              lldb::ByteOrder byte_order = lldb::eByteOrderInvalid);

    int
    PutHex64 (uint64_t uvalue,
              lldb::ByteOrder byte_order = lldb::eByteOrderInvalid);

    int
    PutMaxHex64 (uint64_t uvalue,
                 size_t byte_size,
                 lldb::ByteOrder byte_order = lldb::eByteOrderInvalid);
    int
    PutFloat (float f,
              lldb::ByteOrder byte_order = lldb::eByteOrderInvalid);

    int
    PutDouble (double d,
               lldb::ByteOrder byte_order = lldb::eByteOrderInvalid);

    int
    PutLongDouble (long double ld,
                   lldb::ByteOrder byte_order = lldb::eByteOrderInvalid);

    int
    PutPointer (void *ptr);

    int
    PutBytesAsRawHex8 (const void *src,
                       size_t src_len,
                       lldb::ByteOrder src_byte_order = lldb::eByteOrderInvalid,
                       lldb::ByteOrder dst_byte_order = lldb::eByteOrderInvalid);

    int
    PutRawBytes (const void *s, 
                 size_t src_len,
                 lldb::ByteOrder src_byte_order = lldb::eByteOrderInvalid,
                 lldb::ByteOrder dst_byte_order = lldb::eByteOrderInvalid);

    int
    PutCStringAsRawHex8 (const char *s);

    //------------------------------------------------------------------
    /// Output a NULL terminated C string \a cstr to the stream \a s.
    ///
    /// @param[in] cstr
    ///     A NULL terminated C string.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (const char *cstr);

    //------------------------------------------------------------------
    /// Output a pointer value \a p to the stream \a s.
    ///
    /// @param[in] p
    ///     A void pointer.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (void *p);

    //------------------------------------------------------------------
    /// Output a character \a ch to the stream \a s.
    ///
    /// @param[in] ch
    ///     A printable character value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (char ch);

    //------------------------------------------------------------------
    /// Output a uint8_t \a uval to the stream \a s.
    ///
    /// @param[in] uval
    ///     A uint8_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (uint8_t uval);

    //------------------------------------------------------------------
    /// Output a uint16_t \a uval to the stream \a s.
    ///
    /// @param[in] uval
    ///     A uint16_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (uint16_t uval);

    //------------------------------------------------------------------
    /// Output a uint32_t \a uval to the stream \a s.
    ///
    /// @param[in] uval
    ///     A uint32_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (uint32_t uval);

    //------------------------------------------------------------------
    /// Output a uint64_t \a uval to the stream \a s.
    ///
    /// @param[in] uval
    ///     A uint64_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (uint64_t uval);

    //------------------------------------------------------------------
    /// Output a int8_t \a sval to the stream \a s.
    ///
    /// @param[in] sval
    ///     A int8_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (int8_t sval);

    //------------------------------------------------------------------
    /// Output a int16_t \a sval to the stream \a s.
    ///
    /// @param[in] sval
    ///     A int16_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (int16_t sval);

    //------------------------------------------------------------------
    /// Output a int32_t \a sval to the stream \a s.
    ///
    /// @param[in] sval
    ///     A int32_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (int32_t sval);

    //------------------------------------------------------------------
    /// Output a int64_t \a sval to the stream \a s.
    ///
    /// @param[in] sval
    ///     A int64_t value.
    ///
    /// @return
    ///     A reference to this class so multiple things can be streamed
    ///     in one statement.
    //------------------------------------------------------------------
    Stream&
    operator<< (int64_t sval);

    //------------------------------------------------------------------
    /// Output an address value to this stream.
    ///
    /// Put an address \a addr out to the stream with optional \a prefix
    /// and \a suffix strings.
    ///
    /// @param[in] addr
    ///     An address value.
    ///
    /// @param[in] addr_size
    ///     Size in bytes of the address, used for formatting.
    ///
    /// @param[in] prefix
    ///     A prefix C string. If NULL, no prefix will be output.
    ///
    /// @param[in] suffix
    ///     A suffix C string. If NULL, no suffix will be output.
    //------------------------------------------------------------------
    void
    Address (uint64_t addr, int addr_size, const char *prefix = NULL, const char *suffix = NULL);

    //------------------------------------------------------------------
    /// Output an address range to this stream.
    ///
    /// Put an address range \a lo_addr - \a hi_addr out to the stream
    /// with optional \a prefix and \a suffix strings.
    ///
    /// @param[in] lo_addr
    ///     The start address of the address range.
    ///
    /// @param[in] hi_addr
    ///     The end address of the address range.
    ///
    /// @param[in] addr_size
    ///     Size in bytes of the address, used for formatting.
    ///
    /// @param[in] prefix
    ///     A prefix C string. If NULL, no prefix will be output.
    ///
    /// @param[in] suffix
    ///     A suffix C string. If NULL, no suffix will be output.
    //------------------------------------------------------------------
    void
    AddressRange(uint64_t lo_addr, uint64_t hi_addr, int addr_size, const char *prefix = NULL, const char *suffix = NULL);

    //------------------------------------------------------------------
    /// Output a C string to the stream.
    ///
    /// Print a C string \a cstr to the stream.
    ///
    /// @param[in] cstr
    ///     The string to be output to the stream.
    //------------------------------------------------------------------
    int
    PutCString (const char *cstr);

    //------------------------------------------------------------------
    /// Output and End of Line character to the stream.
    //------------------------------------------------------------------
    int
    EOL();

    //------------------------------------------------------------------
    /// Get the address size in bytes.
    ///
    /// @return
    ///     The size of an address in bytes that is used when outputting
    ///     address and pointer values to the stream.
    //------------------------------------------------------------------
    uint8_t
    GetAddressByteSize () const;

    //------------------------------------------------------------------
    /// Test if debug logging is enabled.
    ///
    /// @return
    //      \b true if the debug flag bit is set in this stream, \b
    //      false otherwise.
    //------------------------------------------------------------------
    bool
    GetDebug() const;

    //------------------------------------------------------------------
    /// The flags accessor.
    ///
    /// @return
    ///     A reference to the Flags member variable.
    //------------------------------------------------------------------
    Flags&
    GetFlags();

    //------------------------------------------------------------------
    /// The flags const accessor.
    ///
    /// @return
    ///     A const reference to the Flags member variable.
    //------------------------------------------------------------------
    const Flags&
    GetFlags() const;
    
    //------------------------------------------------------------------
    //// The byte order accessor.
    ////
    //// @return
    ////     The byte order.
    //------------------------------------------------------------------
    lldb::ByteOrder
    GetByteOrder() const;

    //------------------------------------------------------------------
    /// Get the current indentation level.
    ///
    /// @return
    ///     The current indentation level as an integer.
    //------------------------------------------------------------------
    int
    GetIndentLevel () const;

    //------------------------------------------------------------------
    /// Test if verbose logging is enabled.
    ///
    /// @return
    //      \b true if the verbose flag bit is set in this stream, \b
    //      false otherwise.
    //------------------------------------------------------------------
    bool
    GetVerbose() const;

    //------------------------------------------------------------------
    /// Indent the current line in the stream.
    ///
    /// Indent the current line using the current indentation level and
    /// print an optional string following the idenatation spaces.
    ///
    /// @param[in] s
    ///     A C string to print following the indentation. If NULL, just
    ///     output the indentation characters.
    //------------------------------------------------------------------
    int
    Indent(const char *s = NULL);

    //------------------------------------------------------------------
    /// Decrement the current indentation level.
    //------------------------------------------------------------------
    void
    IndentLess (int amount = 2);

    //------------------------------------------------------------------
    /// Increment the current indentation level.
    //------------------------------------------------------------------
    void
    IndentMore (int amount = 2);

    //------------------------------------------------------------------
    /// Output an offset value.
    ///
    /// Put an offset \a uval out to the stream using the printf format
    /// in \a format.
    ///
    /// @param[in] offset
    ///     The offset value.
    ///
    /// @param[in] format
    ///     The printf style format to use when outputting the offset.
    //------------------------------------------------------------------
    void
    Offset (uint32_t offset, const char *format = "0x%8.8x: ");

    //------------------------------------------------------------------
    /// Output printf formatted output to the stream.
    ///
    /// Print some formatted output to the stream.
    ///
    /// @param[in] format
    ///     A printf style format string.
    ///
    /// @param[in] ...
    ///     Variable arguments that are needed for the printf style
    ///     format string \a format.
    //------------------------------------------------------------------
    int
    Printf (const char *format, ...)  LLDB_ATTR(__attribute__ ((format (printf, 2, 3))));

    int
    PrintfVarArg(const char *format, va_list args);

    //------------------------------------------------------------------
    /// Output a quoted C string value to the stream.
    ///
    /// Print a double quoted NULL terminated C string to the stream
    /// using the printf format in \a format.
    ///
    /// @param[in] cstr
    ///     A NULL terminated C string value.
    ///
    /// @param[in] format
    ///     The optional C string format that can be overridden.
    //------------------------------------------------------------------
    void
    QuotedCString (const char *cstr, const char *format = "\"%s\"");

    //------------------------------------------------------------------
    /// Set the address size in bytes.
    ///
    /// @param[in] addr_size
    ///     The new size in bytes of an address to use when outputting
    ///     address and pointer values.
    //------------------------------------------------------------------
    void
    SetAddressByteSize (uint8_t addr_size);

    //------------------------------------------------------------------
    /// Set the current indentation level.
    ///
    /// @param[in] level
    ///     The new indentation level.
    //------------------------------------------------------------------
    void
    SetIndentLevel (int level);

    //------------------------------------------------------------------
    /// Output a SLEB128 number to the stream.
    ///
    /// Put an SLEB128 \a uval out to the stream using the printf format
    /// in \a format.
    ///
    /// @param[in] uval
    ///     A uint64_t value that was extracted as a SLEB128 value.
    ///
    /// @param[in] format
    ///     The optional printf format that can be overridden.
    //------------------------------------------------------------------
    int
    PutSLEB128 (int64_t uval);

    //------------------------------------------------------------------
    /// Output a ULEB128 number to the stream.
    ///
    /// Put an ULEB128 \a uval out to the stream using the printf format
    /// in \a format.
    ///
    /// @param[in] uval
    ///     A uint64_t value that was extracted as a ULEB128 value.
    ///
    /// @param[in] format
    ///     The optional printf format that can be overridden.
    //------------------------------------------------------------------
    int
    PutULEB128 (uint64_t uval);

    static void
    UnitTest(Stream *s);

private:
    //------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------
    Flags m_flags;      ///< Dump flags.
    uint8_t m_addr_size;    ///< Size of an address in bytes.
    lldb::ByteOrder m_byte_order;///< Byte order to use when encoding scalar types.
    int m_indent_level;     ///< Indention level.

    int _PutHex8 (uint8_t uvalue, bool add_prefix);
};

} // namespace lldb_private

#endif  // #if defined(__cplusplus)
#endif  // liblldb_Stream_h_

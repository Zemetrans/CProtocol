/**
 * @file
 * The simple name service protocol implementation
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <qcc/Debug.h>
#include <qcc/SocketTypes.h>
#include <qcc/IPAddress.h>
#include <qcc/StringUtil.h>
#include <alljoyn/AllJoynStd.h>
#include "IpNsProtocol.h"

#define QCC_MODULE "NS"

//
// Strangely, Android doesn't define the IPV4 presentation format string length
// even though it does define the IPv6 version.
//
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

using namespace std;
using namespace qcc;

namespace ajn {
_Packet::_Packet()
    : m_timer(0), m_destination("0.0.0.0", 0), m_destinationSet(false), m_interfaceIndex((uint32_t)-1), m_interfaceIndexSet(false), m_addressFamily(qcc::QCC_AF_UNSPEC), m_addressFamilySet(false), m_retries(0), m_tick(0), m_version(0)
{
    printf("Enter and Exit _Packet::_Packet()\n");
}

_Packet::~_Packet()
{
    printf("Enter and Exit _Packet::~_Packet()\n");
}
StringData::StringData()
    : m_size(0)
{
    printf("Enter and Exit StringData::StringData()\n");
}

StringData::~StringData()
{
    printf("Enter and Exit StringData::~StringData()\n");
}

void StringData::Set(qcc::String string)
{
    printf("Enter StringData::Set\n");
    m_size = string.size();
    m_string = string;
    printf("Exit StringData::Set\n");
}

qcc::String StringData::Get(void) const
{
    printf("Enter and Exit StringData::Get. Return m_string\n");
    return m_string;
}

size_t StringData::GetSerializedSize(void) const
{
    printf("Enter StringData::GetSerializedSize. return 1 + m_size\n");
    return 1 + m_size;
}

size_t StringData::Serialize(uint8_t* buffer) const
{
    printf("Enter StringData::Serialize\n");
    QCC_DbgPrintf(("StringData::Serialize(): %s to buffer 0x%x", m_string.c_str(), buffer));
    QCC_ASSERT(m_size == m_string.size());
    buffer[0] = static_cast<uint8_t>(m_size);
    memcpy(reinterpret_cast<void*>(&buffer[1]), const_cast<void*>(reinterpret_cast<const void*>(m_string.c_str())), m_size);

    printf("Exit StringData::Serialize. Return 1 + m_size\n");
    return 1 + m_size;
}

size_t StringData::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    printf("Enter StringData::Deserialize\n");
    QCC_DbgPrintf(("StringData::Deserialize()"));

    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 1) {
        QCC_DbgPrintf(("StringData::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit StringData::Deserialize. Return 0\n");
        return 0;
    }

    m_size = buffer[0];
    --bufsize;

    //
    // If there's not enough data in the buffer then bail.
    //
    if (bufsize < m_size) {
        QCC_DbgPrintf(("StringData::Deserialize(): Insufficient bufsize %d", bufsize));
        m_size = 0;
        printf("Exit StringData::Deserialize. Return 0\n");
        return 0;
    }
    if (m_size > 0) {
        m_string.assign(reinterpret_cast<const char*>(buffer + 1), m_size);
    } else {
        m_string.clear();
    }
    QCC_DbgPrintf(("StringData::Deserialize(): %s from buffer", m_string.c_str()));
    printf("Exit StringData::Deserialize. Return 1 + m_size\n");
    return 1 + m_size;
}

IsAt::IsAt()
    : m_version(0), m_flagG(false), m_flagC(false),
    m_flagT(false), m_flagU(false), m_flagS(false), m_flagF(false),
    m_flagR4(false), m_flagU4(false), m_flagR6(false), m_flagU6(false),
    m_port(0),
    m_reliableIPv4Port(0), m_unreliableIPv4Port(0), m_reliableIPv6Port(0), m_unreliableIPv6Port(0)
{
    printf("Enter and exit IsAt::IsAt()\n");
}

IsAt::~IsAt()
{
    printf("Enter and exit IsAt::~IsAt()\n");
}

void IsAt::SetGuid(const qcc::String& guid)
{
    printf("EnterIsAt::SetGuid()\n");
    m_guid = guid;
    m_flagG = true;
    printf("Exit IsAt::SetGuid()\n");
}

qcc::String IsAt::GetGuid(void) const
{
    printf("Enter and exit IsAt::GetGuid. Return m_guid()\n");
    return m_guid;
}

void IsAt::SetPort(uint16_t port)
{
    printf("Enter IsAt::SetPort\n");
    m_port = port;
    printf("Exit IsAt::SetPort\n");
}

uint16_t IsAt::GetPort(void) const
{
    printf("Enter and exit IsAt::GetPort. Return m_port()\n");
    return m_port;
}

void IsAt::ClearIPv4(void)
{
    printf("Enter IsAt::ClearIPv4\n");
    m_ipv4.clear();
    m_flagF = false;
    printf("Exit IsAt::ClearIPv4\n");
}

void IsAt::SetIPv4(qcc::String ipv4)
{
    printf("Enter IsAt::SetIPv4\n");
    m_ipv4 = ipv4;
    m_flagF = true;
    printf("Exit IsAt::SetIPv4\n");
}

qcc::String IsAt::GetIPv4(void) const
{
    printf("Enter and Exit IsAt::GetIPv4. Return m_ipv4\n");
    return m_ipv4;
}

void IsAt::ClearIPv6(void)
{
    printf("Enter IsAt::ClearIPv6\n");
    m_ipv6.clear();
    m_flagS = false;
    printf("Exit IsAt::ClearIPv6\n");
}

void IsAt::SetIPv6(qcc::String ipv6)
{
    printf("Enter IsAt::SetIPv6\n");
    m_ipv6 = ipv6;
    m_flagS = true;
    printf("Exit IsAt::SetIPv6\n");
}

qcc::String IsAt::GetIPv6(void) const
{
    printf("Enter and exit IsAt::GetIPv6. return m_ipv6\n");
    return m_ipv6;
}

void IsAt::ClearReliableIPv4(void)
{
    printf("Enter IsAt::ClearReliableIPv4\n");
    m_reliableIPv4Address.clear();
    m_reliableIPv4Port = 0;
    m_flagR4 = false;
    printf("Exit IsAt::ClearReliableIPv4\n");
}

void IsAt::SetReliableIPv4(qcc::String addr, uint16_t port)
{
    printf("Enter IsAt::SetReliableIPv4\n");
    m_reliableIPv4Address = addr;
    m_reliableIPv4Port = port;
    m_flagR4 = true;
    printf("Enter IsAt::SetReliableIPv4\n");
}

qcc::String IsAt::GetReliableIPv4Address(void) const
{
    printf("Enter and exit IsAt::GetReliableIPv4Address\n");
    return m_reliableIPv4Address;
}

uint16_t IsAt::GetReliableIPv4Port(void) const
{
    printf("Enter and exit IsAt::GetReliableIPv4Port\n");
    return m_reliableIPv4Port;
}

void IsAt::ClearUnreliableIPv4(void)
{
    printf("Enter IsAt::ClearUnreliableIPv4\n");
    m_unreliableIPv4Address.clear();
    m_unreliableIPv4Port = 0;
    m_flagU4 = false;
    printf("Exit IsAt::ClearUnreliableIPv4\n");
}

void IsAt::SetUnreliableIPv4(qcc::String addr, uint16_t port)
{
    printf("Enter IsAt::SetUnreliableIPv4\n");
    m_unreliableIPv4Address = addr;
    m_unreliableIPv4Port = port;
    m_flagU4 = true;
    printf("Exit IsAt::SetUnreliableIPv4\n");
}

qcc::String IsAt::GetUnreliableIPv4Address(void) const
{
    printf("Enter and Exit IsAt::GetUnreliableIPv4Address. Return m_unreliableIPv4Address\n");
    return m_unreliableIPv4Address;
}

uint16_t IsAt::GetUnreliableIPv4Port(void) const
{
    printf("Enter and Exit IsAt::GetUnreliableIPv4Port. return m_unreliableIPv4Port \n");
    return m_unreliableIPv4Port;
}

void IsAt::ClearReliableIPv6(void)
{
    printf("Enter IsAt::ClearReliableIPv6\n");
    m_reliableIPv6Address.clear();
    m_reliableIPv6Port = 0;
    m_flagR6 = false;
    printf("Exit IsAt::ClearReliableIPv6\n");
}

void IsAt::SetReliableIPv6(qcc::String addr, uint16_t port)
{
    printf("Enter IsAt::SetReliableIPv6\n");
    m_reliableIPv6Address = addr;
    m_reliableIPv6Port = port;
    m_flagR6 = true;
    printf("Exit IsAt::SetReliableIPv6\n");
}

qcc::String IsAt::GetReliableIPv6Address(void) const
{
    printf("Enter and Exit IsAt::GetReliableIPv6Address. Return m_reliableIPv6Address\n");
    return m_reliableIPv6Address;
}

uint16_t IsAt::GetReliableIPv6Port(void) const
{
    printf("Enter and Exit IsAt::GetReliableIPv6Port. Return m_reliableIPv6Port\n");
    return m_reliableIPv6Port;
}

void IsAt::ClearUnreliableIPv6(void)
{
    printf("Enter IsAt::ClearUnreliableIPv6\n");
    m_unreliableIPv6Address.clear();
    m_unreliableIPv6Port = 0;
    m_flagU6 = false;
    printf("Exit IsAt::ClearUnreliableIPv6\n");
}

void IsAt::SetUnreliableIPv6(qcc::String addr, uint16_t port)
{
    printf("Enter IsAt::SetUnreliableIPv6\n");
    m_unreliableIPv6Address = addr;
    m_unreliableIPv6Port = port;
    m_flagU6 = true;
    printf("Exit IsAt::SetUnreliableIPv6\n");
}

qcc::String IsAt::GetUnreliableIPv6Address(void) const
{
    printf("Enter and exit IsAt::GetUnreliableIPv6Address. Return m_unreliableIPv6Address\n");
    return m_unreliableIPv6Address;
}

uint16_t IsAt::GetUnreliableIPv6Port(void) const
{
    printf("Enter and exit IsAt::GetUnreliableIPv6Port. Return m_unreliableIPv6Port\n");
    return m_unreliableIPv6Port;
}

void IsAt::Reset(void)
{
    printf("Enter IsAt::Reset\n");
    m_names.clear();
    printf("Exit IsAt::Reset\n");
}

void IsAt::AddName(qcc::String name)
{
    printf("Enter IsAt::AddName\n");
    m_names.push_back(name);
    printf("Exit IsAt::AddName\n");
}

void IsAt::RemoveName(uint32_t index)
{
    printf("Enter IsAt::RemoveName\n");
    if (index < m_names.size()) {
        m_names.erase(m_names.begin() + index);
    }
    printf("Exit IsAt::RemoveName\n");
}

uint32_t IsAt::GetNumberNames(void) const
{
    printf("Enter and exit IsAt::GetNumberNames. Return m_names.size()\n");
    return m_names.size();
}

qcc::String IsAt::GetName(uint32_t index) const
{
    printf("Enter IsAt::GetName\n");
    QCC_ASSERT(index < m_names.size());
    printf("Exit IsAt::GetName. Return m_names[index]\n");
    return m_names[index];
}

size_t IsAt::GetSerializedSize(void) const
{
    printf("Enter IsAt::GetSerializedSize\n");
    size_t size;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        //
        // We have one octet for type and flags, one octet for count and
        // two octets for port.  Four octets to start.
        //
        size = 4;

        //
        // If the F bit is set, we are going to include an IPv4 address
        // which is 32 bits long;
        //
        if (m_flagF) {
            size += 32 / 8;
        }

        //
        // If the S bit is set, we are going to include an IPv6 address
        // which is 128 bits long;
        //
        if (m_flagS) {
            size += 128 / 8;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  The rest of the message will be a possible GUID
        // string and the names.
        //
        if (m_flagG) {
            StringData s;
            s.Set(m_guid);
            size += s.GetSerializedSize();
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData s;
            s.Set(m_names[i]);
            size += s.GetSerializedSize();
        }
        break;

    case 1:
        //
        // We have one octet for type and flags, one octet for count and
        // two octets for the transport mask.  Four octets to start.
        //
        size = 4;

        //
        // If the R4 bit is set, we are going to include an IPv4 address
        // which is 32 bits long and a port which is 16 bits long.
        //
        if (m_flagR4) {
            size += 6;
        }

        //
        // If the U4 bit is set, we are going to include an IPv4 address
        // which is 32 bits long and a port which is 16 bits long.
        //
        if (m_flagU4) {
            size += 6;
        }

        //
        // If the R6 bit is set, we are going to include an IPv6 address
        // which is 128 bits long and a port which is 16 bits long.
        //
        if (m_flagR6) {
            size += 18;
        }

        //
        // If the U6 bit is set, we are going to include an IPv6 address
        // which is 32 bits long and a port which is 16 bits long.
        //
        if (m_flagU6) {
            size += 18;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  The rest of the message will be a possible GUID
        // string and the names.
        //
        if (m_flagG) {
            StringData s;
            s.Set(m_guid);
            size += s.GetSerializedSize();
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData s;
            s.Set(m_names[i]);
            size += s.GetSerializedSize();
        }
        break;

    default:
        QCC_ASSERT(false && "IsAt::GetSerializedSize(): Unexpected version");
        QCC_LogError(ER_WARNING, ("IsAt::GetSerializedSize(): Unexpected version %d", m_version & 0xf));
        size = 0;
        break;
    }
    printf("Exit IsAt::GetSerializedSize. Return size\n");
    return size;
}

size_t IsAt::Serialize(uint8_t* buffer) const
{
    printf("Enter IsAt::Serialize\n");
    QCC_DbgPrintf(("IsAt::Serialize(): to buffer 0x%x", buffer));

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    uint8_t typeAndFlags;
    uint8_t* p = NULL;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        //
        // The first octet is type (M = 1) and flags.
        //
        typeAndFlags = 1 << 6;

        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Serialize(): G flag"));
            typeAndFlags |= 0x20;
        }
        if (m_flagC) {
            QCC_DbgPrintf(("IsAt::Serialize(): C flag"));
            typeAndFlags |= 0x10;
        }
        if (m_flagT) {
            QCC_DbgPrintf(("IsAt::Serialize(): T flag"));
            typeAndFlags |= 0x8;
        }
        if (m_flagU) {
            QCC_DbgPrintf(("IsAt::Serialize(): U flag"));
            typeAndFlags |= 0x4;
        }
        if (m_flagS) {
            QCC_DbgPrintf(("IsAt::Serialize(): S flag"));
            typeAndFlags |= 0x2;
        }
        if (m_flagF) {
            QCC_DbgPrintf(("IsAt::Serialize(): F flag"));
            typeAndFlags |= 0x1;
        }

        buffer[0] = typeAndFlags;
        size += 1;

        //
        // The second octet is the count of bus names.
        //
        QCC_ASSERT(m_names.size() < 256);
        buffer[1] = static_cast<uint8_t>(m_names.size());
        QCC_DbgPrintf(("IsAt::Serialize(): Count %d", m_names.size()));
        size += 1;

        //
        // The following two octets are the port number in network byte
        // order (big endian, or most significant byte first).
        //
        buffer[2] = static_cast<uint8_t>(m_port >> 8);
        buffer[3] = static_cast<uint8_t>(m_port);
        QCC_DbgPrintf(("IsAt::Serialize(): Port %d", m_port));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];

        //
        // If the F bit is set, we need to include the IPv4 address.
        //
        if (m_flagF) {
            qcc::IPAddress::StringToIPv4(m_ipv4, p, 4);
            QCC_DbgPrintf(("IsAt::Serialize(): IPv4: %s", m_ipv4.c_str()));
            p += 4;
            size += 4;
        }

        //
        // If the S bit is set, we need to include the IPv6 address.
        //
        if (m_flagS) {
            qcc::IPAddress::StringToIPv6(m_ipv6, p, 16);
            QCC_DbgPrintf(("IsAt::Serialize(): IPv6: %s", m_ipv6.c_str()));
            p += 16;
            size += 16;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  If the G bit is set, we need to include the GUID
        // string.
        //
        if (m_flagG) {
            StringData stringData;
            stringData.Set(m_guid);
            QCC_DbgPrintf(("IsAt::Serialize(): GUID %s", m_guid.c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData stringData;
            stringData.Set(m_names[i]);
            QCC_DbgPrintf(("IsAt::Serialize(): name %s", m_names[i].c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }
        break;

    case 1:
        //
        // The first octet is type (M = 1) and flags.
        //
        typeAndFlags = 1 << 6;

        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Serialize(): G flag"));
            typeAndFlags |= 0x20;
        }
        if (m_flagC) {
            QCC_DbgPrintf(("IsAt::Serialize(): C flag"));
            typeAndFlags |= 0x10;
        }
        if (m_flagR4) {
            QCC_DbgPrintf(("IsAt::Serialize(): R4 flag"));
            typeAndFlags |= 0x8;
        }
        if (m_flagU4) {
            QCC_DbgPrintf(("IsAt::Serialize(): U4 flag"));
            typeAndFlags |= 0x4;
        }
        if (m_flagR6) {
            QCC_DbgPrintf(("IsAt::Serialize(): R6 flag"));
            typeAndFlags |= 0x2;
        }
        if (m_flagU6) {
            QCC_DbgPrintf(("IsAt::Serialize(): U6 flag"));
            typeAndFlags |= 0x1;
        }

        buffer[0] = typeAndFlags;
        size += 1;

        //
        // The second octet is the count of bus names.
        //
        QCC_ASSERT(m_names.size() < 256);
        buffer[1] = static_cast<uint8_t>(m_names.size());
        QCC_DbgPrintf(("IsAt::Serialize(): Count %d", m_names.size()));
        size += 1;

        //
        // The following two octets are the transport mask in network byte
        // order (big endian, or most significant byte first).
        //
        buffer[2] = static_cast<uint8_t>(m_transportMask >> 8);
        buffer[3] = static_cast<uint8_t>(m_transportMask);
        QCC_DbgPrintf(("IsAt::Serialize(): TransportMask 0x%x", m_transportMask));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];

        //
        // If the R4 bit is set, we need to include the reliable IPv4 address
        // and port.
        //
        if (m_flagR4) {
            qcc::IPAddress::StringToIPv4(m_reliableIPv4Address, p, 4);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv4: %s", m_reliableIPv4Address.c_str()));
            p += 4;
            size += 4;

            *p++ = static_cast<uint8_t>(m_reliableIPv4Port >> 8);
            *p++ = static_cast<uint8_t>(m_reliableIPv4Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv4 port %d", m_reliableIPv4Port));
            size += 2;
        }

        //
        // If the U4 bit is set, we need to include the unreliable IPv4 address
        // and port.
        //
        if (m_flagU4) {
            qcc::IPAddress::StringToIPv4(m_unreliableIPv4Address, p, 4);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv4: %s", m_unreliableIPv4Address.c_str()));
            p += 4;
            size += 4;

            *p++ = static_cast<uint8_t>(m_unreliableIPv4Port >> 8);
            *p++ = static_cast<uint8_t>(m_unreliableIPv4Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv4 port %d", m_unreliableIPv4Port));
            size += 2;
        }

        //
        // If the R6 bit is set, we need to include the reliable IPv6 address
        // and port.
        //
        if (m_flagR6) {
            qcc::IPAddress::StringToIPv6(m_reliableIPv6Address, p, 16);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv6: %s", m_reliableIPv6Address.c_str()));
            p += 16;
            size += 16;

            *p++ = static_cast<uint8_t>(m_reliableIPv6Port >> 8);
            *p++ = static_cast<uint8_t>(m_reliableIPv6Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Reliable IPv6 port %d", m_reliableIPv6Port));
            size += 2;
        }

        //
        // If the U6 bit is set, we need to include the unreliable IPv6 address
        // and port.
        //
        if (m_flagU6) {
            qcc::IPAddress::StringToIPv6(m_unreliableIPv6Address, p, 16);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv6: %s", m_unreliableIPv6Address.c_str()));
            p += 16;
            size += 16;

            *p++ = static_cast<uint8_t>(m_unreliableIPv6Port >> 8);
            *p++ = static_cast<uint8_t>(m_unreliableIPv6Port);
            QCC_DbgPrintf(("IsAt::Serialize(): Unreliable IPv6 port %d", m_unreliableIPv6Port));
            size += 2;
        }

        //
        // Let the string data decide for themselves how long the rest of the
        // message will be.  If the G bit is set, we need to include the GUID
        // string.
        //
        if (m_flagG) {
            StringData stringData;
            stringData.Set(m_guid);
            QCC_DbgPrintf(("IsAt::Serialize(): GUID %s", m_guid.c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }

        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData stringData;
            stringData.Set(m_names[i]);
            QCC_DbgPrintf(("IsAt::Serialize(): name %s", m_names[i].c_str()));
            size_t stringSize = stringData.Serialize(p);
            size += stringSize;
            p += stringSize;
        }
        break;

    default:
        QCC_ASSERT(false && "IsAt::Serialize(): Unexpected version");
        break;
    }
    printf("Exit IsAt::Serialize. Return size\n");
    return size;
}

size_t IsAt::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    printf("Enter IsAt::Deserialize\n");
    QCC_DbgPrintf(("IsAt::Deserialize()"));

    //
    // We keep track of the size (the size of the buffer we read) so testers
    // can check coherence between GetSerializedSize() and Serialize() and
    // Deserialize().
    //
    size_t size = 0;

    uint8_t typeAndFlags = 0;
    uint8_t const* p = NULL;
    uint8_t numberNames = 0;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        //
        // If there's not enough room in the buffer to get the fixed part out then
        // bail (one byte of type and flags, one byte of name count and two bytes
        // of port).
        //
        if (bufsize < 4) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
            return 0;
        }

        size = 0;

        //
        // The first octet is type (1) and flags.
        //
        typeAndFlags = buffer[0];
        size += 1;

        //
        // This had better be an IsAt message we're working on
        //
        if ((typeAndFlags & 0xc0) != 1 << 6) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Incorrect type %d", typeAndFlags & 0xc0));
            printf("Exit IsAt::Deserialize. Return 0\n");
            return 0;
        }

        m_flagG = (typeAndFlags & 0x20) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): G flag %d", m_flagG));

        m_flagC = (typeAndFlags & 0x10) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): C flag %d", m_flagC));

        m_flagT = (typeAndFlags & 0x8) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): T flag %d", m_flagT));

        m_flagU = (typeAndFlags & 0x4) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): U flag %d", m_flagU));

        m_flagS = (typeAndFlags & 0x2) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): S flag %d", m_flagS));

        m_flagF = (typeAndFlags & 0x1) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): F flag %d", m_flagF));

        //
        // The second octet is the count of bus names.
        //
        numberNames = buffer[1];
        QCC_DbgPrintf(("IsAt::Deserialize(): Count %d", numberNames));
        size += 1;

        //
        // The following two octets are the port number in network byte
        // order (big endian, or most significant byte first).
        //
        m_port = (static_cast<uint16_t>(buffer[2]) << 8) | (static_cast<uint16_t>(buffer[3]) & 0xff);
        QCC_DbgPrintf(("IsAt::Deserialize(): Port %d", m_port));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];
        bufsize -= 4;

        //
        // If the F bit is set, we need to read off an IPv4 address; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagF) {
            if (bufsize < 4) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }
            m_ipv4 = qcc::IPAddress::IPv4ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): IPv4: %s", m_ipv4.c_str()));
            p += 4;
            size += 4;
            bufsize -= 4;
        }

        //
        // If the S bit is set, we need to read off an IPv6 address; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagS) {
            if (bufsize < 16) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                return 0;
            }
            m_ipv6 = qcc::IPAddress::IPv6ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): IPv6: %s", m_ipv6.c_str()));
            p += 16;
            size += 16;
            bufsize -= 16;
        }

        //
        // If the G bit is set, we need to read off a GUID string.
        //
        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() GUID"));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                printf("Exit IsAt::Deserialize. Return 0\n");
                return 0;
            }
            SetGuid(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }

        //
        // Now we need to read out <numberNames> names that the packet has told us
        // will be there.
        //
        for (uint32_t i = 0; i < numberNames; ++i) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() name %d", i));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                printf("Exit IsAt::Deserialize. Return 0\n");
                return 0;
            }
            AddName(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }
        break;

    case 1:
        //
        // If there's not enough room in the buffer to get the fixed part out then
        // bail (one byte of type and flags, one byte of name count)
        //
        if (bufsize < 4) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
            printf("Exit IsAt::Deserialize. Return 0\n");
            return 0;
        }

        //
        // We keep track of the size (the size of the buffer we read) so testers
        // can check coherence between GetSerializedSize() and Serialize() and
        // Deserialize().
        //
        size = 0;

        //
        // The first octet is type (1) and flags.
        //
        typeAndFlags = buffer[0];
        size += 1;

        //
        // This had better be an IsAt message we're working on
        //
        if ((typeAndFlags & 0xc0) != 1 << 6) {
            QCC_DbgPrintf(("IsAt::Deserialize(): Incorrect type %d", typeAndFlags & 0xc0));
            printf("Exit IsAt::Deserialize. Return 0\n");
            return 0;
        }

        m_flagG = (typeAndFlags & 0x20) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): G flag %d", m_flagG));

        m_flagC = (typeAndFlags & 0x10) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): C flag %d", m_flagC));

        m_flagR4 = (typeAndFlags & 0x8) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): R4 flag %d", m_flagR4));

        m_flagU4 = (typeAndFlags & 0x4) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): U4 flag %d", m_flagU4));

        m_flagR6 = (typeAndFlags & 0x2) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): R6 flag %d", m_flagR6));

        m_flagU6 = (typeAndFlags & 0x1) != 0;
        QCC_DbgPrintf(("IsAt::Deserialize(): F flag %d", m_flagU6));

        //
        // The second octet is the count of bus names.
        //
        numberNames = buffer[1];
        QCC_DbgPrintf(("IsAt::Deserialize(): Count %d", numberNames));
        size += 1;

        m_transportMask = (static_cast<uint16_t>(buffer[2]) << 8) | (static_cast<uint16_t>(buffer[3]) & 0xff);
        QCC_DbgPrintf(("IsAt::Serialize(): TransportMask 0x%x", m_transportMask));
        size += 2;

        //
        // From this point on, things are not at fixed addresses
        //
        p = &buffer[4];
        bufsize -= 4;

        //
        // If the R4 bit is set, we need to read off an IPv4 address and port;
        // and we'd better have enough buffer to read it out of.
        //
        if (m_flagR4) {
            if (bufsize < 6) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                printf("Exit IsAt::Deserialize. Return 0\n");
                return 0;
            }

            m_reliableIPv4Address = qcc::IPAddress::IPv4ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv4: %s", m_reliableIPv4Address.c_str()));
            p += 4;
            size += 4;
            bufsize -= 4;

            m_reliableIPv4Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv4 port %d", m_reliableIPv4Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the R4 bit is set, we need to read off an IPv4 address and port;
        // and we'd better have enough buffer to read it out of.
        //
        if (m_flagU4) {
            if (bufsize < 6) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                printf("Exit IsAt::Deserialize. Return 0\n");
                return 0;
            }

            m_unreliableIPv4Address = qcc::IPAddress::IPv4ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv4: %s", m_unreliableIPv4Address.c_str()));
            p += 4;
            size += 4;
            bufsize -= 4;

            m_unreliableIPv4Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv4 port %d", m_unreliableIPv4Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the R6 bit is set, we need to read off an IPv6 address and port; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagR6) {
            if (bufsize < 18) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                printf("Exit IsAt::Deserialize. Return 0\n");
                return 0;
            }

            m_reliableIPv6Address = qcc::IPAddress::IPv6ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv6: %s", m_reliableIPv6Address.c_str()));
            p += 16;
            size += 16;
            bufsize -= 16;

            m_reliableIPv6Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Reliable IPv6 port %d", m_reliableIPv6Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the U6 bit is set, we need to read off an IPv6 address and port; and we'd better
        // have enough buffer to read it out of.
        //
        if (m_flagU6) {
            if (bufsize < 18) {
                QCC_DbgPrintf(("IsAt::Deserialize(): Insufficient bufsize %d", bufsize));
                printf("Exit IsAt::Deserialize. Return 0\n");
                return 0;
            }

            m_unreliableIPv6Address = qcc::IPAddress::IPv6ToString(p);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv6: %s", m_unreliableIPv6Address.c_str()));
            p += 16;
            size += 16;
            bufsize -= 16;

            m_unreliableIPv6Port = (static_cast<uint16_t>(p[0]) << 8) | (static_cast<uint16_t>(p[1]) & 0xff);
            QCC_DbgPrintf(("IsAt::Deserialize(): Unreliable IPv6 port %d", m_unreliableIPv6Port));
            p += 2;
            size += 2;
            bufsize -= 2;
        }

        //
        // If the G bit is set, we need to read off a GUID string.
        //
        if (m_flagG) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() GUID"));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                printf("Exit IsAt::Deserialize. Return 0\n");
                return 0;
            }
            SetGuid(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }

        //
        // Now we need to read out <numberNames> names that the packet has told us
        // will be there.
        //
        for (uint32_t i = 0; i < numberNames; ++i) {
            QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize() name %d", i));
            StringData stringData;

            //
            // Tell the string to read itself out.  If there's not enough buffer
            // it will complain by returning 0.  We pass the complaint on up.
            //
            size_t stringSize = stringData.Deserialize(p, bufsize);
            if (stringSize == 0) {
                QCC_DbgPrintf(("IsAt::Deserialize(): StringData::Deserialize():  Error"));
                return 0;
            }
            AddName(stringData.Get());
            size += stringSize;
            p += stringSize;
            bufsize -= stringSize;
        }
        break;

    default:
        QCC_ASSERT(false && "IsAt::Deserialize(): Unexpected version");
        break;
    }
    printf("Exit IsAt::Deserialize. Return size\n");
    return size;
}

WhoHas::WhoHas()
    : m_version(0), m_transportMask(TRANSPORT_NONE), m_flagT(false), m_flagU(false), m_flagS(false), m_flagF(false)
{
    printf("Enter WhoHas::WhoHas\n");
}

WhoHas::~WhoHas()
{
    printf("Enter WhoHas::~WhoHas\n");
}

void WhoHas::Reset(void)
{
    printf("Enter WhoHas::Reset\n");
    m_names.clear();
    printf("Exit WhoHas::Reset\n");
}

void WhoHas::AddName(qcc::String name)
{
    printf("Enter WhoHas::AddName\n");
    m_names.push_back(name);
    printf("Exit WhoHas::AddName\n");
}

uint32_t WhoHas::GetNumberNames(void) const
{
    printf("Enter and WhoHas::GetNumberNames. Return m_names.size()\n");
    return m_names.size();
}

qcc::String WhoHas::GetName(uint32_t index) const
{
    printf("Enter WhoHas::GetName\n");
    QCC_ASSERT(index < m_names.size());
    printf("Exit WhoHas::GetName. Return m_names[index]\n");
    return m_names[index];
}

size_t WhoHas::GetSerializedSize(void) const
{
    printf("Enter WhoHas::GetSerializedSize\n");
    //
    // Version zero and one are identical with the exeption of the definition
    // of the flags, so the size is the same.
    //
    size_t size = 0;

    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
    case 1:
        //
        // We have one octet for type and flags and one octet for count.
        // Two octets to start.
        //
        size = 2;

        //
        // Let the string data decide for themselves how long the rest
        // of the message will be.
        //
        for (uint32_t i = 0; i < m_names.size(); ++i) {
            StringData s;
            s.Set(m_names[i]);
            size += s.GetSerializedSize();
        }
        break;

    default:
        QCC_ASSERT(false && "WhoHas::GetSerializedSize(): Unexpected version");
        break;
    }
    printf("Exit WhoHas::GetSerializedSize. Return size\n");
    return size;
}

size_t WhoHas::Serialize(uint8_t* buffer) const
{
    printf("Enter WhoHas::Serialize\n");
    QCC_DbgPrintf(("WhoHas::Serialize(): to buffer 0x%x", buffer));

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is type (M = 2) and flags.
    //
    uint8_t typeAndFlags = 2 << 6;

    //
    // The only difference between version zero and one is that in version one
    // the flags are deprecated and revert to reserved.  So we just don't
    // serialize them if we are writing a version one object.
    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    if ((m_version & 0xf) == 0) {
        if (m_flagT) {
            QCC_DbgPrintf(("WhoHas::Serialize(): T flag"));
            typeAndFlags |= 0x8;
        }
        if (m_flagU) {
            QCC_DbgPrintf(("WhoHas::Serialize(): U flag"));
            typeAndFlags |= 0x4;
        }
        if (m_flagS) {
            QCC_DbgPrintf(("WhoHas::Serialize(): S flag"));
            typeAndFlags |= 0x2;
        }
        if (m_flagF) {
            QCC_DbgPrintf(("WhoHas::Serialize(): F flag"));
            typeAndFlags |= 0x1;
        }
    } else {
        typeAndFlags |= 0x4;
    }

    buffer[0] = typeAndFlags;
    size += 1;

    //
    // The second octet is the count of bus names.
    //
    QCC_ASSERT(m_names.size() < 256);
    buffer[1] = static_cast<uint8_t>(m_names.size());
    QCC_DbgPrintf(("WhoHas::Serialize(): Count %d", m_names.size()));
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t* p = &buffer[2];

    //
    // Let the string data decide for themselves how long the rest
    // of the message will be.
    //
    for (uint32_t i = 0; i < m_names.size(); ++i) {
        StringData stringData;
        stringData.Set(m_names[i]);
        QCC_DbgPrintf(("Whohas::Serialize(): name %s", m_names[i].c_str()));
        size_t stringSize = stringData.Serialize(p);
        size += stringSize;
        p += stringSize;
    }
    printf("Exit WhoHas::Serialize. Return size\n");
    return size;
}

size_t WhoHas::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    printf("Enter WhoHas::Deserialize\n");
    QCC_DbgPrintf(("WhoHas::Deserialize()"));

    //
    // If there's not enough room in the buffer to get the fixed part out then
    // bail (one byte of type and flags, one byte of name count).
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("WhoHas::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit WhoHas::Deserialize. Return 0\n");
        return 0;
    }

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is type (1) and flags.
    //
    uint8_t typeAndFlags = buffer[0];
    size += 1;

    //
    // This had better be an WhoHas message we're working on
    //
    if ((typeAndFlags & 0xc0) != 2 << 6) {
        QCC_DbgPrintf(("WhoHas::Deserialize(): Incorrect type %d", typeAndFlags & 0xc0));
        printf("Exit WhoHas::Deserialize. Return 0\n");
        return 0;
    }

    //
    // Due to an oversight, the transport mask was not actually serialized,
    // so we initialize it to 0, which means no transport.
    //
    m_transportMask = TRANSPORT_NONE;

    //
    // The only difference between the version zero and version one protocols
    // is that the flags are deprecated in version one.  In the case of
    // deserializing a version one object, we just don't set the old flags.
    //
    // The message version is in the least significant nibble of the version.
    // We don't care about the peer name service protocol version which is
    // meta-data about the other side and is in the most significant nibble.
    //
    switch (m_version & 0xf) {
    case 0:
        m_flagT = (typeAndFlags & 0x8) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): T flag %d", m_flagT));

        m_flagU = (typeAndFlags & 0x4) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): U flag %d", m_flagU));

        m_flagS = (typeAndFlags & 0x2) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): S flag %d", m_flagS));

        m_flagF = (typeAndFlags & 0x1) != 0;
        QCC_DbgPrintf(("WhoHas::Deserialize(): F flag %d", m_flagF));
        break;

    case 1:
        m_flagU = (typeAndFlags & 0x4) != 0;
        m_flagT = m_flagS = m_flagF = false;

        break;

    default:
        QCC_ASSERT(false && "WhoHas::Deserialize(): Unexpected version");
        break;
    }

    //
    // The second octet is the count of bus names.
    //
    uint8_t numberNames = buffer[1];
    QCC_DbgPrintf(("WhoHas::Deserialize(): Count %d", numberNames));
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t const* p = &buffer[2];
    bufsize -= 2;

    //
    // Now we need to read out <numberNames> names that the packet has told us
    // will be there.
    //
    for (uint32_t i = 0; i < numberNames; ++i) {
        QCC_DbgPrintf(("WhoHas::Deserialize(): StringData::Deserialize() name %d", i));
        StringData stringData;

        //
        // Tell the string to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t stringSize = stringData.Deserialize(p, bufsize);
        if (stringSize == 0) {
            QCC_DbgPrintf(("WhoHas::Deserialize(): StringData::Deserialize():  Error"));
            printf("Exit WhoHas::Deserialize. Return 0\n");
            return 0;
        }

        AddName(stringData.Get());
        size += stringSize;
        p += stringSize;
        bufsize -= stringSize;
    }
    printf("Exit WhoHas::Deserialize. Return 0\n");
    return size;
}

_NSPacket::_NSPacket()
{
    printf("Enter _NSPacket::_NSPacket\n");
}

_NSPacket::~_NSPacket()
{
    printf("Enter _NSPacket::~_NSPacket\n");
}

void _Packet::SetTimer(uint8_t timer)
{
    printf("Enter _Packet::SetTimer\n");
    m_timer = timer;
    printf("Exit _Packet::SetTimer\n");
}

uint8_t _Packet::GetTimer(void) const
{
    printf("Enter and exit _Packet::GetTimer. Return m_timer\n");
    return m_timer;
}

void _NSPacket::Reset(void)
{
    printf("Enter _NSPacket::Reset\n");
    m_questions.clear();
    m_answers.clear();
    printf("Exit _NSPacket::Reset\n");
}

void _NSPacket::AddQuestion(WhoHas question)
{
    printf("Enter _NSPacket::AddQuestion\n");
    m_questions.push_back(question);
    printf("Exit _NSPacket::AddQuestion\n");
}

uint32_t _NSPacket::GetNumberQuestions(void) const
{
    printf("Enter and exit _NSPacket::GetNumberQuestions. return m_questions.size()\n");
    return m_questions.size();
}

WhoHas _NSPacket::GetQuestion(uint32_t index) const
{
    printf("Enter _NSPacket::GetQuestion\n");
    QCC_ASSERT(index < m_questions.size());
    printf("Exit _NSPacket::GetQuestion. Return m_questions[index]\n");
    return m_questions[index];
}


void _NSPacket::GetQuestion(uint32_t index, WhoHas** question)
{
    printf("Enter _NSPacket::GetQuestion\n");
    QCC_ASSERT(index < m_questions.size());
    *question = &m_questions[index];
    printf("Exit _NSPacket::GetQuestion\n");
}


void _NSPacket::AddAnswer(IsAt answer)
{
    printf("Enter _NSPacket::AddAnswer\n");
    m_answers.push_back(answer);
    printf("Exit _NSPacket::AddAnswer\n");
}

void _NSPacket::RemoveAnswer(uint32_t index)
{
    printf("Enter _NSPacket::RemoveAnswer\n");
    if (index < m_answers.size()) {
        m_answers.erase(m_answers.begin() + index);
    }
    printf("Exit _NSPacket::RemoveAnswer\n");
}

uint32_t _NSPacket::GetNumberAnswers(void) const
{
    printf("Enter and exit _NSPacket::GetNumberAnswers. Return m_answes.size()\n");
    return m_answers.size();
}

IsAt _NSPacket::GetAnswer(uint32_t index) const
{
    printf("Enter _NSPacket::GetAnswer\n");
    QCC_ASSERT(index < m_answers.size());
    printf("Exit _NSPacket::GetAnswer. Return m_answers[index]\n");
    return m_answers[index];
}

void _NSPacket::GetAnswer(uint32_t index, IsAt** answer)
{
    printf("Enter _NSPacket::GetAnswer\n");
    QCC_ASSERT(index < m_answers.size());
    *answer = &m_answers[index];
    printf("Exit _NSPacket::GetAnswer\n");
}

size_t _NSPacket::GetSerializedSize(void) const
{
    printf("Enter _NSPacket::GetSerializedSize\n");
    //
    // We have one octet for version, one four question count, one for answer
    // count and one for timer.  Four octets to start.
    //
    size_t size = 4;

    //
    // Let the questions data decide for themselves how long the question part
    // of the message will be.
    //
    for (uint32_t i = 0; i < m_questions.size(); ++i) {
        WhoHas whoHas = m_questions[i];
        size += whoHas.GetSerializedSize();
    }

    //
    // Let the answers decide for themselves how long the answer part
    // of the message will be.
    //
    for (uint32_t i = 0; i < m_answers.size(); ++i) {
        IsAt isAt = m_answers[i];
        size += isAt.GetSerializedSize();
    }
    printf("Exit _NSPacket::GetSerializedSize. Return size\n");
    return size;
}

size_t _NSPacket::Serialize(uint8_t* buffer) const
{
    printf("Enter _NSPacket::Serialize\n");
    QCC_DbgPrintf(("NSPacket::Serialize(): to buffer 0x%x", buffer));
    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is version
    //
    buffer[0] = m_version;
    QCC_DbgPrintf(("NSPacket::Serialize(): version = %d", m_version));
    size += 1;

    //
    // The second octet is the count of questions.
    //
    buffer[1] = static_cast<uint8_t>(m_questions.size());
    QCC_DbgPrintf(("NSPacket::Serialize(): QCount = %d", m_questions.size()));
    size += 1;

    //
    // The third octet is the count of answers.
    //
    buffer[2] = static_cast<uint8_t>(m_answers.size());
    QCC_DbgPrintf(("NSPacket::Serialize(): ACount = %d", m_answers.size()));
    size += 1;

    //
    // The fourth octet is the timer for the answers.
    //
    buffer[3] = GetTimer();
    QCC_DbgPrintf(("NSPacket::Serialize(): timer = %d", GetTimer()));
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t* p = &buffer[4];

    //
    // Let the questions push themselves out.
    //
    for (uint32_t i = 0; i < m_questions.size(); ++i) {
        QCC_DbgPrintf(("NSPacket::Serialize(): WhoHas::Serialize() question %d", i));
        WhoHas whoHas = m_questions[i];
        size_t questionSize = whoHas.Serialize(p);
        size += questionSize;
        p += questionSize;
    }

    //
    // Let the answers push themselves out.
    //
    for (uint32_t i = 0; i < m_answers.size(); ++i) {
        QCC_DbgPrintf(("NSPacket::Serialize(): IsAt::Serialize() answer %d", i));
        IsAt isAt = m_answers[i];
        size_t answerSize = isAt.Serialize(p);
        size += answerSize;
        p += answerSize;
    }
    printf("Exit _NSPacket::Serialize. Return size\n");
    return size;
}

size_t _NSPacket::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    printf("Enter _NSPacket::Deserialize\n");
    //
    // If there's not enough room in the buffer to get the fixed part out then
    // bail (one byte of version, one byte of question count, one byte of answer
    // count and one byte of timer).
    //
    if (bufsize < 4) {
        QCC_DbgPrintf(("NSPacket::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit _NSPacket::Deserialize. Return 0\n");
        return 0;
    }

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first octet is version.  We need to filter out bogus versions here
    // since we are going to promptly set this version in the included who-has
    // and is-at messages and they will assert that the versions we set actually
    // make sense.
    //
    uint8_t msgVersion;
    msgVersion = buffer[0] & 0xf;

    if (msgVersion != 0 && msgVersion != 1) {
        QCC_DbgPrintf(("Header::Deserialize(): Bad message version %d", msgVersion));
        printf("Exit _NSPacket::Deserialize. Return 0\n");
        return 0;
    }

    m_version = buffer[0];
    size += 1;

    //
    // The second octet is the count of questions.
    //
    uint8_t qCount = buffer[1];
    size += 1;

    //
    // The third octet is the count of answers.
    //
    uint8_t aCount = buffer[2];
    size += 1;

    //
    // The fourth octet is the timer for the answers.
    //
    SetTimer(buffer[3]);
    size += 1;

    //
    // From this point on, things are not at fixed addresses
    //
    uint8_t const* p = &buffer[4];
    bufsize -= 4;

    //
    // Now we need to read out <qCount> questions that the packet has told us
    // will be there.
    //
    for (uint8_t i = 0; i < qCount; ++i) {
        QCC_DbgPrintf(("NSPacket::Deserialize(): WhoHas::Deserialize() question %d", i));
        WhoHas whoHas;
        whoHas.SetVersion(m_version >> 4, m_version & 0xf);

        //
        // Tell the question to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t qSize = whoHas.Deserialize(p, bufsize);
        if (qSize == 0) {
            QCC_DbgPrintf(("NSPacket::Deserialize(): WhoHas::Deserialize():  Error"));
            printf("Exit _NSPacket::Deserialize. Return 0\n");
            return 0;
        }
        m_questions.push_back(whoHas);
        size += qSize;
        p += qSize;
        bufsize -= qSize;
    }

    //
    // Now we need to read out <aCount> answers that the packet has told us
    // will be there.
    //
    for (uint8_t i = 0; i < aCount; ++i) {
        QCC_DbgPrintf(("NSPacket::Deserialize(): IsAt::Deserialize() answer %d", i));
        IsAt isAt;
        isAt.SetVersion(m_version >> 4, m_version & 0xf);

        //
        // Tell the answer to read itself out.  If there's not enough buffer
        // it will complain by returning 0.  We pass the complaint on up.
        //
        size_t aSize = isAt.Deserialize(p, bufsize);
        if (aSize == 0) {
            QCC_DbgPrintf(("NSPacket::Deserialize(): IsAt::Deserialize():  Error"));
            printf("Exit _NSPacket::Deserialize. Return 0\n");
            return 0;
        }
        m_answers.push_back(isAt);
        size += aSize;
        p += aSize;
        bufsize -= aSize;
    }

    printf("Exit _NSPacket::Deserialize. Return size\n");
    return size;
}

//MDNSDomainName
void MDNSDomainName::SetName(String name)
{
    printf("Enter MDNSDomainName::SetName\n");
    m_name = name;
    printf("Exit MDNSDomainName::SetName\n");
}

String MDNSDomainName::GetName() const
{
    printf("Enter and exit MDNSDomainName::GetName. Return m_name\n");
    return m_name;
}

MDNSDomainName::MDNSDomainName()
{
    printf("Enter and exit MDNSDomainName::MDNSDomainName.\n");
}

MDNSDomainName::~MDNSDomainName()
{
    printf("Enter and exit MDNSDomainName::~MDNSDomainName.\n");
}

size_t MDNSDomainName::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter MDNSDomainName::GetSerializedSize.\n");
    size_t size = 0;
    String name = m_name;
    for (;;) {
        if (name.empty()) {
            size++;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            size++;
            size++;
            break;
        } else {
            offsets[name] = 0; /* 0 is used as a placeholder so that the serialized size is computed correctly */
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            size++;
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    printf("Exit MDNSDomainName::GetSerializedSize. return size\n");
    return size;
}

size_t MDNSDomainName::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSDomainName::Serialize\n");
    size_t size = 0;
    String name = m_name;
    for (;;) {
        if (name.empty()) {
            buffer[size++] = 0;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            buffer[size++] = 0xc0 | ((offsets[name] & 0xFF00) >> 8);
            buffer[size++] = (offsets[name] & 0xFF);
            break;
        } else {
            offsets[name] = size + headerOffset;
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            buffer[size++] = temp.length();
            memcpy(reinterpret_cast<void*>(&buffer[size]), const_cast<void*>(reinterpret_cast<const void*>(temp.c_str())), temp.size());
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    printf("Exit MDNSDomainName::Serialize. Return size\n");
    return size;
}

size_t MDNSDomainName::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSDomainName::Deserialize\n");
    m_name.clear();
    size_t size = 0;
    if (bufsize < 1) {
        QCC_DbgPrintf(("MDNSDomainName::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSDomainName::Deserialize. Return 0\n");
        return 0;
    }
    vector<uint32_t> offsets;

    while (bufsize > 0) {
        if (((buffer[size] & 0xc0) >> 6) == 3 && bufsize > 1) {
            uint32_t pointer = ((buffer[size] << 8 | buffer[size + 1]) & 0x3FFF);
            if (compressedOffsets.find(pointer) != compressedOffsets.end()) {
                if (m_name.length() > 0) {
                    m_name.append(1, '.');
                }
                m_name.append(compressedOffsets[pointer]);
                size += 2;
                break;
            } else {
                printf("Exit MDNSDomainName::Deserialize. Return 0\n");
                return 0;
            }
        }
        size_t temp_size = buffer[size++];
        bufsize--;
        //
        // If there's not enough data in the buffer then bail.
        //
        if (bufsize < temp_size) {
            QCC_DbgPrintf(("MDNSDomainName::Deserialize(): Insufficient bufsize %d", bufsize));
            printf("Exit MDNSDomainName::Deserialize. Return 0\n");
            return 0;
        }
        if (m_name.length() > 0) {
            m_name.append(1, '.');
        }
        if (temp_size > 0) {
            offsets.push_back(headerOffset + size - 1);
            m_name.append(reinterpret_cast<const char*>(buffer + size), temp_size);
            bufsize -= temp_size;
            size += temp_size;
        } else {
            break;
        }

    }

    for (uint32_t i = 0; i < offsets.size(); ++i) {
        compressedOffsets[offsets[i]] = m_name.substr(offsets[i] - headerOffset);
    }
    printf("Exit MDNSDomainName::Deserialize. Return size\n");
    return size;
}

//MDNSQuestion
MDNSQuestion::MDNSQuestion(qcc::String qName, uint16_t qType, uint16_t qClass) :
    m_qType(qType),
    m_qClass(qClass | QU_BIT)
{
    printf("Enter MDNSQuestion::MDNSQuestion\n");
    m_qName.SetName(qName);
    printf("Exit MDNSQuestion::MDNSQuestion\n");
}
void MDNSQuestion::SetQName(String qName)
{
    printf("Enter MDNSQuestion::SetQName\n");
    m_qName.SetName(qName);
    printf("Exit MDNSQuestion::SetQName\n");
}

String MDNSQuestion::GetQName()
{
    printf("Enter and Exit MDNSQuestion::GetQName. Return m_qName.GetName\n");
    return m_qName.GetName();
}

void MDNSQuestion::SetQType(uint16_t qType)
{
    printf("Enter MDNSQuestion::SetQName\n");
    m_qType = qType;
    printf("Exit MDNSQuestion::SetQName\n");
}

uint16_t MDNSQuestion::GetQType()
{
    printf("Enter and Exit MDNSQuestion::GetQType. Return m_qType\n");
    return m_qType;
}

void MDNSQuestion::SetQClass(uint16_t qClass)
{
    printf("Enter MDNSQuestion::SetQClass\n");
    m_qClass = qClass | QU_BIT;
    QCC_DbgPrintf(("%X %X ", qClass, m_qClass));
    printf("Exit MDNSQuestion::SetQClass\n");
}

uint16_t MDNSQuestion::GetQClass()
{
    printf("Enter and Exit MDNSQuestion::GetQClass. return m_qClass & ~QU_BIT;\n");
    return m_qClass & ~QU_BIT;
}

size_t MDNSQuestion::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter and Exit MDNSQuestion::GetSerializedSize. return m_qName.GetSerializedSize(offsets) + 4;\n");
    return m_qName.GetSerializedSize(offsets) + 4;
}

size_t MDNSQuestion::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSQuestion::Serialize\n");
    // Serialize the QNAME first
    size_t size = m_qName.Serialize(buffer, offsets, headerOffset);

    //Next two octets are QTYPE
    buffer[size] = (m_qType & 0xFF00) >> 8;
    buffer[size + 1] = (m_qType & 0xFF);

    //Next two octets are QCLASS
    buffer[size + 2] = (m_qClass & 0xFF00) >> 8;
    buffer[size + 3] = (m_qClass & 0xFF);
    QCC_DbgPrintf(("Set %X %X", buffer[size + 2], buffer[size + 3]));
    printf("Exit MDNSQuestion::Serialize. Return size+4\n");
    return size + 4;
}

size_t MDNSQuestion::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSQuestion::Deserialize\n");
    // Deserialize the QNAME first
    size_t size = m_qName.Deserialize(buffer, bufsize, compressedOffsets, headerOffset);
    if (size >= bufsize) {
        printf("Exit MDNSQuestion::Deserialize. Return 0\n");
        return 0;
    }
    bufsize -= size;
    if (size == 0 || bufsize < 4) {
        //Error while deserializing QNAME or insufficient buffer size
        QCC_DbgPrintf(("MDNSQuestion::Deserialize Error while deserializing QName"));
        printf("Exit MDNSQuestion::Deserialize. Return 0\n");
        return 0;
    }

    //Next two octets are QTYPE
    m_qType = (buffer[size] << 8) | buffer[size + 1];
    size += 2;

    //Next two octets are QCLASS
    m_qClass = (buffer[size] << 8) | buffer[size + 1];
    size += 2;
    printf("Exit MDNSQuestion::Deserialize. Return size\n");
    return size;
}

MDNSRData::~MDNSRData()
{
    printf("Enter and exit MDNSRData::~MDNSRData\n");
}

//MDNSResourceRecord
MDNSResourceRecord::MDNSResourceRecord() : m_rdata(NULL)
{
    printf("Enter and exit MDNSResourceRecord::MDNSResourceRecord\n");
}

MDNSResourceRecord::MDNSResourceRecord(qcc::String domainName, RRType rrType, RRClass rrClass, uint16_t ttl, MDNSRData* rdata) :

    m_rrType(rrType),
    m_rrClass(rrClass),
    m_rrTTL(ttl)
{
    printf("Enter MDNSResourceRecord::MDNSResourceRecord\n");
    m_rrDomainName.SetName(domainName);
    m_rdata = rdata->GetDeepCopy();
    printf("Exit MDNSResourceRecord::MDNSResourceRecord\n");
}

MDNSResourceRecord::MDNSResourceRecord(const MDNSResourceRecord& r) :
    m_rrDomainName(r.m_rrDomainName),
    m_rrType(r.m_rrType),
    m_rrClass(r.m_rrClass),
    m_rrTTL(r.m_rrTTL)
{
    printf("Enter MDNSResourceRecord::MDNSResourceRecord\n");
    m_rdata = r.m_rdata->GetDeepCopy();
    printf("Exit MDNSResourceRecord::MDNSResourceRecord\n");
}

MDNSResourceRecord& MDNSResourceRecord::operator=(const MDNSResourceRecord& r)
{
    if (this != &r) {
        m_rrDomainName = r.m_rrDomainName;
        m_rrType = r.m_rrType;
        m_rrClass = r.m_rrClass;
        m_rrTTL = r.m_rrTTL;
        if (m_rdata) {
            delete m_rdata;
        }
        m_rdata = r.m_rdata->GetDeepCopy();
    }
    return *this;
}

MDNSResourceRecord::~MDNSResourceRecord()
{

    printf("Enter MDNSResourceRecord::~MDNSResourceRecord\n");
    if (m_rdata) {
        delete m_rdata;
        m_rdata = NULL;
    }
    printf("Exit MDNSResourceRecord::~MDNSResourceRecord\n");
}

size_t MDNSResourceRecord::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter MDNSResourceRecord::GetSerializedSize\n");
    QCC_ASSERT(m_rdata);
    size_t size = m_rrDomainName.GetSerializedSize(offsets);
    size += 8;
    size += m_rdata->GetSerializedSize(offsets);
    printf("Exit MDNSResourceRecord::GetSerializedSize\n");
    return size;
}

size_t MDNSResourceRecord::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSResourceRecord::Serialize\n");
    QCC_ASSERT(m_rdata);

    //
    // Serialize the NAME first
    //
    size_t size = m_rrDomainName.Serialize(buffer, offsets, headerOffset);

    //Next two octets are TYPE
    buffer[size] = (m_rrType & 0xFF00) >> 8;
    buffer[size + 1] = (m_rrType & 0xFF);

    //Next two octets are CLASS
    buffer[size + 2] = (m_rrClass & 0xFF00) >> 8;
    buffer[size + 3] = (m_rrClass & 0xFF);

    //Next four octets are TTL
    buffer[size + 4] = (m_rrTTL & 0xFF000000) >> 24;
    buffer[size + 5] = (m_rrTTL & 0xFF0000) >> 16;
    buffer[size + 6] = (m_rrTTL & 0xFF00) >> 8;
    buffer[size + 7] = (m_rrTTL & 0xFF);
    size += 8;

    uint8_t* p = &buffer[size];
    size += m_rdata->Serialize(p, offsets, headerOffset + size);
    printf("Exit MDNSResourceRecord::Serialize. Return size\n");
    return size;
}

size_t MDNSResourceRecord::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSResourceRecord::Deserialize\n");
    if (m_rdata) {
        delete m_rdata;
        m_rdata = NULL;
    }
    //
    // Deserialize the NAME first
    //
    size_t size = m_rrDomainName.Deserialize(buffer, bufsize, compressedOffsets, headerOffset);
    if (size == 0 || bufsize < 8) {
        //error
        QCC_DbgPrintf((" MDNSResourceRecord::Deserialize() Error occured while deserializing domain name or insufficient buffer"));
        printf("Exit MDNSResourceRecord::Deserialize. Return 0\n");
        return 0;
    }

    //Next two octets are TYPE
    if (size > bufsize || ((size + 8) > bufsize)) {
        printf("Enter MDNSResourceRecord::Deserialize. Return 0\n");
        return 0;
    }
    m_rrType = (RRType)((buffer[size] << 8) | buffer[size + 1]);
    switch (m_rrType) {
    case A:
        m_rdata = new MDNSARData();
        break;

    case NS:
    case MD:
    case MF:
    case CNAME:
    case MB:
    case MG:
    case MR:
    case PTR:
        m_rdata = new MDNSPtrRData();
        break;

    case RNULL:
        m_rdata = new MDNSDefaultRData();
        break;

    case HINFO:
    case TXT:
        m_rdata = new MDNSTextRData();
        break;

    case AAAA:
        m_rdata = new MDNSAAAARData();
        break;

    case SRV:
        m_rdata = new MDNSSrvRData();
        break;

    default:
        m_rdata = new MDNSDefaultRData();
        QCC_DbgPrintf(("Ignoring unrecognized rrtype %d", m_rrType));
        break;
    }

    if (!m_rdata) {
        printf("Exit MDNSResourceRecord::Deserialize. Return 0\n");
        return 0;
    }
    //Next two octets are CLASS
    m_rrClass = (RRClass)((buffer[size + 2] << 8) | buffer[size + 3]);

    //Next four octets are TTL
    m_rrTTL = (buffer[size + 4] << 24) | (buffer[size + 5] << 16) | (buffer[size + 6] << 8) | buffer[size + 7];
    bufsize -= (size + 8);
    size += 8;
    headerOffset += size;
    uint8_t const* p = &buffer[size];
    size_t processed = m_rdata->Deserialize(p, bufsize, compressedOffsets, headerOffset);
    if (!processed) {
        QCC_DbgPrintf(("MDNSResourceRecord::Deserialize() Error occured while deserializing resource data"));
        printf("Exit MDNSResourceRecord::Deserialize. Return 0\n");
        return 0;
    }
    size += processed;
    printf("Exit MDNSResourceRecord::Deserialize. Return size\n");
    return size;
}

void MDNSResourceRecord::SetDomainName(qcc::String domainName)
{
    printf("Enter MDNSResourceRecord::SetDomainName\n");
    m_rrDomainName.SetName(domainName);
    printf("Exit MDNSResourceRecord::SetDomainName\n");
}

qcc::String MDNSResourceRecord::GetDomainName() const
{
    printf("Enter and Exit MDNSResourceRecord::GetDomainName. Return m_rrDomainName.GetName()\n");
    return m_rrDomainName.GetName();
}

void MDNSResourceRecord::SetRRType(RRType rrtype)
{
    printf("Enter MDNSResourceRecord::SetRRType\n");
    m_rrType = rrtype;
    printf("Exit MDNSResourceRecord::SetRRType\n");
}

MDNSResourceRecord::RRType MDNSResourceRecord::GetRRType() const
{
    printf("Enter and Exit MDNSResourceRecord::GetRRType. Return m_rrType\n");
    return m_rrType;
}

void MDNSResourceRecord::SetRRClass(RRClass rrclass)
{
    printf("Enter MDNSResourceRecord::SetRRClass\n");
    m_rrClass = rrclass;
    printf("Exit MDNSResourceRecord::SetRRClass\n");
}

MDNSResourceRecord::RRClass MDNSResourceRecord::GetRRClass() const
{
    printf("Enter and Exit MDNSResourceRecord::GetRRClass. Return m_rrClass\n");
    return m_rrClass;
}

void MDNSResourceRecord::SetRRttl(uint16_t ttl)
{
    printf("Enter MDNSResourceRecord::SetRRttl\n");
    m_rrTTL = ttl;
    printf("Exit MDNSResourceRecord::SetRRttl\n");
}

uint16_t MDNSResourceRecord::GetRRttl() const
{
    printf("Enter and Exit MDNSResourceRecord::GetRRttl. Return m_rrTTL\n");
    return m_rrTTL;
}

void MDNSResourceRecord::SetRData(MDNSRData* rdata)
{
    printf("Enter MDNSResourceRecord::SetRData\n");
    if (m_rdata) {
        delete m_rdata;
        m_rdata = NULL;
    }
    m_rdata = rdata;
    printf("Exit MDNSResourceRecord::SetRData\n");
}

MDNSRData* MDNSResourceRecord::GetRData()
{
    printf("Enter and exit MDNSResourceRecord::GetRData. Return m_rdata\n");
    return m_rdata;
}

//MDNSDefaultRData
size_t MDNSDefaultRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter MDNSDefaultRData::GetSerializedSize\n");
    QCC_UNUSED(offsets);
    printf("Exit MDNSResourceRecord::GetSerializedSize. Return 0\n");
    return 0;
}

size_t MDNSDefaultRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSDefaultRData::Serialize\n");
    QCC_UNUSED(buffer);
    QCC_UNUSED(offsets);
    QCC_UNUSED(headerOffset);
    printf("Exit MDNSDefaultRData::Serialize. Return 0\n");
    return 0;
}

size_t MDNSDefaultRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSDefaultRData::Deserialize\n");
    QCC_UNUSED(compressedOffsets);
    QCC_UNUSED(headerOffset);
    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSDefaultRData::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSDefaultRData::Deserialize. Return 0\n");
        return 0;
    }
    uint16_t rdlen = buffer[0] << 8 | buffer[1];
    bufsize -= 2;
    if (bufsize < rdlen) {
        QCC_DbgPrintf(("MDNSDefaultRData::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSDefaultRData::Deserialize. Return 0\n");
        return 0;
    }
    printf("Exit MDNSDefaultRData::Deserialize. Return rdlen + 2\n");
    return rdlen + 2;
}

//MDNSTextRData
const uint16_t MDNSTextRData::TXTVERS = 0;

MDNSTextRData::MDNSTextRData(uint16_t version, bool uniquifyKeys)
    : version(version), uniquifier(uniquifyKeys ? 1 : 0)
{
    printf("Enter MDNSTextRData::MDNSTextRData\n");
    m_fields["txtvers"] = U32ToString(version);
    printf("Exit MDNSTextRData::MDNSTextRData\n");
}

void MDNSTextRData::SetUniqueCount(uint16_t count)
{
    printf("Enter MDNSTextRData::SetUniqueCount\n");
    uniquifier = count;
    printf("Exit MDNSTextRData::SetUniqueCount\n");
}

uint16_t MDNSTextRData::GetUniqueCount()
{
    printf("Enter and exit MDNSTextRData::GetUniqueCount. Return uniquifier\n");
    return uniquifier;
}

void MDNSTextRData::Reset()
{
    printf("Enter MDNSTextRData::Reset\n");
    m_fields.clear();
    m_fields["txtvers"] = U32ToString(version);
    if (uniquifier) {
        uniquifier = 1;
    }
    printf("Exit MDNSTextRData::Reset\n");
}

void MDNSTextRData::RemoveEntry(qcc::String key)
{
    printf("Enter MDNSTextRData::RemoveEntry\n");
    m_fields.erase(key);
    printf("Exit MDNSTextRData::RemoveEntry\n");
}

void MDNSTextRData::SetValue(String key, String value, bool shared)
{
    printf("Enter MDNSTextRData::SetValue\n");
    if (uniquifier && !shared) {
        key += "_" + U32ToString(uniquifier++);
    }
    m_fields[key] = value;
    printf("Exit MDNSTextRData::SetValue\n");
}

void MDNSTextRData::SetValue(String key, bool shared)
{
    printf("Enter MDNSTextRData::SetValue\n");
    if (uniquifier && !shared) {
        key += "_" + U32ToString(uniquifier++);
    }
    m_fields[key] = String();
    printf("Exit MDNSTextRData::SetValue\n");
}

String MDNSTextRData::GetValue(String key)
{
    printf("Enter MDNSTextRData::GetValue\n");
    if (m_fields.find(key) != m_fields.end()) {
        printf("Exit MDNSTextRData::GetValue. return m_fields[key]\n");
        return m_fields[key];
    } else {
        printf("Exit MDNSTextRData::GetValue. return ""\n");
        return "";
    }
}

bool MDNSTextRData::HasKey(qcc::String key)
{
    printf("Enter MDNSTextRData::HasKey\n");
    if (m_fields.find(key) != m_fields.end()) {
        printf("Exit MDNSTextRData::HasKey. Return true\n");
        return true;
    } else {
        printf("Exit MDNSTextRData::HasKey. Return false\n");
        return false;
    }
}

uint16_t MDNSTextRData::GetU16Value(String key) {
    printf("Enter MDNSTextRData::GetU16Value\n");
    if (m_fields.find(key) != m_fields.end()) {
        printf("Exit MDNSTextRData::GetU16Value. Return StringToU32(m_fields[key])\n");
        return StringToU32(m_fields[key]);
    } else {
        printf("Exit MDNSTextRData::GetU16Value. Return 0\n");
        return 0;
    }
}

uint16_t MDNSTextRData::GetNumFields(String key)
{
    printf("Enter MDNSTextRData::GetNumFields\n");
    key += "_";
    uint16_t numNames = 0;
    for (Fields::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find(key) == 0) {
            ++numNames;
        }
    }
    printf("Exit MDNSTextRData::GetNumFields\n");
    return numNames;
}

qcc::String MDNSTextRData::GetFieldAt(String key, int i)
{
    printf("Enter MDNSTextRData::GetFieldAt\n");
    key += "_";
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find(key) == 0 && (i-- == 0)) {
            break;
        }
    }
    if (it != m_fields.end()) {
        printf("Exit MDNSTextRData::GetFieldAt. Return it->second\n");
        return it->second;
    } else {
        printf("Exit MDNSTextRData::GetFieldAt. Return String()\n");
        return String();
    }
}

void MDNSTextRData::RemoveFieldAt(String key, int i)
{
    printf("Enter MDNSTextRData::RemoveFieldAt\n");
    key += "_";
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find(key) == 0 && (i-- == 0)) {
            break;
        }
    }
    if (it != m_fields.end()) {
        MDNSTextRData::RemoveEntry(it->first);
    }
    printf("Exit MDNSTextRData::RemoveFieldAt\n");
}

size_t MDNSTextRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter MDNSTextRData::GetSerializedSize\n");
    QCC_UNUSED(offsets);
    size_t rdlen = 0;
    Fields::const_iterator it = m_fields.begin();

    while (it != m_fields.end()) {
        String str = it->first;
        if (!it->second.empty()) {
            str += "=" + it->second;
        }
        rdlen += str.length() + 1;
        it++;
    }
    printf("Exit MDNSTextRData::GetSerializedSize. Return rdlen + 2\n");
    return rdlen + 2;
}

size_t MDNSTextRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSTextRData::Serialize\n");
    QCC_UNUSED(offsets);
    QCC_UNUSED(headerOffset);

    size_t rdlen = 0;
    uint8_t* p = &buffer[2];

    //
    // txtvers must appear first in the record
    //
    Fields::const_iterator txtvers = m_fields.find("txtvers");
    QCC_ASSERT(txtvers != m_fields.end());
    String str = txtvers->first + "=" + txtvers->second;
    *p++ = str.length();
    memcpy(reinterpret_cast<void*>(p), const_cast<void*>(reinterpret_cast<const void*>(str.c_str())), str.length());
    p += str.length();
    rdlen += str.length() + 1;

    //
    // Then we rely on the Fields comparison object so make sure things are
    // serialized in the proper order
    //
    for (Fields::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it == txtvers) {
            continue;
        }
        str = it->first;
        if (!it->second.empty()) {
            str += "=" + it->second;
        }
        *p++ = str.length();
        memcpy(reinterpret_cast<void*>(p), const_cast<void*>(reinterpret_cast<const void*>(str.c_str())), str.length());
        p += str.length();
        rdlen += str.length() + 1;
    }

    buffer[0] = (rdlen & 0xFF00) >> 8;
    buffer[1] = (rdlen & 0xFF);

    printf("Exit MDNSTextRData::Serialize. Return rdlen + 2\n");
    return rdlen + 2;
}

size_t MDNSTextRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSTextRData::Deserialize\n");
    QCC_UNUSED(compressedOffsets);
    QCC_UNUSED(headerOffset);
    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSTextRData::Deserialize. Return 0\n");
        return 0;
    }
    uint16_t rdlen = buffer[0] << 8 | buffer[1];

    bufsize -= 2;
    size_t size = 2 + rdlen;

    //
    // If there's not enough data in the buffer then bail.
    //
    if (bufsize < rdlen) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSTextRData::Deserialize. Return 0\n");
        return 0;
    }
    uint8_t const* p = &buffer[2];
    while (rdlen > 0 && bufsize > 0) {
        uint8_t sz = *p++;
        String str;
        bufsize--;
        if (bufsize < sz) {
            QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
            printf("Exit MDNSTextRData::Deserialize. Return 0\n");
            return 0;
        }
        if (sz) {
            str.assign(reinterpret_cast<const char*>(p), sz);
        }
        size_t eqPos = str.find_first_of('=', 0);
        if (eqPos != String::npos) {
            m_fields[str.substr(0, eqPos)] = str.substr(eqPos + 1);
        } else {
            m_fields[str.substr(0, eqPos)] = String();
        }
        p += sz;
        rdlen -= sz + 1;
        bufsize -= sz;
    }

    if (rdlen != 0) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Mismatched RDLength"));
        printf("Exit MDNSTextRData::Deserialize. Return 0\n");
        return 0;
    }
    printf("Exit MDNSTextRData::Deserialize. Return size\n");
    return size;
}

//MDNSARecord
void MDNSARData::SetAddr(qcc::String ipAddr)
{
    printf("Enter MDNSARData::SetAddr\n");
    m_ipv4Addr = ipAddr;
    printf("Exit MDNSARData::SetAddr\n");
}

qcc::String MDNSARData::GetAddr()
{
    printf("Enter and Exit MDNSARData::GetAddr. Return m_ipv4Addr\n");
    return m_ipv4Addr;
}

size_t MDNSARData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter MDNSARData::GetSerializedSize\n");
    QCC_UNUSED(offsets);
    printf("Exit MDNSARData::GetSerializedSize. Return 4 + 2\n");
    //4 bytes for address, 2 bytes length
    return 4 + 2;
}

size_t MDNSARData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSARData::Serialize\n");
    QCC_UNUSED(offsets);
    QCC_UNUSED(headerOffset);

    buffer[0] = 0;
    buffer[1] = 4;
    uint8_t* p = &buffer[2];
    QStatus status = qcc::IPAddress::StringToIPv4(m_ipv4Addr, p, 4);
    if (status != ER_OK) {
        QCC_DbgPrintf(("MDNSARData::Serialize Error occured during conversion of String to IPv4 address"));
        printf("Exit MDNSARData::Serialize. Return 0\n");
        return 0;
    }
    printf("Exit MDNSARData::Serialize. Return 6\n");
    return 6;
}

size_t MDNSARData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSARData::Deserialize\n");
    QCC_UNUSED(compressedOffsets);
    QCC_UNUSED(headerOffset);

    if (bufsize < 6) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSARData::Deserialize. Return 0\n");
        return 0;
    }
    if (buffer[0] != 0 || buffer[1] != 4) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Invalid RDLength"));
        printf("Exit MDNSARData::Deserialize. Return 0\n");
        return 0;

    }
    uint8_t const* p = &buffer[2];
    m_ipv4Addr = qcc::IPAddress::IPv4ToString(p);
    bufsize -= 6;
    printf("Exit MDNSARData::Deserialize. Return 6\n");
    return 6;
}

//MDNSAAAARecord
void MDNSAAAARData::SetAddr(qcc::String ipAddr)
{
    printf("Enter MDNSAAAARData::SetAddr\n");
    m_ipv6Addr = ipAddr;
    printf("Exit MDNSAAAARData::SetAddr\n");
}

qcc::String MDNSAAAARData::GetAddr() const
{
    printf("Enter and exit MDNSAAAARData::GetAddr. Return m_ipv6Addr\n");
    return m_ipv6Addr;
}

size_t MDNSAAAARData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter MDNSAAAARData::GetSerializedSize\n");
    QCC_UNUSED(offsets);
    //16 bytes for address, 2 bytes length
    printf("Exit MDNSAAAARData::GetSerializedSize. Return 16 + 2\n");
    return 16 + 2;
}

size_t MDNSAAAARData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSAAAARData::Serialize\n");
    QCC_UNUSED(offsets);
    QCC_UNUSED(headerOffset);

    buffer[0] = 0;
    buffer[1] = 16;
    uint8_t* p = &buffer[2];
    qcc::IPAddress::StringToIPv6(m_ipv6Addr, p, 16);
    printf("Exit MDNSAAAARData::Serialize. Return 18\n");
    return 18;
}

size_t MDNSAAAARData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSAAAARData::Deserialize\n");
    QCC_UNUSED(compressedOffsets);
    QCC_UNUSED(headerOffset);

    if (bufsize < 18) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        return 0;
    }
    if (buffer[0] != 0 || buffer[1] != 16) {
        QCC_DbgPrintf(("MDNSTextRecord::Deserialize(): Invalid RDLength"));
        return 0;
    }
    uint8_t const* p = &buffer[2];
    m_ipv6Addr = qcc::IPAddress::IPv6ToString(p);
    bufsize -= 18;
    printf("Exit MDNSAAAARData::Deserialize. Return 18\n");
    return 18;
}

//MDNSPtrRData
void MDNSPtrRData::SetPtrDName(qcc::String rdataStr)
{
    printf("Enter MDNSPtrRData::SetPtrDName\n");
    m_rdataStr = rdataStr;
    printf("Exit MDNSPtrRData::SetPtrDName\n");
}

qcc::String MDNSPtrRData::GetPtrDName() const
{
    printf("Enter and Exit MDNSPtrRData::GetPtrDName. Return m_rdataStr\n");
    return m_rdataStr;
}

size_t MDNSPtrRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    printf("Enter MDNSPtrRData::GetSerializedSize\n");
    size_t size = 2;
    String name = m_rdataStr;
    for (;;) {
        if (name.empty()) {
            size++;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            size++;
            size++;
            break;
        } else {
            offsets[name] = 0; /* 0 is used as a placeholder so that the serialized size is computed correctly */
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            size++;
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    printf("Exit MDNSPtrRData::GetSerializedSize. Return size\n");
    return size;
}

size_t MDNSPtrRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSPtrRData::Serialize\n");
    size_t size = 2;
    String name = m_rdataStr;
    for (;;) {
        if (name.empty()) {
            buffer[size++] = 0;
            break;
        } else if (offsets.find(name) != offsets.end()) {
            buffer[size++] = 0xc0 | ((offsets[name] & 0xFF00) >> 8);
            buffer[size++] = (offsets[name] & 0xFF);
            break;
        } else {
            offsets[name] = size + headerOffset;
            size_t newPos = name.find_first_of('.');
            String temp = name.substr(0, newPos);
            buffer[size++] = temp.length();
            memcpy(reinterpret_cast<void*>(&buffer[size]), const_cast<void*>(reinterpret_cast<const void*>(temp.c_str())), temp.size());
            size += temp.length();
            size_t pos = (newPos == String::npos) ? String::npos : (newPos + 1);
            name = name.substr(pos);
        }
    }
    buffer[0] = ((size - 2) & 0xFF00) >> 8;
    buffer[1] = ((size - 2) & 0xFF);
    printf("Exit MDNSPtrRData::Serialize. Return size\n");
    return size;
}

size_t MDNSPtrRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{
    printf("Enter MDNSPtrRData::Deserialize\n");
    m_rdataStr.clear();
    //
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSPtrRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSPtrRData::Deserialize. Return 0\n");
        return 0;
    }

    uint16_t szStr = buffer[0] << 8 | buffer[1];
    bufsize -= 2;

    size_t size = 2;

    vector<uint32_t> offsets;

    // If there's not enough data in the buffer then bail.
    //
    if (bufsize < szStr) {
        QCC_DbgPrintf(("MDNSPtrRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSPtrRData::Deserialize. Return 0\n");
        return 0;
    }

    while (bufsize > 0) {
        if (((buffer[size] & 0xc0) >> 6) == 3 && bufsize > 1) {
            uint32_t pointer = ((buffer[size] << 8 | buffer[size + 1]) & 0x3FFF);
            if (compressedOffsets.find(pointer) != compressedOffsets.end()) {
                if (m_rdataStr.length() > 0) {
                    m_rdataStr.append(1, '.');
                }
                m_rdataStr.append(compressedOffsets[pointer]);
                size += 2;
                break;
            } else {
                printf("Exit MDNSPtrRData::Deserialize. Return 0\n");
                return 0;
            }
        }
        size_t temp_size = buffer[size++];
        bufsize--;
        //
        // If there's not enough data in the buffer then bail.
        //
        if (bufsize < temp_size) {
            QCC_DbgPrintf(("MDNSPtrRecord::Deserialize(): Insufficient bufsize %d", bufsize));
            printf("Exit MDNSPtrRData::Deserialize. Return 0\n");
            return 0;
        }
        if (m_rdataStr.length() > 0) {
            m_rdataStr.append(1, '.');
        }
        if (temp_size > 0) {
            offsets.push_back(headerOffset + size - 1);
            m_rdataStr.append(reinterpret_cast<const char*>(buffer + size), temp_size);
            bufsize -= temp_size;
            size += temp_size;
        } else {
            break;
        }
    }

    for (uint32_t i = 0; i < offsets.size(); ++i) {
        compressedOffsets[offsets[i]] = m_rdataStr.substr(offsets[i] - 2 - headerOffset);
    }
    printf("Exit MDNSPtrRData::Deserialize. Return size\n");
    return size;
}

//MDNSSrvRData
MDNSSrvRData::MDNSSrvRData(uint16_t priority, uint16_t weight, uint16_t port, qcc::String target) :
    m_priority(priority),
    m_weight(weight),
    m_port(port)
{
    printf("Enter MDNSSrvRData::MDNSSrvRData\n");
    m_target.SetName(target);
    printf("Exit MDNSSrvRData::MDNSSrvRData\n");
}

void MDNSSrvRData::SetPriority(uint16_t priority)
{
    printf("Enter MDNSSrvRData::SetPriority\n");
    m_priority = priority;
    printf("Exit MDNSSrvRData::SetPriority\n");
}

uint16_t MDNSSrvRData::GetPriority()  const
{
    printf("Enter and exit MDNSSrvRData::GetPriority. Return m_priority\n");
    return m_priority;
}

void MDNSSrvRData::SetWeight(uint16_t weight)
{
    printf("Enter MDNSSrvRData::SetWeight\n");
    m_weight = weight;
    printf("Exit MDNSSrvRData::SetWeight\n");
}

uint16_t MDNSSrvRData::GetWeight()  const
{
    printf("Enter MDNSSrvRData::GetWeight. Return m_weight\n");
    return m_weight;
}

void MDNSSrvRData::SetPort(uint16_t port)
{
    printf("Enter MDNSSrvRData::SetPort\n");
    m_port = port;
    printf("Exit MDNSSrvRData::SetPort\n");
}

uint16_t MDNSSrvRData::GetPort()  const
{
    printf("Enter and exit MDNSSrvRData::GetPort. Return m_port\n");
    return m_port;
}

void MDNSSrvRData::SetTarget(qcc::String target)
{
    printf("Enter MDNSSrvRData::SetTarget\n");
    m_target.SetName(target);
    printf("Exit MDNSSrvRData::SetTarget\n");
}

qcc::String MDNSSrvRData::GetTarget()  const
{
    printf("Enter and exit MDNSSrvRData::GetTarget. Return m_target.GetName()\n");    
    return m_target.GetName();
}

size_t MDNSSrvRData::GetSerializedSize(std::map<qcc::String, uint32_t>& offsets) const
{
    //2 bytes length
    printf("Enter and exit MDNSSrvRData::GetSerializedSize. Return 2 + 6 + m_target.GetSerializedSize(offsets)\n");
    return 2 + 6 + m_target.GetSerializedSize(offsets);
}

size_t MDNSSrvRData::Serialize(uint8_t* buffer, std::map<qcc::String, uint32_t>& offsets, uint32_t headerOffset) const
{
    printf("Enter MDNSSrvRData::Serialize\n");
    //length filled in after we know it below

    //priority
    buffer[2] = (m_priority & 0xFF00) >> 8;
    buffer[3] = (m_priority & 0xFF);

    //weight
    buffer[4] = (m_weight & 0xFF00) >> 8;
    buffer[5] = (m_weight & 0xFF);

    //port
    buffer[6] = (m_port & 0xFF00) >> 8;
    buffer[7] = (m_port & 0xFF);
    size_t size = 2 + 6;

    uint8_t* p = &buffer[size];
    size += m_target.Serialize(p, offsets, headerOffset + size);

    uint16_t length = size - 2;
    buffer[0] = (length & 0xFF00) >> 8;
    buffer[1] = (length & 0xFF);
    printf("Exit MDNSSrvRData::Serialize. Return size\n");
    return size;
}

size_t MDNSSrvRData::Deserialize(uint8_t const* buffer, uint32_t bufsize, std::map<uint32_t, qcc::String>& compressedOffsets, uint32_t headerOffset)
{

//
    // If there's not enough data in the buffer to even get the string size out
    // then bail.
    //
    printf("Enter MDNSSrvRData::Deserialize\n");
    if (bufsize < 2) {
        QCC_DbgPrintf(("MDNSSrvRecord::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSSrvRData::Deserialize. Return 0\n");
        return 0;
    }

    uint16_t length = buffer[0] << 8 | buffer[1];
    bufsize -= 2;

    if (bufsize < length || length < 6) {
        QCC_DbgPrintf(("MDNSSrvRecord::Deserialize(): Insufficient bufsize %d or invalid length %d", bufsize, length));
        printf("Exit MDNSSrvRData::Deserialize. Return 0\n");
        return 0;
    }
    m_priority = buffer[2] << 8 | buffer[3];
    bufsize -= 2;

    m_weight = buffer[4] << 8 | buffer[5];
    bufsize -= 2;

    m_port = buffer[6] << 8 | buffer[7];
    bufsize -= 2;

    size_t size = 8;
    headerOffset += 8;
    uint8_t const* p = &buffer[size];
    size += m_target.Deserialize(p, bufsize, compressedOffsets, headerOffset);
    printf("Enter MDNSSrvRData::Deserialize. Return size\n");
    return size;
}

//MDNSAdvertiseRecord
void MDNSAdvertiseRData::Reset()
{
    printf("Enter MDNSAdvertiseRData::Reset\n");
    MDNSTextRData::Reset();
    printf("Exit MDNSAdvertiseRData::Reset\n");
}

void MDNSAdvertiseRData::SetTransport(TransportMask tm)
{
    printf("Enter MDNSAdvertiseRData::SetTransport\n");
    MDNSTextRData::SetValue("t", U32ToString(tm, 16));
    printf("Exit MDNSAdvertiseRData::SetTransport\n");
}

void MDNSAdvertiseRData::AddName(qcc::String name)
{
    printf("Enter MDNSAdvertiseRData::AddName\n");
    MDNSTextRData::SetValue("n", name);
    printf("Exit MDNSAdvertiseRData::AddName\n");
}

void MDNSAdvertiseRData::SetValue(String key, String value)
{
    printf("Enter MDNSAdvertiseRData::SetValue\n");
    //
    // Commonly used keys name and implements get abbreviated over the air.
    //
    if (key == "name") {
        MDNSTextRData::SetValue("n", value);
    }  else if (key == "transport") {
        MDNSTextRData::SetValue("t", value);
    } else if (key == "implements") {
        MDNSTextRData::SetValue("i", value);
    } else {
        MDNSTextRData::SetValue(key, value);
    }
    printf("Exit MDNSAdvertiseRData::SetValue\n");
}

void MDNSAdvertiseRData::SetValue(String key)
{
    printf("Enter MDNSAdvertiseRData::SetValue\n");
    MDNSTextRData::SetValue(key);
    printf("Exit MDNSAdvertiseRData::SetValue\n");
}

uint16_t MDNSAdvertiseRData::GetNumFields()
{
    printf("Enter and exit MDNSAdvertiseRData::GetNumFields. Return m_fields.size()\n");
    return m_fields.size();
}
uint16_t MDNSAdvertiseRData::GetNumNames(TransportMask transportMask)
{
    printf("Enter MDNSAdvertiseRData::GetNumNames\n");
    uint16_t numNames = 0;
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find("t_") != String::npos && (StringToU32(it->second, 16) == transportMask)) {
            it++;
            while (it != m_fields.end() && it->first.find("t_") == String::npos) {
                if (it->first.find("n_") != String::npos) {
                    numNames++;
                }
                it++;
            }
            break;
        }
    }
    printf("Exit MDNSAdvertiseRData::GetNumNames. Return numNames\n");
    return numNames;
}

qcc::String MDNSAdvertiseRData::GetNameAt(TransportMask transportMask, int index)
{
    printf("Enter MDNSAdvertiseRData::GetNameAt\n");
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find("t_") != String::npos && (StringToU32(it->second, 16) == transportMask)) {
            it++;
            while (it != m_fields.end() && it->first.find("t_") == String::npos) {
                if (it->first.find("n_") != String::npos && index-- == 0) {
                    printf("Exit MDNSAdvertiseRData::GetNameAt. Return it->second\n");
                    return it->second;
                }
                it++;
            }
            break;
        }
    }
    printf("Exit MDNSAdvertiseRData::GetNameAt. Return ""\n");
    return "";
}

void MDNSAdvertiseRData::RemoveNameAt(TransportMask transportMask, int index)
{
    printf("Enter MDNSAdvertiseRData::RemoveNameAt\n");
    Fields::const_iterator it;
    for (it = m_fields.begin(); it != m_fields.end(); ++it) {
        if (it->first.find("t_") != String::npos && (StringToU32(it->second, 16) == transportMask)) {
            uint32_t numNames = 0;
            Fields::const_iterator trans = it;
            it++;
            while (it != m_fields.end() && it->first.find("t_") == String::npos) {
                Fields::const_iterator nxt = it;
                nxt++;
                if (it->first.find("n_") != String::npos) {
                    if (index-- == 0) {
                        MDNSTextRData::RemoveEntry(it->first);
                    } else {
                        numNames++;
                    }
                }
                it = nxt;
            }
            if (!numNames) {
                MDNSTextRData::RemoveEntry(trans->first);
            }
            break;
        }
    }
    printf("Exit MDNSAdvertiseRData::RemoveNameAt\n");

}
std::pair<qcc::String, qcc::String> MDNSAdvertiseRData::GetFieldAt(int i)
{
    printf("Enter MDNSAdvertiseRData::GetFieldAt\n");
    Fields::const_iterator it = m_fields.begin();
    while (i-- && it != m_fields.end()) {
        ++it;
    }
    if (it == m_fields.end()) {
        printf("Exit MDNSAdvertiseRData::GetFieldAt. Return pair<String, String>(***);\n");
        return pair<String, String>("", "");
    }
    String key = it->first;
    key = key.substr(0, key.find_last_of_std('_'));
    if (key == "n") {
        key = "name";
    } else if (key == "i") {
        key = "implements";
    } else if (key == "t") {
        key = "transport";
    }
    printf("Exit MDNSAdvertiseRData::GetFieldAt. Return pair<String, String>(key, it->second)\n");
    return pair<String, String>(key, it->second);
}

//MDNSSearchRecord
MDNSSearchRData::MDNSSearchRData(qcc::String name, uint16_t version)
    : MDNSTextRData(version, true)
{
    printf("Enter MDNSSearchRData::MDNSSearchRData\n");
    MDNSTextRData::SetValue("n", name);
    printf("Exit MDNSSearchRData::MDNSSearchRData\n");
}

void MDNSSearchRData::SetValue(String key, String value)
{
    printf("Enter MDNSSearchRData::SetValue\n");
    //
    // Commonly used keys name and implements get abbreviated over the air.
    //
    if (key == "name") {
        MDNSTextRData::SetValue("n", value);
    } else if (key == "implements") {
        MDNSTextRData::SetValue("i", value);
    } else if (key == "send_match_only" || key == "m") {
        MDNSTextRData::SetValue("m", value, true);
    } else {
        MDNSTextRData::SetValue(key, value);
    }
    printf("Exit MDNSSearchRData::SetValue\n");
}
void MDNSSearchRData::SetValue(String key)
{
    printf("Enter MDNSSearchRData::SetValue\n");
    MDNSTextRData::SetValue(key);
    printf("Exot MDNSSearchRData::SetValue\n");
}

bool MDNSSearchRData::SendMatchOnly() 
{
    printf("Enter and Exit MDNSSearchRData::SendMatchOnly. Return MDNSTextRData::HasKey(m)\n");
    return MDNSTextRData::HasKey("m");
}

uint16_t MDNSSearchRData::GetNumFields()
{
    printf("Enter and Exit MDNSSearchRData::GetNumFields. Return m_fields.size()\n");
    return m_fields.size();
}

uint16_t MDNSSearchRData::GetNumSearchCriteria() {
    printf("Enter MDNSSearchRData::GetNumSearchCriteria\n");
    int numEntries = GetNumFields() - 1 /*txtvers*/;
    printf("Exit MDNSSearchRData::GetNumSearchCriteria. Return something big\n");
    return (numEntries > 0) ? (MDNSTextRData::GetNumFields(";") + 1) : 0;
}
String MDNSSearchRData::GetSearchCriterion(int index)
{
    printf("Enter MDNSSearchRData::GetSearchCriterion\n");
    Fields::const_iterator it = m_fields.begin();
    String ret = "";
    while (it != m_fields.end()) {
        String key = it->first;
        key = key.substr(0, key.find_last_of_std('_'));
        if (key == ";") {
            if (index-- == 0) {
                break;
            }
            ret = "";
        } else if (key != "txtvers") {
            if (key == "n") {
                key = "name";
            } else if (key == "i") {
                key = "implements";
            }
            if (!ret.empty()) {
                ret += ",";
            }
            ret += key + "='" + it->second + "'";
        }
        ++it;
    }
    printf("Exit MDNSSearchRData::GetSearchCriterion. Return ret\n");
    return ret;


}
void MDNSSearchRData::RemoveSearchCriterion(int index)
{
    printf("Enter MDNSSearchRData::RemoveSearchCriterion\n");
    Fields::iterator it = m_fields.begin();
    String ret = "";
    while (it != m_fields.end() && index > 0) {
        String key = it->first;
        key = key.substr(0, key.find_last_of_std('_'));
        if (key == ";") {
            if (--index == 0) {
                it++;
                break;
            }
            ret = "";
        } else if (key != "txtvers") {
            if (key == "n") {
                key = "name";
            } else if (key == "i") {
                key = "implements";
            }
            if (!ret.empty()) {
                ret += ",";
            }
            ret += key + "='" + it->second + "'";
        }
        ++it;
    }
    if (it != m_fields.end()) {
        while (it != m_fields.end()) {
            String key = it->first;
            key = key.substr(0, key.find_last_of_std('_'));
            if (key == ";") {
                m_fields.erase(it);
                break;
            } else if (key == "txtvers") {
                it++;
            } else {
                m_fields.erase(it++);
            }
        }

    }
    printf("Exit MDNSSearchRData::RemoveSearchCriterion\n");
}
std::pair<qcc::String, qcc::String> MDNSSearchRData::GetFieldAt(int i)
{
    printf("Enter MDNSSearchRData::GetFieldAt\n");
    Fields::const_iterator it = m_fields.begin();
    while (i-- && it != m_fields.end()) {
        ++it;
    }
    if (it == m_fields.end()) {
        printf("Exit MDNSSearchRData::GetFieldAt. Return pair***\n");
        return pair<String, String>("", "");
    }

    String key = it->first;
    key = key.substr(0, key.find_last_of_std('_'));
    if (key == "n") {
        key = "name";
    } else if (key == "i") {
        key = "implements";
    }
    printf("Exit MDNSSearchRData::GetFieldAt. Return pair<String, String>(key, it->second)\n");
    return pair<String, String>(key, it->second);
}

//MDNSPingRecord
MDNSPingRData::MDNSPingRData(qcc::String name, uint16_t version)
    : MDNSTextRData(version)
{
    printf("Enter MDNSPingRData::MDNSPingRData\n");
    MDNSTextRData::SetValue("n", name);
    printf("Exit MDNSPingRData::MDNSPingRData\n");
}

String MDNSPingRData::GetWellKnownName()
{
    printf("Enter MDNSPingRData::GetWellKnownName. Return MDNSTextRData::GetValue(***)\n");
    return MDNSTextRData::GetValue("n");
}

void MDNSPingRData::SetWellKnownName(qcc::String name)
{
    printf("Enter MDNSPingRData::SetWellKnownName\n");
    MDNSTextRData::SetValue("n", name);
    printf("Exit MDNSPingRData::SetWellKnownName\n");
}

//MDNSPingReplyRecord
MDNSPingReplyRData::MDNSPingReplyRData(qcc::String name, uint16_t version)
    : MDNSTextRData(version)
{
    printf("Enter MDNSPingReplyRData::MDNSPingReplyRData\n");
    MDNSTextRData::SetValue("n", name);
    printf("Exit MDNSPingReplyRData::MDNSPingReplyRData\n");
}

String MDNSPingReplyRData::GetWellKnownName()
{
    printf("Enter and Exit MDNSPingReplyRData::GetWellKnownName. Return MDNSTextRData::GetValue(***)\n");
    return MDNSTextRData::GetValue("n");
}

void MDNSPingReplyRData::SetWellKnownName(qcc::String name)
{
    printf("Enter MDNSPingReplyRData::SetWellKnownName\n");
    MDNSTextRData::SetValue("n", name);
    printf("Exit MDNSPingReplyRData::SetWellKnownName\n");
}

String MDNSPingReplyRData::GetReplyCode()
{
    printf("Enter and exit MDNSPingReplyRData::GetReplyCode. Return MDNSTextRData::GetValue(***)\n");
    return MDNSTextRData::GetValue("replycode");
}

void MDNSPingReplyRData::SetReplyCode(qcc::String replyCode)
{
    printf("Enter MDNSPingReplyRData::SetReplyCode\n");
    MDNSTextRData::SetValue("replycode", replyCode);
    printf("Exit MDNSPingReplyRData::SetReplyCode\n");
}


//MDNSSenderRData
MDNSSenderRData::MDNSSenderRData(uint16_t version)
    : MDNSTextRData(version)
{
    printf("Enter MDNSSenderRData::MDNSSenderRData\n");
    MDNSTextRData::SetValue("pv", U32ToString(NS_VERSION));
    MDNSTextRData::SetValue("ajpv", U32ToString(ALLJOYN_PROTOCOL_VERSION));
    printf("Exit MDNSSenderRData::MDNSSenderRData\n");
}

uint16_t MDNSSenderRData::GetSearchID()
{
    printf("Enter and exit MDNSSenderRData::GetSearchID. Return MDNSTextRData::GetU16Value(***)\n");
    return MDNSTextRData::GetU16Value("sid");
}

void MDNSSenderRData::SetSearchID(uint16_t searchId)
{
    printf("Enter MDNSSenderRData::SetSearchID\n");
    MDNSTextRData::SetValue("sid", U32ToString(searchId));
    printf("Exit MDNSSenderRData::SetSearchID\n");
}

uint16_t MDNSSenderRData::GetIPV4ResponsePort()
{
    printf("Enter and exit MDNSSenderRData::GetIPV4ResponsePort. return MDNSTextRData::GetU16Value\n");
    return MDNSTextRData::GetU16Value("upcv4");
}

void MDNSSenderRData::SetIPV4ResponsePort(uint16_t ipv4Port)
{
    printf("Enter MDNSSenderRData::SetIPV4ResponsePort\n");
    MDNSTextRData::SetValue("upcv4", U32ToString(ipv4Port));
    printf("Exit MDNSSenderRData::SetIPV4ResponsePort\n");
}

qcc::String MDNSSenderRData::GetIPV4ResponseAddr()
{
    printf("Enter and exit  MDNSSenderRData::GetIPV4ResponseAddr. return MDNSTextRData::GetValue\n");
    return MDNSTextRData::GetValue("ipv4");
}

void MDNSSenderRData::SetIPV4ResponseAddr(qcc::String ipv4Addr)
{
    printf("Enter MDNSSenderRData::SetIPV4ResponseAddr\n");
    MDNSTextRData::SetValue("ipv4", ipv4Addr);
    printf("Exit MDNSSenderRData::SetIPV4ResponseAddr\n");
}

//MDNSHeader
MDNSHeader::MDNSHeader() :
    m_queryId(0),
    m_qrType(0),
    m_authAnswer(0),
    m_rCode(NOT_ERROR),
    m_qdCount(0),
    m_anCount(0),
    m_nsCount(0),
    m_arCount(0)
{
    printf("Enter and exit MDNSHeader::MDNSHeade\n");

}

MDNSHeader::MDNSHeader(uint16_t id, bool qrType, uint16_t qdCount, uint16_t anCount, uint16_t nsCount, uint16_t arCount) :
    m_queryId(id),
    m_qrType(qrType),
    m_authAnswer(0),
    m_rCode(NOT_ERROR),
    m_qdCount(qdCount),
    m_anCount(anCount),
    m_nsCount(nsCount),
    m_arCount(arCount)
{
    printf("Enter and exit MDNSHeader::MDNSHeade\n");
}

MDNSHeader::MDNSHeader(uint16_t id, bool qrType) :
    m_queryId(id),
    m_qrType(qrType),
    m_authAnswer(0),
    m_rCode(NOT_ERROR),
    m_qdCount(0),
    m_anCount(0),
    m_nsCount(0),
    m_arCount(0)
{
    printf("Enter and exit MDNSHeader::MDNSHeade\n");
}
MDNSHeader::~MDNSHeader()
{
    printf("Enter and exit MDNSHeader::~MDNSHeade\n");
}

void MDNSHeader::SetId(uint16_t id)
{
    printf("Enter MDNSHeader::SetId\n");
    m_queryId = id;
    printf("Exit MDNSHeader::SetId\n");
}

uint16_t MDNSHeader::GetId()
{
    printf("Enter and exit MDNSHeader::GetId. Return m_queryId\n");
    return m_queryId;
}

void MDNSHeader::SetQRType(bool qrType)
{
    printf("Enter MDNSHeader::SetQRType\n");
    m_qrType = qrType;
    printf("Exit MDNSHeader::SetQRType\n");
}

bool MDNSHeader::GetQRType()
{
    printf("Enter and exit MDNSHeader::GetQRType. Return m_qrType\n");
    return m_qrType;
}


void MDNSHeader::SetQDCount(uint16_t qdCount)
{
    printf("Enter MDNSHeader::SetQDCount\n");
    m_qdCount = qdCount;
    printf("Exit MDNSHeader::SetQDCount\n");
}
uint16_t MDNSHeader::GetQDCount()
{
    printf("Enter and exit MDNSHeader::GetQDCount. Return m_qdCount\n");
    return m_qdCount;
}

void MDNSHeader::SetANCount(uint16_t anCount)
{
    printf("Enter MDNSHeader::SetANCount\n");
    m_anCount = anCount;
    printf("Exit MDNSHeader::SetANCount\n");
}
uint16_t MDNSHeader::GetANCount()
{
    printf("Enter and exit MDNSHeader::GetANCount. Return m_anCount\n");
    return m_anCount;
}

void MDNSHeader::SetNSCount(uint16_t nsCount)
{
    printf("Enter MDNSHeader::SetNSCount.\n");
    m_nsCount = nsCount;
    printf("Exit MDNSHeader::SetNSCount.\n");
}
uint16_t MDNSHeader::GetNSCount()
{
    printf("Enter MDNSHeader::GetNSCount. Return m_nsCount\n");
    return m_nsCount;
}

void MDNSHeader::SetARCount(uint16_t arCount)
{
    printf("Enter MDNSHeader::SetARCount.\n");
    m_arCount = arCount;
    printf("Exit MDNSHeader::SetARCount.\n");
}

uint16_t MDNSHeader::GetARCount()
{
    printf("Enter and exit MDNSHeader::GetARCount. Return m_arCount\n");
    return m_arCount;
}

size_t MDNSHeader::Serialize(uint8_t* buffer) const
{
    printf("Enter MDNSHeader::Serialize.\n");
    QCC_DbgPrintf(("MDNSHeader::Serialize(): to buffer 0x%x", buffer));

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first two octets are ID
    //
    buffer[0] = (m_queryId & 0xFF00) >> 8;
    buffer[1] = (m_queryId & 0xFF);
    size += 2;

    //
    // The third octet is |QR|   Opcode  |AA|TC|RD|RA|
    //
    // QR = packet type Query/Response.
    // Opcode = 0 a standard query (QUERY)
    // AA = 0 for now
    // TC = 0 for now
    // RD = 0 for now
    // RA = 0 for now
    buffer[2] = m_qrType << 7;
    QCC_DbgPrintf(("Header::Serialize(): Third octet = %x", buffer[2]));
    size += 1;

    //
    // The fourth octet is |   Z    |   RCODE   |
    // Z = reserved for future use. Must be zero.
    buffer[3] = m_rCode;
    QCC_DbgPrintf(("Header::Serialize(): RCode = %d", buffer[3]));
    size += 1;

    //
    // The next two octets are QDCOUNT
    //
    //QCC_ASSERT((m_qdCount == 0) || (m_qdCount == 1));
    buffer[4] = 0;
    buffer[5] = m_qdCount;
    QCC_DbgPrintf(("Header::Serialize(): QDCOUNT = %d", buffer[5]));
    size += 2;


    //
    // The next two octets are ANCOUNT
    //
    //QCC_ASSERT((m_anCount == 0) || (m_anCount == 1));
    buffer[6] = 0;
    buffer[7] = m_anCount;
    QCC_DbgPrintf(("Header::Serialize(): ANCOUNT = %d", buffer[7]));
    size += 2;

    //
    // The next two octets are NSCOUNT
    //
    buffer[8] = m_nsCount >> 8;
    buffer[9] = m_nsCount & 0xFF;
    QCC_DbgPrintf(("Header::Serialize(): NSCOUNT = %d", m_nsCount));
    size += 2;

    //
    // The next two octets are ARCOUNT
    //
    buffer[10] = m_arCount >> 8;
    buffer[11] = m_arCount & 0xFF;
    QCC_DbgPrintf(("Header::Serialize(): ARCOUNT = %d", m_arCount));
    size += 2;

    printf("Exit MDNSHeader::Serialize. Return size\n");
    return size;
}

size_t MDNSHeader::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    printf("Enter MDNSHeader::Deserialize.\n");
    //
    // If there's not enough room in the buffer to get the fixed part out then
    // bail (one byte of version, one byte of question count, one byte of answer
    // count and one byte of timer).
    //
    if (bufsize < 12) {
        QCC_DbgPrintf(("Header::Deserialize(): Insufficient bufsize %d", bufsize));
        printf("Exit MDNSHeader::Deserialize. Return 0\n");
        return 0;
    }

    //
    // We keep track of the size so testers can check coherence between
    // GetSerializedSize() and Serialize() and Deserialize().
    //
    size_t size = 0;

    //
    // The first two octets are ID
    //
    m_queryId = (buffer[0] << 8) | buffer[1];

    size += 2;

    //
    // The third octet is |QR|   Opcode  |AA|TC|RD|RA|
    //
    // QR = packet type Query/Response.
    // Opcode = 0 a standard query (QUERY)
    // AA = 0 for now
    // TC = 0 for now
    // RD = 0 for now
    // RA = 0 for now
    // Extract QR type
    m_qrType = buffer[2] >> 7;
    size += 1;

    //
    // The fourth octet is |   Z    |   RCODE   |
    // Z = reserved for future use. Must be zero.
    // Extract RCode.
    m_rCode = (RCodeType)(buffer[3] & 0x0F);
    size += 1;

    //
    // The next two octets are QDCOUNT
    //
    m_qdCount = buffer[5];
    //QCC_ASSERT((m_qdCount == 0) || (m_qdCount == 1));
    size += 2;

    //
    // The next two octets are ANCOUNT
    //
    m_anCount = (buffer[6] << 8) | buffer[7];
    //QCC_ASSERT((m_anCount == 0) || (m_anCount == 1));
    size += 2;

    //
    // The next two octets are NSCOUNT
    //
    m_nsCount = (buffer[8] << 8) | buffer[9];
    QCC_DbgPrintf(("Header::Deserialize(): NSCOUNT = %d", m_nsCount));
    size += 2;

    //
    // The next two octets are ARCOUNT
    //
    m_arCount = (buffer[10] << 8) | buffer[11];
    size += 2;

    bufsize -= 12;
    printf("Exit MDNSHeader::Deserialize. Return size\n");
    return size;
}

size_t MDNSHeader::GetSerializedSize(void) const
{
    //
    // We have 12 octets in the header.
    //
    printf("Enter and Exit MDNSHeader::GetSerializedSize. Return 12\n");
    return 12;
}

//MDNSPacket
_MDNSPacket::_MDNSPacket() {
    printf("Enter _MDNSPacket::_MDNSPacket\n");
    m_questions.reserve(MIN_RESERVE);
    m_answers.reserve(MIN_RESERVE);
    m_authority.reserve(MIN_RESERVE);
    m_additional.reserve(MIN_RESERVE);
    printf("Exit _MDNSPacket::_MDNSPacket\n");
}

_MDNSPacket::~_MDNSPacket() {
    printf("Enter and exit _MDNSPacket::~_MDNSPacket\n");
}

void _MDNSPacket::Clear()
{
    printf("Enter _MDNSPacket::Clear\n");
    m_questions.clear();
    m_answers.clear();
    m_authority.clear();
    m_additional.clear();
    printf("Exit _MDNSPacket::Clear\n");
}

void _MDNSPacket::SetHeader(MDNSHeader header)
{
    printf("Enter _MDNSPacket::SetHeader\n");
    m_header = header;
    printf("Exit _MDNSPacket::SetHeader\n");
}

MDNSHeader _MDNSPacket::GetHeader()
{
    printf("Enter and exit _MDNSPacket::GetHeader. Return m_header\n");
    return m_header;
}

void _MDNSPacket::AddQuestion(MDNSQuestion question)
{
    printf("Enter _MDNSPacket::AddQuestion\n");
    m_questions.push_back(question);
    QCC_ASSERT(m_questions.size() <= MIN_RESERVE);
    m_header.SetQDCount(m_questions.size());
    printf("Exit _MDNSPacket::AddQuestion\n");
}

bool _MDNSPacket::GetQuestionAt(uint32_t i, MDNSQuestion** question)
{
    printf("Enter _MDNSPacket::_GetQuestionAt\n");
    if (i >= m_questions.size()) {
        printf("Exit _MDNSPacket::_etQuestionAt. Return false\n");
        return false;
    }
    *question = &m_questions[i];
    printf("Exit _MDNSPacket::GetQuestionAt. Return true\n");
    return true;
}
bool _MDNSPacket::GetQuestion(qcc::String str, MDNSQuestion** question)
{
    printf("Enter _MDNSPacket::GetQuestion\n");    
    std::vector<MDNSQuestion>::iterator it1 = m_questions.begin();
    while (it1 != m_questions.end()) {
        if (((*it1).GetQName() == str)) {
            *question = &(*it1);
            printf("Exit _MDNSPacket::GetQuestion. Return true\n"); 
            return true;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::GetQuestion. Return false\n"); 
    return false;
}
uint16_t _MDNSPacket::GetNumQuestions()
{
    printf("Enter and exit _MDNSPacket::GetNumQuestions. Return m_questions.size()\n"); 
    return m_questions.size();
}

void _MDNSPacket::AddAdditionalRecord(MDNSResourceRecord record)
{

    printf("Enter _MDNSPacket::AddAdditionalRecord\n");
    m_additional.push_back(record);
    QCC_ASSERT(m_additional.size() <= MIN_RESERVE);
    m_header.SetARCount(m_additional.size());
    printf("Exit _MDNSPacket::AddAdditionalRecord\n");
}

bool _MDNSPacket::GetAdditionalRecordAt(uint32_t i, MDNSResourceRecord** additional)
{
    printf("Enter _MDNSPacket::GetAdditionalRecordAt\n");
    if (i >= m_additional.size()) {
        printf("Exit _MDNSPacket::GetAdditionalRecordAt. Return false\n");
        return false;
    }
    *additional = &m_additional[i];
    printf("Exit _MDNSPacket::GetAdditionalRecordAt. Return true\n");
    return true;
}

uint16_t _MDNSPacket::GetNumAdditionalRecords()
{
    printf("Enter and exit _MDNSPacket::GetNumAdditionalRecords. return m_additional.size()\n");
    return m_additional.size();
}

void _MDNSPacket::RemoveAdditionalRecord(qcc::String str, MDNSResourceRecord::RRType type)
{
    printf("Enter _MDNSPacket::RemoveAdditionalRecord\n");
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type)) {
            m_additional.erase(it1++);
            m_header.SetARCount(m_additional.size());
            printf("Exit _MDNSPacket::RemoveAdditionalRecord\n");
            return;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::RemoveAdditionalRecord\n");
}

bool _MDNSPacket::GetAdditionalRecord(qcc::String str, MDNSResourceRecord::RRType type, MDNSResourceRecord** additional)
{
    printf("Enter _MDNSPacket::GetAdditionalRecord\n");
    size_t starPos = str.find_last_of_std('*');
    String name = str.substr(0, starPos);
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type)) {
            *additional = &(*it1);
            printf("Exit _MDNSPacket::GetAdditionalRecord. Return true\n");
            return true;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::GetAdditionalRecord. Return false\n");
    return false;
}

bool _MDNSPacket::GetAdditionalRecord(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version, MDNSResourceRecord** additional)
{
    printf("Enter _MDNSPacket::GetAdditionalRecord\n");
    if (type != MDNSResourceRecord::TXT) {
        printf("Exit _MDNSPacket::GetAdditionalRecord. Return false\n");
        return false;
    }
    size_t starPos = str.find_last_of_std('*');
    String name = str.substr(0, starPos);
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            *additional = &(*it1);
            printf("Exit _MDNSPacket::GetAdditionalRecord. Return true\n");
            return true;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::GetAdditionalRecord. Return false\n");
    return false;
}

uint32_t _MDNSPacket::GetNumMatches(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version)
{
    printf("Enter _MDNSPacket::GetNumMatches\n");
    if (type != MDNSResourceRecord::TXT) {
        printf("Exit _MDNSPacket::GetNumMatches. Return false\n");
        return false;
    }
    uint32_t numMatches =  0;
    size_t starPos = str.find_last_of_std('*');
    String name = str.substr(0, starPos);
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            numMatches++;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::GetNumMatches. Return numMatches\n");
    return numMatches;
}

bool _MDNSPacket::GetAdditionalRecordAt(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version, uint32_t index, MDNSResourceRecord** additional)
{
    printf("Enter _MDNSPacket::GetAdditionalRecordAt\n");
    if (type != MDNSResourceRecord::TXT) {
        printf("Exit _MDNSPacket::GetAdditionalRecordAt. Return false\n");
        return false;
    }
    size_t starPos = str.find_last_of_std('*');
    String name = str.substr(0, starPos);
    uint32_t i = 0;
    std::vector<MDNSResourceRecord>::iterator it1 = m_additional.begin();
    while (it1 != m_additional.end()) {
        String dname = (*it1).GetDomainName();
        bool nameMatches = (starPos == String::npos) ? (dname == name) : (dname.find(name) == 0);
        if (nameMatches && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            if (i++ == index) {
                *additional = &(*it1);
                printf("Exit _MDNSPacket::GetAdditionalRecordAt. Return true\n");
                return true;
            }
        }
        it1++;
    }
    printf("Exit _MDNSPacket::GetAdditionalRecordAt. Return false\n");
    return false;
}

bool _MDNSPacket::GetAnswer(qcc::String str, MDNSResourceRecord::RRType type, MDNSResourceRecord** answer)
{
    printf("Enter _MDNSPacket::GetAnswer\n");
    std::vector<MDNSResourceRecord>::iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type)) {
            *answer = &(*it1);
            printf("Exit _MDNSPacket::GetAnswer. Return true\n");
            return true;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::GetAnswer. Return false\n");
    return false;
}

bool _MDNSPacket::GetAnswer(qcc::String str, MDNSResourceRecord::RRType type, uint16_t version, MDNSResourceRecord** answer)
{
    printf("Enter _MDNSPacket::GetAnswer\n");
    if (type != MDNSResourceRecord::TXT) {
        printf("Exit _MDNSPacket::GetAnswer. Return false\n")  ;      
        return false;
    }
    std::vector<MDNSResourceRecord>::iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type) &&
            (static_cast<MDNSTextRData*>((*it1).GetRData())->GetU16Value("txtvers") == version)) {
            *answer = &(*it1);
            printf("Exit _MDNSPacket::GetAnswer. Return true\n");
            return true;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::GetAnswer. Return false\n");
    return false;
}

void _MDNSPacket::AddAnswer(MDNSResourceRecord record)
{
    printf("Enter _MDNSPacket::AddAnswer\n");
    m_answers.push_back(record);
    QCC_ASSERT(m_answers.size() <= MIN_RESERVE);
    m_header.SetANCount(m_answers.size());
    printf("Exit _MDNSPacket::AddAnswer\n");
}

bool _MDNSPacket::GetAnswerAt(uint32_t i, MDNSResourceRecord** answer)
{
    printf("Enter _MDNSPacket::GetAnswerAt\n");
    if (i >= m_answers.size()) {
        printf("Exit _MDNSPacket::GetAnswerAt. Return false\n");
        return false;
    }
    *answer = &m_answers[i];
    printf("Exit _MDNSPacket::GetAnswerAt. Return true\n");
    return true;
}

uint16_t _MDNSPacket::GetNumAnswers()
{
    printf("Enter _MDNSPacket::GetNumAnswers. Return m_answers.size()\n");
    return m_answers.size();
}

size_t _MDNSPacket::GetSerializedSize(void) const
{
    printf("Enter _MDNSPacket::GetSerializedSize\n");
    std::map<qcc::String, uint32_t> offsets;
    size_t ret;

    size_t size = m_header.GetSerializedSize();

    std::vector<MDNSQuestion>::const_iterator it = m_questions.begin();
    while (it != m_questions.end()) {
        ret = (*it).GetSerializedSize(offsets);
        size += ret;
        it++;
    }

    std::vector<MDNSResourceRecord>::const_iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        ret = (*it1).GetSerializedSize(offsets);
        size += ret;
        it1++;
    }

    it1 = m_authority.begin();
    while (it1 != m_authority.end()) {
        ret = (*it1).GetSerializedSize(offsets);
        size += ret;
        it1++;
    }

    it1 =  m_additional.begin();
    while (it1 != m_additional.end()) {
        ret = (*it1).GetSerializedSize(offsets);
        size += ret;
        it1++;
    }

    printf("Exit _MDNSPacket::GetSerializedSize. Return size\n");
    return size;
}

size_t _MDNSPacket::Serialize(uint8_t* buffer) const
{
    printf("Enter _MDNSPacket::Serialize\n");
    std::map<qcc::String, uint32_t> offsets;
    size_t ret;

    size_t size = m_header.Serialize(buffer);
    size_t headerOffset = size;

    uint8_t* p = &buffer[size];
    std::vector<MDNSQuestion>::const_iterator it = m_questions.begin();
    while (it != m_questions.end()) {
        ret = (*it).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it++;
    }

    std::vector<MDNSResourceRecord>::const_iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        ret = (*it1).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it1++;

    }

    it1 = m_authority.begin();
    while (it1 != m_authority.end()) {
        ret = (*it1).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it1++;
    }

    it1 =  m_additional.begin();
    while (it1 != m_additional.end()) {
        ret = (*it1).Serialize(p, offsets, headerOffset);
        size += ret;
        headerOffset += ret;
        p += ret;
        it1++;
    }
    printf("Exit _MDNSPacket::Serialize. Return size\n");
    return size;
}

size_t _MDNSPacket::Deserialize(uint8_t const* buffer, uint32_t bufsize)
{
    printf("Enter _MDNSPacket::Deserialize\n");
    Clear();
    std::map<uint32_t, qcc::String> compressedOffsets;
    size_t size = m_header.Deserialize(buffer, bufsize);
    size_t ret;
    if (size == 0) {
        QCC_DbgPrintf(("Error occured while deserializing header"));
        printf("Exit _MDNSPacket::Deserialize. Return size\n");
        return size;
    }
    if (m_header.GetQRType() == MDNSHeader::MDNS_QUERY && m_header.GetQDCount() == 0) {
        //QRType = 0  and QDCount = 0. Invalid
        printf("Exit _MDNSPacket::Deserialize. Return 0\n");
        return 0;
    }
    if (size >= bufsize) {
        printf("Exit _MDNSPacket::Deserialize. Return 0\n");
        return 0;
    }
    bufsize -= size;
    uint8_t const* p = &buffer[size];
    size_t headerOffset = size;
    for (int i = 0; i < m_header.GetQDCount(); i++) {
        MDNSQuestion q;
        ret = q.Deserialize(p, bufsize, compressedOffsets, headerOffset);
        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing question"));
            printf("Exit _MDNSPacket::Deserialize. Return 0\n");
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_questions.push_back(q);

    }
    for (int i = 0; i < m_header.GetANCount(); i++) {
        MDNSResourceRecord r;
        ret = r.Deserialize(p, bufsize, compressedOffsets, headerOffset);
        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing answer"));
            printf("Exit _MDNSPacket::Deserialize. Return 0\n");
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_answers.push_back(r);
    }
    for (int i = 0; i < m_header.GetNSCount(); i++) {
        MDNSResourceRecord r;
        ret = r.Deserialize(p, bufsize, compressedOffsets, headerOffset);
        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing NS"));
            printf("Exit _MDNSPacket::Deserialize. Return 0\n");
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_authority.push_back(r);
    }
    for (int i = 0; i < m_header.GetARCount(); i++) {
        MDNSResourceRecord r;
        ret = r.Deserialize(p, bufsize, compressedOffsets, headerOffset);

        if (ret == 0 || ret > bufsize) {
            QCC_DbgPrintf(("Error while deserializing additional"));
            printf("Exit _MDNSPacket::Deserialize. Return 0\n");
            return 0;
        }

        size += ret;
        bufsize -= ret;
        p += ret;
        headerOffset += ret;
        m_additional.push_back(r);
    }
    printf("Exit _MDNSPacket::Deserialize. Return size\n");
    return size;

}
TransportMask _MDNSPacket::GetTransportMask()
{
    printf("Enter _MDNSPacket::GetTransportMask\n");
    TransportMask transportMask = TRANSPORT_NONE;
    if (GetHeader().GetQRType() == MDNSHeader::MDNS_QUERY) {
        MDNSQuestion* question;
        if (GetQuestion("_alljoyn._tcp.local.", &question)) {
            transportMask |= TRANSPORT_TCP;

        }
        if (GetQuestion("_alljoyn._udp.local.", &question)) {
            transportMask |= TRANSPORT_UDP;

        }
    } else {
        MDNSResourceRecord* answer;
        if (GetAnswer("_alljoyn._tcp.local.", MDNSResourceRecord::PTR, &answer)) {
            transportMask |= TRANSPORT_TCP;


        }
        if (GetAnswer("_alljoyn._udp.local.", MDNSResourceRecord::PTR, &answer)) {
            transportMask |= TRANSPORT_UDP;
        }
    }
    printf("Exit _MDNSPacket::GetTransportMask. Return transportMask\n");
    return transportMask;
}
void _MDNSPacket::RemoveAnswer(qcc::String str, MDNSResourceRecord::RRType type)
{
    printf("Enter _MDNSPacket::RemoveAnswer\n");
    std::vector<MDNSResourceRecord>::iterator it1 = m_answers.begin();
    while (it1 != m_answers.end()) {
        if (((*it1).GetDomainName() == str) && ((*it1).GetRRType() == type)) {
            m_answers.erase(it1++);
            m_header.SetANCount(m_answers.size());
            printf("Exit _MDNSPacket::RemoveAnswer\n");
            return;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::RemoveAnswer\n");
}

void _MDNSPacket::RemoveQuestion(qcc::String str)
{
    printf("Enter _MDNSPacket::RemoveQuestion\n");
    std::vector<MDNSQuestion>::iterator it1 = m_questions.begin();
    while (it1 != m_questions.end()) {
        if (((*it1).GetQName() == str)) {
            m_questions.erase(it1);
            m_header.SetQDCount(m_questions.size());
            printf("Exit _MDNSPacket::RemoveQuestion\n");
            return;
        }
        it1++;
    }
    printf("Exit _MDNSPacket::RemoveQuestion\n");
}

} // namespace ajn

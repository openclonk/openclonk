/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include "C4Include.h"
#include "netpuncher/C4PuncherPacket.h"

#include "network/C4Network2Address.h"

static const char C4NetpuncherProtocolVersion = 1;
// Netpuncher packet header: (1 byte type "Status"), 1 byte version
static const size_t HeaderSize = 2, HeaderPSize = 1;

void C4NetpuncherID::CompileFunc(StdCompiler *pComp) {
	pComp->Value(mkNamingAdapt(v4, "IPv4", 0u));
	pComp->Value(mkNamingAdapt(v6, "IPv6", 0u));
}

std::unique_ptr<C4NetpuncherPacket> C4NetpuncherPacket::Construct(const C4NetIOPacket& rpack) {
	if (!rpack.getPData() || *rpack.getPData() != C4NetpuncherProtocolVersion) return nullptr;
	try {
		switch (rpack.getStatus())
		{
			case PID_Puncher_AssID: return uptr(new C4NetpuncherPacketAssID(rpack));
			case PID_Puncher_SReq:  return uptr(new C4NetpuncherPacketSReq(rpack));
			case PID_Puncher_CReq:  return uptr(new C4NetpuncherPacketCReq(rpack));
			case PID_Puncher_IDReq: return uptr(new C4NetpuncherPacketIDReq(rpack));
			default: return nullptr;
		}
	}
	catch (StdCompiler::Exception *e) { delete e; return nullptr; }
	catch (...) { return nullptr; }
}
C4NetIOPacket C4NetpuncherPacket::PackTo(const C4NetIO::addr_t& addr) const {
	C4NetIOPacket pkt;
	pkt.SetAddr(addr);
	StdBuf content(PackInto());
	char type = GetType();
	pkt.New(sizeof(type) + sizeof(C4NetpuncherProtocolVersion) + content.getSize());
	size_t offset = 0;
	pkt.Write(&type, sizeof(type), offset);
	offset += sizeof(type);
	pkt.Write(&C4NetpuncherProtocolVersion, sizeof(C4NetpuncherProtocolVersion), offset);
	offset += sizeof(C4NetpuncherProtocolVersion);
	pkt.Write(content, offset);
	return pkt;
}

C4NetpuncherPacketCReq::C4NetpuncherPacketCReq(const C4NetIOPacket& rpack) {
	if (rpack.getPSize() < HeaderPSize + 2 + 16) throw "invalid size";
	uint16_t port = *getBufPtr<uint16_t>(rpack, HeaderSize);
	addr.SetAddress(C4NetIO::addr_t::Any, port);
	memcpy(&static_cast<sockaddr_in6*>(&addr)->sin6_addr, getBufPtr<char>(rpack, HeaderSize + sizeof(port)), 16);
}

StdBuf C4NetpuncherPacketCReq::PackInto() const {
	StdBuf buf;
	auto sin6 = static_cast<sockaddr_in6>(addr.AsIPv6());
	auto port = addr.GetPort();
	buf.New(sizeof(port) + sizeof(sin6.sin6_addr));
	size_t offset = 0;
	buf.Write(&port, sizeof(port), offset);
	offset += sizeof(port);
	buf.Write(&sin6.sin6_addr, sizeof(sin6.sin6_addr), offset);
	static_assert(sizeof(sin6.sin6_addr) == 16, "expected sin6_addr to be 16 bytes");
	return buf;
}

template<C4NetpuncherPacketType TYPE>
C4NetpuncherPacketID<TYPE>::C4NetpuncherPacketID(const C4NetIOPacket& rpack) {
	if (rpack.getPSize() < HeaderPSize + sizeof(id)) throw "invalid size";
	id = *getBufPtr<CID>(rpack, HeaderSize);
}

template<C4NetpuncherPacketType TYPE>
StdBuf C4NetpuncherPacketID<TYPE>::PackInto() const {
	StdBuf buf;
	auto id = GetID();
	buf.New(sizeof(id));
	buf.Write(&id, sizeof(id));
	return buf;
}

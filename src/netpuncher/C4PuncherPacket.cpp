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
#include <sstream>

void C4NetpuncherID::CompileFunc(StdCompiler *pComp) {
	pComp->Value(mkNamingAdapt(v4, "IPv4", 0u));
	pComp->Value(mkNamingAdapt(v6, "IPv6", 0u));
}

std::unique_ptr<C4NetpuncherPacket> C4NetpuncherPacket::Construct(const C4NetIOPacket& rpack) {
	if (!rpack.getPData()) return nullptr;
	try {
		switch (rpack.getStatus())
		{
			case PID_Puncher_AssID: return uptr(new C4NetpuncherPacketAssID(rpack));
			case PID_Puncher_SReq:  return uptr(new C4NetpuncherPacketSReq(rpack));
			case PID_Puncher_CReq:  return uptr(new C4NetpuncherPacketCReq(rpack));
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
	pkt.New(sizeof(type) + content.getSize());
	pkt.Write(&type, sizeof(type));
	pkt.Write(content, /*offset*/sizeof(type));
	return pkt;
}

C4NetpuncherPacketCReq::C4NetpuncherPacketCReq(const C4NetIOPacket& rpack) {
	C4Network2Address parse_addr;
	CompileFromBuf<StdCompilerBinRead>(parse_addr, rpack.getPBuf());
	if (parse_addr.getProtocol() != P_UDP) throw P_UDP;
	addr = parse_addr.getAddr();
}

StdBuf C4NetpuncherPacketCReq::PackInto() const {
	C4Network2Address ser_addr;
	ser_addr.SetAddr(addr);
	ser_addr.SetProtocol(P_UDP);
	return DecompileToBuf<StdCompilerBinWrite>(ser_addr);
}

template<C4NetpuncherPacketType TYPE>
C4NetpuncherPacketID<TYPE>::C4NetpuncherPacketID(const C4NetIOPacket& rpack) {
	std::istringstream iss(std::string(rpack.getPData(), rpack.getPSize()));
	iss >> id;
}

template<C4NetpuncherPacketType TYPE>
StdBuf C4NetpuncherPacketID<TYPE>::PackInto() const {
	std::ostringstream oss;
	oss << GetID();
	std::string s = oss.str();
	return StdCopyBuf(s.c_str(), s.length());
}

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

#ifndef C4PuncherPacket_H
#define C4PuncherPacket_H

/* I'm cooking up a seperate solution for the netpuncher because using C4Packet2.cpp would require introducing half a ton of stubs, and it seems like overkill */

#include "network/C4NetIO.h"

enum C4NetpuncherPacketType {
	PID_Puncher_AssID = 0x51, // Puncher announcing ID to host
	PID_Puncher_SReq  = 0x52, // Client requesting to be served with punching (for an ID)
	PID_Puncher_CReq  = 0x53, // Puncher requesting clients to punch (towards an address)
	PID_Puncher_IDReq = 0x54, // Host requesting an ID
	// extend this with exchanging ICE parameters, some day?
};

struct C4NetpuncherID {
	typedef uint32_t value;

	value v4 = 0, v6 = 0;

	void CompileFunc(StdCompiler *pComp);
	bool operator==(const C4NetpuncherID& other) const { return v4 == other.v4 && v6 == other.v6; }
};

class C4NetpuncherPacket {
public:
	typedef std::unique_ptr<C4NetpuncherPacket> uptr;
	static std::unique_ptr<C4NetpuncherPacket> Construct(const C4NetIOPacket& rpack);
	virtual ~C4NetpuncherPacket() = default;
	virtual C4NetpuncherPacketType GetType() const = 0;
	C4NetIOPacket PackTo(const C4NetIO::addr_t&) const;
protected:
	virtual StdBuf PackInto() const = 0;
	typedef C4NetpuncherID::value CID;
};

class C4NetpuncherPacketIDReq : public C4NetpuncherPacket {
private:
	StdBuf PackInto() const override { return StdBuf(); }
public:
	C4NetpuncherPacketIDReq() = default;
	C4NetpuncherPacketIDReq(const C4NetIOPacket& rpack) { }
	C4NetpuncherPacketType GetType() const final { return PID_Puncher_IDReq; }
};

template<C4NetpuncherPacketType TYPE>
class C4NetpuncherPacketID : public C4NetpuncherPacket {
private: 
	CID id;
	StdBuf PackInto() const override; 
protected: 
	C4NetpuncherPacketID(const C4NetIOPacket& rpack);
	C4NetpuncherPacketID(CID id) : id(id) {};
public:
	C4NetpuncherPacketType GetType() const final { return TYPE; }
	CID GetID() const { return id; }
	~C4NetpuncherPacketID() override = default;;
};

#define PIDC(n) \
	class C4NetpuncherPacket##n : public C4NetpuncherPacketID<PID_Puncher_##n> { \
		public: explicit C4NetpuncherPacket##n(const C4NetIOPacket& p) : C4NetpuncherPacketID<PID_Puncher_##n>(p) {}; \
		public: explicit C4NetpuncherPacket##n(CID id) : C4NetpuncherPacketID<PID_Puncher_##n>(id) {}; \
	}
PIDC(AssID); PIDC(SReq);
#undef PIDC

class C4NetpuncherPacketCReq : public C4NetpuncherPacket {
private:
	C4NetIO::addr_t addr; 
	StdBuf PackInto() const override;
public:
	C4NetpuncherPacketType GetType() const final { return PID_Puncher_CReq; }
	explicit C4NetpuncherPacketCReq(const C4NetIOPacket& rpack);
	explicit C4NetpuncherPacketCReq(const C4NetIO::addr_t& addr) : addr(addr) {};
	const C4NetIO::addr_t& GetAddr() { return addr; }
};

#endif

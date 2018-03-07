/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
#include "network/C4NetIO.h"

#include "config/C4Constants.h"
#include "lib/C4Random.h"

#include <sys/stat.h>

// platform specifics
#ifdef _WIN32

#include <process.h>
#include <share.h>
#include <winsock2.h>
#include <iphlpapi.h>

typedef int socklen_t;
int pipe(int *phandles) { return _pipe(phandles, 10, O_BINARY); }

#else

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>

#define ioctlsocket ioctl
#define closesocket close
#define SOCKET_ERROR (-1)

#endif

#ifdef _MSC_VER
#pragma warning (disable : 4355)
#endif

// These are named differently on mac.
#if !defined(IPV6_ADD_MEMBERSHIP) && defined(IPV6_JOIN_GROUP)
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

#ifdef __linux__
#include <linux/in6.h>
#include <linux/if_addr.h>

// Linux definitions needed for parsing /proc/if_inet6
#define IPV6_ADDR_LOOPBACK	0x0010U
#define IPV6_ADDR_LINKLOCAL	0x0020U
#define IPV6_ADDR_SITELOCAL	0x0040U
#endif

// constants definition
const int C4NetIO::TO_INF = -1;

// simulate packet loss (loss probability in percent)
// #define C4NETIO_SIMULATE_PACKETLOSS 10

// *** helpers

#ifdef HAVE_WINSOCK

const char *GetSocketErrorMsg(int iError)
{
	switch (iError)
	{
	case WSAEACCES: return "Permission denied.";
	case WSAEADDRINUSE: return "Address already in use.";
	case WSAEADDRNOTAVAIL: return "Cannot assign requested address.";
	case WSAEAFNOSUPPORT: return "Address family not supported by protocol family.";
	case WSAEALREADY: return "Operation already in progress.";
	case WSAECONNABORTED: return "Software caused connection abort.";
	case WSAECONNREFUSED: return "Connection refused.";
	case WSAECONNRESET: return "Connection reset by peer.";
	case WSAEDESTADDRREQ: return "Destination address required.";
	case WSAEFAULT: return "Bad address.";
	case WSAEHOSTDOWN: return "Host is down.";
	case WSAEHOSTUNREACH: return "No route to host.";
	case WSAEINPROGRESS: return "Operation now in progress.";
	case WSAEINTR: return "Interrupted function call.";
	case WSAEINVAL: return "Invalid argument.";
	case WSAEISCONN: return "Socket is already connected.";
	case WSAEMFILE: return "Too many open files.";
	case WSAEMSGSIZE: return "Message too long.";
	case WSAENETDOWN: return "Network is down.";
	case WSAENETRESET: return "Network dropped connection on reset.";
	case WSAENETUNREACH: return "Network is unreachable.";
	case WSAENOBUFS: return "No buffer space available.";
	case WSAENOPROTOOPT: return "Bad protocol option.";
	case WSAENOTCONN: return "Socket is not connected.";
	case WSAENOTSOCK: return "Socket operation on non-socket.";
	case WSAEOPNOTSUPP: return "Operation not supported.";
	case WSAEPFNOSUPPORT: return "Protocol family not supported.";
	case WSAEPROCLIM: return "Too many processes.";
	case WSAEPROTONOSUPPORT: return "Protocol not supported.";
	case WSAEPROTOTYPE: return "Protocol wrong type for socket.";
	case WSAESHUTDOWN: return "Cannot send after socket shutdown.";
	case WSAESOCKTNOSUPPORT: return "Socket type not supported.";
	case WSAETIMEDOUT: return "Connection timed out.";
	case WSATYPE_NOT_FOUND: return "Class type not found.";
	case WSAEWOULDBLOCK: return "Resource temporarily unavailable.";
	case WSAHOST_NOT_FOUND: return "Host not found.";
	case WSA_INVALID_HANDLE: return "Specified event object handle is invalid.";
	case WSA_INVALID_PARAMETER: return "One or more parameters are invalid.";
	case WSA_IO_INCOMPLETE: return "Overlapped I/O event object not in signaled state.";
	case WSA_IO_PENDING: return "Overlapped operations will complete later.";
	case WSA_NOT_ENOUGH_MEMORY: return "Insufficient memory available.";
	case WSANOTINITIALISED: return "Successful WSAStartup not yet performed.";
	case WSANO_DATA: return "Valid name, no data record of requested type.";
	case WSANO_RECOVERY: return "This is a non-recoverable error.";
	case WSASYSCALLFAILURE: return "System call failure.";
	case WSASYSNOTREADY: return "Network subsystem is unavailable.";
	case WSATRY_AGAIN: return "Non-authoritative host not found.";
	case WSAVERNOTSUPPORTED: return "WINSOCK.DLL version out of range.";
	case WSAEDISCON: return "Graceful shutdown in progress.";
	case WSA_OPERATION_ABORTED: return "Overlapped operation aborted.";
	case 0: return "no error";
	default: return "Stupid Error.";
	}
}
const char *GetSocketErrorMsg()
{
	return GetSocketErrorMsg(WSAGetLastError());
}
bool HaveSocketError()
{
	return !! WSAGetLastError();
}
bool HaveWouldBlockError()
{
	return WSAGetLastError() == WSAEWOULDBLOCK;
}
bool HaveConnResetError()
{
	return WSAGetLastError() == WSAECONNRESET;
}
void ResetSocketError()
{
	WSASetLastError(0);
}

static int iWSockUseCounter = 0;

bool AcquireWinSock()
{
	if (!iWSockUseCounter)
	{
		// initialize winsock
		WSADATA data;
		int res = WSAStartup(WINSOCK_VERSION, &data);
		// success? count
		if (!res)
			iWSockUseCounter++;
		// return result
		return !res;
	}
	// winsock already initialized
	iWSockUseCounter++;
	return true;
}

void ReleaseWinSock()
{
	iWSockUseCounter--;
	// last use?
	if (!iWSockUseCounter)
		WSACleanup();
}

#else

const char *GetSocketErrorMsg(int iError)
{
	return strerror(iError);
}
const char *GetSocketErrorMsg()
{
	return GetSocketErrorMsg(errno);
}

bool HaveSocketError()
{
	return !! errno;
}
bool HaveWouldBlockError()
{
	return errno == EINPROGRESS || errno == EWOULDBLOCK;
}
bool HaveConnResetError()
{
	return errno == ECONNRESET;
}
void ResetSocketError()
{
	errno = 0;
}

#endif // HAVE_WINSOCK

// *** C4NetIO::HostAddress
void C4NetIO::HostAddress::Clear()
{
	v6.sin6_family = AF_INET6;
	v6.sin6_flowinfo = 0;
	v6.sin6_scope_id = 0;
	memset(&v6.sin6_addr, 0, sizeof(v6.sin6_addr));
}

// *** C4NetIO::EndpointAddress
const C4NetIO::EndpointAddress::EndpointAddressPtr C4NetIO::EndpointAddress::operator &() const { return EndpointAddressPtr(const_cast<EndpointAddress*>(this)); }
C4NetIO::EndpointAddress::EndpointAddressPtr C4NetIO::EndpointAddress::operator &() { return EndpointAddressPtr(this); }

void C4NetIO::EndpointAddress::Clear()
{
	HostAddress::Clear();
	SetPort(IPPORT_NONE);
}

void C4NetIO::HostAddress::SetHost(const HostAddress &other)
{
	SetHost(&other.gen);
}

bool C4NetIO::HostAddress::IsMulticast() const
{
	if (gen.sa_family == AF_INET6)
		return IN6_IS_ADDR_MULTICAST(&v6.sin6_addr) != 0;
	if (gen.sa_family == AF_INET)
		return (ntohl(v4.sin_addr.s_addr) >> 24) == 239;
	return false;
}

bool C4NetIO::HostAddress::IsLoopback() const
{
	if (gen.sa_family == AF_INET6)
		return IN6_IS_ADDR_LOOPBACK(&v6.sin6_addr) != 0;
	if (gen.sa_family == AF_INET)
		return (ntohl(v4.sin_addr.s_addr) >> 24) == 127;
	return false;
}

bool C4NetIO::HostAddress::IsLocal() const
{
	if (gen.sa_family == AF_INET6)
		return IN6_IS_ADDR_LINKLOCAL(&v6.sin6_addr) != 0;
	// We don't really care about local 169.256.0.0/16 addresses here as users will either have a
	// router doing DHCP (which will prevent usage of these addresses) or have a network that
	// doesn't care about IP and IPv6 link-local addresses will work.
	return false;
}

bool C4NetIO::HostAddress::IsPrivate() const
{
	// IPv6 unique local address
	if (gen.sa_family == AF_INET6)
		return (v6.sin6_addr.s6_addr[0] & 0xfe) == 0xfc;
	if (gen.sa_family == AF_INET)
	{
		uint32_t addr = ntohl(v4.sin_addr.s_addr);
		uint32_t s = (addr >> 16) & 0xff;
		switch (addr >> 24)
		{
		case 10:  return true;
		case 172: return s >= 16 && s <= 31;
		case 192: return s == 168;
		}
	}
	return false;
}

void C4NetIO::HostAddress::SetScopeId(int scopeId)
{
	if (gen.sa_family != AF_INET6) return;
	if (IN6_IS_ADDR_LINKLOCAL(&v6.sin6_addr) != 0)
		v6.sin6_scope_id = scopeId;
}

int C4NetIO::HostAddress::GetScopeId() const
{
	if (gen.sa_family == AF_INET6)
		return v6.sin6_scope_id;
	return 0;
}

C4NetIO::HostAddress C4NetIO::HostAddress::AsIPv6() const
{
	static const uint8_t v6_mapped_v4_prefix[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

	HostAddress nrv(*this);
	switch (gen.sa_family)
	{
	case AF_INET6:
		// That was easy
		break;
	case AF_INET:
		memmove(((char*)&nrv.v6.sin6_addr) + sizeof(v6_mapped_v4_prefix), &v4.sin_addr, sizeof(v4.sin_addr));
		nrv.v6.sin6_family = AF_INET6;
		memcpy(&nrv.v6.sin6_addr, v6_mapped_v4_prefix, sizeof(v6_mapped_v4_prefix));
		nrv.v6.sin6_flowinfo = 0;
		nrv.v6.sin6_scope_id = 0;
		break;
	default: assert(!"Shouldn't reach this"); break;
	}
	return nrv;
}

C4NetIO::HostAddress C4NetIO::HostAddress::AsIPv4() const
{
	HostAddress nrv(*this);
	if (gen.sa_family == AF_INET6 && IN6_IS_ADDR_V4MAPPED(&v6.sin6_addr))
	{
		nrv.v4.sin_family = AF_INET;
		memcpy((char*) &nrv.v4.sin_addr, (char*) &v6.sin6_addr.s6_addr[12], sizeof(v4.sin_addr));
	}
	return nrv;
}

C4NetIO::EndpointAddress C4NetIO::EndpointAddress::AsIPv6() const
{
	return EndpointAddress(HostAddress::AsIPv6(), GetPort());
}

C4NetIO::EndpointAddress C4NetIO::EndpointAddress::AsIPv4() const
{
	return EndpointAddress(HostAddress::AsIPv4(), GetPort());
}

void C4NetIO::HostAddress::SetHost(const sockaddr *addr)
{
	// Copy all but port number
	if (addr->sa_family == AF_INET6)
	{
		v6.sin6_family = ((const sockaddr_in6*)addr)->sin6_family;
		v6.sin6_flowinfo = ((const sockaddr_in6*)addr)->sin6_flowinfo;
		memcpy(&v6.sin6_addr, &((const sockaddr_in6*)addr)->sin6_addr, sizeof(v6.sin6_addr));
		v6.sin6_scope_id = ((const sockaddr_in6*)addr)->sin6_scope_id;
	}
	else if (addr->sa_family == AF_INET)
	{
		v4.sin_family = ((const sockaddr_in*)addr)->sin_family;
		v4.sin_addr.s_addr = ((const sockaddr_in*)addr)->sin_addr.s_addr;
		memset(&v4.sin_zero, 0, sizeof(v4.sin_zero));
	}
}

void C4NetIO::EndpointAddress::SetAddress(const sockaddr *addr)
{
	switch (addr->sa_family)
	{
	case AF_INET: memcpy(&v4, addr, sizeof(v4)); break;
	case AF_INET6: memcpy(&v6, addr, sizeof(v6)); break;
	default:
		assert(!"Unexpected address family");
		memcpy(&gen, addr, sizeof(gen)); break;
	}
}

void C4NetIO::HostAddress::SetHost(SpecialAddress addr)
{
	switch (addr)
	{
	case Any:
		v6.sin6_family = AF_INET6;
		memset(&v6.sin6_addr, 0, sizeof(v6.sin6_addr));
		v6.sin6_flowinfo = 0;
		v6.sin6_scope_id = 0;
		break;
	case AnyIPv4:
		v4.sin_family = AF_INET;
		v4.sin_addr.s_addr = 0;
		memset(&v4.sin_zero, 0, sizeof(v4.sin_zero));
		break;
	case Loopback:
		v6.sin6_family = AF_INET6;
		memset(&v6.sin6_addr, 0, sizeof(v6.sin6_addr)); v6.sin6_addr.s6_addr[15] = 1;
		v6.sin6_flowinfo = 0;
		v6.sin6_scope_id = 0;
		break;
	}
}

void C4NetIO::HostAddress::SetHost(uint32_t v4addr)
{
		v4.sin_family = AF_INET;
		v4.sin_addr.s_addr = v4addr;
		memset(&v4.sin_zero, 0, sizeof(v4.sin_zero));
}

void C4NetIO::HostAddress::SetHost(const StdStrBuf &addr, AddressFamily family)
{
	addrinfo hints = addrinfo();
	hints.ai_family = family;
	addrinfo *addresses = nullptr;
	if (getaddrinfo(addr.getData(), nullptr, &hints, &addresses) != 0)
		// GAI failed
		return;
	SetHost(addresses->ai_addr);
	freeaddrinfo(addresses);
}

void C4NetIO::EndpointAddress::SetAddress(const StdStrBuf &addr, AddressFamily family)
{
	Clear();

	if (addr.isNull()) return;

	const char *begin = addr.getData();
	const char *end = begin + addr.getLength();

	const char *ab = begin;
	const char *ae = end;
	
	const char *pb = end;
	const char *pe = end;

	bool isIPv6 = false;

	// If addr begins with [, it's an IPv6 address
	if (ab[0] == '[')
	{
		++ab; // skip bracket
		const char *cbracket = std::find(ab, ae, ']');
		if (cbracket == ae)
			// No closing bracket found: invalid
			return;
		ae = cbracket++;
		if (cbracket != end && cbracket[0] == ':')
		{
			// port number given
			pb = ++cbracket;
			if (pb == end)
				// Trailing colon: invalid
				return;
		}
		isIPv6 = true;
	}
	// If there's more than 1 colon in the address, it's IPv6
	else if (std::count(ab, ae, ':') > 1)
	{
		isIPv6 = true;
	}
	// It's probably not IPv6, but look for a port specification
	else
	{
		const char *colon = std::find(ab, ae, ':');
		if (colon != ae)
		{
			ae = colon;
			pb = colon + 1;
			if (pb == end)
				// Trailing colon: invalid
				return;
		}
	}

	addrinfo hints = addrinfo();
	hints.ai_family = family;
	//hints.ai_flags = AI_NUMERICHOST;
	addrinfo *addresses = nullptr;
	if (getaddrinfo(std::string(ab, ae).c_str(), pb != end ? std::string(pb, pe).c_str() : nullptr, &hints, &addresses) != 0)
		// GAI failed
		return;
	SetAddress(addresses->ai_addr);
	freeaddrinfo(addresses);
}

void C4NetIO::EndpointAddress::SetAddress(const EndpointAddress &addr)
{
	SetHost(addr);
	SetPort(addr.GetPort());
}

void C4NetIO::EndpointAddress::SetAddress(HostAddress::SpecialAddress host, uint16_t port)
{
	SetHost(host);
	SetPort(port);
}

void C4NetIO::EndpointAddress::SetAddress(const HostAddress &host, uint16_t port)
{
	SetHost(host);
	SetPort(port);
}

bool C4NetIO::EndpointAddress::IsNull() const
{
	return IsNullHost() && GetPort() == IPPORT_NONE;
}

bool C4NetIO::HostAddress::IsNull() const
{
	switch (gen.sa_family)
	{
	case AF_INET: return v4.sin_addr.s_addr == 0;
	case AF_INET6:
		return !!IN6_IS_ADDR_UNSPECIFIED(&v6.sin6_addr);
	}
	assert(!"Shouldn't reach this");
	return false;
}

C4NetIO::HostAddress::AddressFamily C4NetIO::HostAddress::GetFamily() const
{
	return gen.sa_family == AF_INET ? IPv4 :
		gen.sa_family == AF_INET6 ? IPv6 : UnknownFamily;
}

size_t C4NetIO::HostAddress::GetAddrLen() const
{
	return GetFamily() == IPv4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
}

void C4NetIO::EndpointAddress::SetPort(uint16_t port)
{
	switch (gen.sa_family)
	{
	case AF_INET: v4.sin_port = htons(port); break;
	case AF_INET6: v6.sin6_port = htons(port); break;
	default: assert(!"Shouldn't reach this"); break;
	}
}

void C4NetIO::EndpointAddress::SetDefaultPort(uint16_t port)
{
	if (GetPort() == IPPORT_NONE)
		SetPort(port);
}

uint16_t C4NetIO::EndpointAddress::GetPort() const
{
	switch (gen.sa_family)
	{
	case AF_INET: return ntohs(v4.sin_port);
	case AF_INET6: return ntohs(v6.sin6_port);
	}
	assert(!"Shouldn't reach this");
	return IPPORT_NONE;
}

bool C4NetIO::HostAddress::operator ==(const HostAddress &rhs) const
{
	// Check for IPv4-mapped IPv6 addresses.
	if (gen.sa_family != rhs.gen.sa_family)
		return AsIPv6() == rhs.AsIPv6();
	if (gen.sa_family == AF_INET)
		return v4.sin_addr.s_addr == rhs.v4.sin_addr.s_addr;
	if (gen.sa_family == AF_INET6)
		return memcmp(&v6.sin6_addr, &rhs.v6.sin6_addr, sizeof(v6.sin6_addr)) == 0 &&
			v6.sin6_scope_id == rhs.v6.sin6_scope_id;
	assert(!"Shouldn't reach this");
	return false;
}

bool C4NetIO::EndpointAddress::operator ==(const addr_t &rhs) const
{
	if (!HostAddress::operator==(rhs)) return false;
	if (gen.sa_family == AF_INET)
	{
		return v4.sin_port == rhs.v4.sin_port;
	}
	else if (gen.sa_family == AF_INET6)
	{
		return v6.sin6_port == rhs.v6.sin6_port &&
			v6.sin6_scope_id == rhs.v6.sin6_scope_id;
	}
	assert(!"Shouldn't reach this");
	return false;
}

StdStrBuf C4NetIO::HostAddress::ToString(int flags) const
{
	if (gen.sa_family == AF_INET6 && v6.sin6_scope_id != 0 && (flags & TSF_SkipZoneId))
	{
		HostAddress addr = *this;
		addr.v6.sin6_scope_id = 0;
		return addr.ToString(flags);
	}

	char buf[INET6_ADDRSTRLEN];
	if (getnameinfo(&gen, GetAddrLen(), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST) != 0)
		return StdStrBuf();

	return StdStrBuf(buf, true);
}

StdStrBuf C4NetIO::EndpointAddress::ToString(int flags) const
{
	if (flags & TSF_SkipPort)
		return HostAddress::ToString(flags);

	switch (GetFamily())
	{
	case IPv4: return FormatString("%s:%d", HostAddress::ToString(flags).getData(), GetPort());
	case IPv6: return FormatString("[%s]:%d", HostAddress::ToString(flags).getData(), GetPort());
	default: assert(!"Shouldn't reach this");
	}
	return StdStrBuf();
}

void C4NetIO::EndpointAddress::CompileFunc(StdCompiler *comp)
{
	if (!comp->isDeserializer())
	{
		StdStrBuf val(ToString(TSF_SkipZoneId));
		comp->Value(val);
	} else {
		StdStrBuf val;
		comp->Value(val);
		SetAddress(val);
	}
}

std::vector<C4NetIO::HostAddress> C4NetIO::GetLocalAddresses()
{
	std::vector<HostAddress> result;

#ifdef HAVE_WINSOCK
	HostAddress addr;
	const size_t BUFFER_SIZE = 16000;
	PIP_ADAPTER_ADDRESSES addresses = nullptr;
	for (int i = 0; i < 3; ++i)
	{
		addresses = (PIP_ADAPTER_ADDRESSES) realloc(addresses, BUFFER_SIZE * (i+1));
		if (!addresses)
			// allocation failed
			return result;
		ULONG bufsz = BUFFER_SIZE * (i+1);
		DWORD rv = GetAdaptersAddresses(AF_UNSPEC,
			GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER|GAA_FLAG_SKIP_FRIENDLY_NAME,
			nullptr, addresses, &bufsz);
		if (rv == ERROR_BUFFER_OVERFLOW)
			// too little space, try again
			continue;
		if (rv != NO_ERROR)
		{
			// Something else happened
			free(addresses);
			return result;
		}
		// All okay, add addresses
		for (PIP_ADAPTER_ADDRESSES address = addresses; address; address = address->Next)
		{
			for (PIP_ADAPTER_UNICAST_ADDRESS unicast = address->FirstUnicastAddress; unicast; unicast = unicast->Next)
			{
				addr.SetHost(unicast->Address.lpSockaddr);
				if (addr.IsLoopback())
					continue;
				result.push_back(addr);
			}
		}
	}
	free(addresses);
#else
	bool have_ipv6 = false;

#ifdef __linux__
	// Get IPv6 addresses on Linux from procfs which allows filtering deprecated privacy addresses.
	FILE *f = fopen("/proc/net/if_inet6", "r");
	if (f)
	{
		sockaddr_in6 sa6 = sockaddr_in6();
		sa6.sin6_family = AF_INET6;
		auto a6 = sa6.sin6_addr.s6_addr;
		uint8_t if_idx, plen, scope, flags;
		char devname[20];
		while (fscanf(f, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx %02hhx %02hhx %02hhx %02hhx %20s\n",
					&a6[0], &a6[1], &a6[2],  &a6[3],  &a6[4],  &a6[5],  &a6[6],  &a6[7],
					&a6[8], &a6[9], &a6[10], &a6[11], &a6[12], &a6[13], &a6[14], &a6[15],
					&if_idx, &plen, &scope, &flags, devname) != EOF)
		{
			// Skip loopback and deprecated addresses.
			if (scope == IPV6_ADDR_LOOPBACK || flags & IFA_F_DEPRECATED)
				continue;
			sa6.sin6_scope_id = scope == IPV6_ADDR_LINKLOCAL ? if_idx : 0;
			result.emplace_back((sockaddr*) &sa6);
		}
		have_ipv6 = result.size() > 0;
		fclose(f);
	}
#endif

	struct ifaddrs* addrs;
	if (getifaddrs(&addrs) < 0)
	    return result;
	for (struct ifaddrs* ifaddr = addrs; ifaddr != nullptr; ifaddr = ifaddr->ifa_next)
	{
		struct sockaddr* ad = ifaddr->ifa_addr;
		if (ad == nullptr) continue;

		if ((ad->sa_family == AF_INET || (!have_ipv6 && ad->sa_family == AF_INET6)) && (~ifaddr->ifa_flags & IFF_LOOPBACK)) // Choose only non-loopback IPv4/6 devices
		{
			result.emplace_back(ad);
		}
	}
	freeifaddrs(addrs);
#endif

	return result;
}

// *** C4NetIO

// construction / destruction

C4NetIO::C4NetIO()
{
	ResetError();
}

C4NetIO::~C4NetIO() = default;

bool C4NetIO::InitIPv6Socket(SOCKET socket)
{
	int opt = 0;
	if (setsockopt(socket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&opt), sizeof(opt)) == SOCKET_ERROR)
	{
		SetError("could not enable dual-stack socket", true);
		return false;
	}

#ifdef IPV6_ADDR_PREFERENCES
	// Prefer stable addresses. This should prevent issues with address
	// deprecation while a match is running. No error handling - if the call
	// fails, we just take any address.
	opt = IPV6_PREFER_SRC_PUBLIC;
	setsockopt(socket, IPPROTO_IPV6, IPV6_ADDR_PREFERENCES, reinterpret_cast<char*>(&opt), sizeof(opt));
#endif

	return true;
}

void C4NetIO::SetError(const char *strnError, bool fSockErr)
{
	fSockErr &= HaveSocketError();
	if (fSockErr)
		Error.Format("%s (%s)", strnError, GetSocketErrorMsg());
	else
		Error.Copy(strnError);
}

// *** C4NetIOPacket

// construction / destruction

C4NetIOPacket::C4NetIOPacket() = default;

C4NetIOPacket::C4NetIOPacket(const void *pnData, size_t inSize, bool fCopy, const C4NetIO::addr_t &naddr)
		: StdCopyBuf(pnData, inSize, fCopy), addr(naddr)
{
}

C4NetIOPacket::C4NetIOPacket(const StdBuf &Buf, const C4NetIO::addr_t &naddr)
		: StdCopyBuf(Buf), addr(naddr)
{
}

C4NetIOPacket::C4NetIOPacket(uint8_t cStatusByte, const char *pnData, size_t inSize, const C4NetIO::addr_t &naddr)
{
	// Create buffer
	New(sizeof(cStatusByte) + inSize);
	// Write data
	*getMBufPtr<uint8_t>(*this) = cStatusByte;
	Write(pnData, inSize, sizeof(cStatusByte));
}

C4NetIOPacket::~C4NetIOPacket()
{
	Clear();
}

void C4NetIOPacket::Clear()
{
	addr = C4NetIO::addr_t();
	StdBuf::Clear();
}

// *** C4NetIOTCP

// construction / destruction

C4NetIOTCP::C4NetIOTCP() :
		PeerListCSec(this),
		iListenPort(~0), lsock(INVALID_SOCKET)
{

}

C4NetIOTCP::~C4NetIOTCP()
{
	Close();
}

bool C4NetIOTCP::Init(uint16_t iPort)
{
	// already init? close first
	if (fInit) Close();

#ifdef HAVE_WINSOCK
	// init winsock
	if (!AcquireWinSock())
	{
		SetError("could not start winsock");
		return false;
	}
#endif

#ifdef STDSCHEDULER_USE_EVENTS
	// create event
	if ((Event = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		SetError("could not create socket event", true); // to do: more error information
		return false;
	}
#else
	// create pipe
	if (pipe(Pipe) != 0)
	{
		SetError("could not create pipe", true);
		return false;
	}
#endif

	// create listen socket (if necessary)
	if (iPort != addr_t::IPPORT_NONE)
		if (!Listen(iPort))
			return false;

	// ok
	fInit = true;
	return true;
}

bool C4NetIOTCP::InitBroadcast(addr_t *pBroadcastAddr)
{
	// ignore
	return true;
}

bool C4NetIOTCP::Close()
{
	ResetError();

	// not init?
	if (!fInit) return false;

	// terminate connections
	CStdShareLock PeerListLock(&PeerListCSec);
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->Open())
		{
			pPeer->Close();
			if (pCB) pCB->OnDisconn(pPeer->GetAddr(), this, "owner class closed");
		}

	ClearConnectWaits();

	// close listen socket
	if (lsock != INVALID_SOCKET)
	{
		closesocket(lsock);
		lsock = INVALID_SOCKET;
	}

#ifdef STDSCHEDULER_USE_EVENTS
	// close event
	if (Event != nullptr)
	{
		WSACloseEvent(Event);
		Event = nullptr;
	}
#else
	// close pipe
	close(Pipe[0]);
	close(Pipe[1]);
#endif

#ifdef HAVE_WINSOCK
	// release winsock
	ReleaseWinSock();
#endif

	// ok
	fInit = false;
	return true;
}

bool C4NetIOTCP::CloseBroadcast()
{
	return true;
}

#ifdef __APPLE__
static int fix_poll_timeout(int timeout) {
	if (timeout < 0 || timeout > 1000)
		return 1000;
	else
		return timeout;
}
#endif

bool C4NetIOTCP::Execute(int iMaxTime, pollfd *fds) // (mt-safe)
{
	// security
	if (!fInit) return false;

#ifdef STDSCHEDULER_USE_EVENTS
	// wait for something to happen
	if (WaitForSingleObject(Event, iMaxTime == C4NetIO::TO_INF ? INFINITE : iMaxTime) == WAIT_TIMEOUT)
		// timeout -> nothing happened
		return true;
	WSAResetEvent(Event);

	WSANETWORKEVENTS wsaEvents;
#else

#ifdef __APPLE__
	iMaxTime = fix_poll_timeout(iMaxTime);
#endif

	std::vector<pollfd> fdvec;
	std::map<SOCKET, const pollfd*> fdmap;
	if (!fds)
	{
		// build socket sets
		GetFDs(fdvec);
		fds = &fdvec[0];
		// wait for something to happen
		int ret = poll(fds, fdvec.size(), iMaxTime);
		// error
		if (ret < 0)
		{
			SetError("poll failed");
			return false;
		}
		// nothing happened
		if (ret == 0)
			return true;
	}
	else
	{
		// We need to know the size of fdvec, so construct the vector
		GetFDs(fdvec);
		// Now overwrite with the poll result
		std::copy(fds, fds + fdvec.size(), fdvec.begin());
	}

	// flush pipe
	assert(fdvec[0].fd == Pipe[0]);
	if (fdvec[0].events & fdvec[0].revents)
	{
		char c;
		if (::read(Pipe[0], &c, 1) == -1)
			SetError("read failed");
	}

	for (std::vector<pollfd>::const_iterator i = fdvec.begin(); i != fdvec.end(); ++i)
		fdmap[i->fd] = &*i;
	std::map<SOCKET, const pollfd*>::const_iterator cur_fd;
#endif

	// check sockets for events

	// first: the listen socket
	if (lsock != INVALID_SOCKET)
	{

#ifdef STDSCHEDULER_USE_EVENTS
		// get event list
		if (::WSAEnumNetworkEvents(lsock, nullptr, &wsaEvents) == SOCKET_ERROR)
			return false;

		// a connection waiting for accept?
		if (wsaEvents.lNetworkEvents & FD_ACCEPT)
#else
		cur_fd = fdmap.find(lsock);
		// a connection waiting for accept?
		if (cur_fd != fdmap.end() && (cur_fd->second->events & cur_fd->second->revents))
#endif
			if (!Accept())
				return false;
		// (note: what happens if there are more connections waiting?)

#ifdef STDSCHEDULER_USE_EVENTS
		// closed?
		if (wsaEvents.lNetworkEvents & FD_CLOSE)
			// try to recreate the listen socket
			Listen(iListenPort);
#endif
	}

	// second: waited-for connection
	CStdShareLock PeerListLock(&PeerListCSec);
	for (ConnectWait *pWait = pConnectWaits, *pNext; pWait; pWait = pNext)
	{
		pNext = pWait->Next;

		// not closed?
		if (pWait->sock)
		{
#ifdef STDSCHEDULER_USE_EVENTS
			// get event list
			if (::WSAEnumNetworkEvents(pWait->sock, nullptr, &wsaEvents) == SOCKET_ERROR)
				return false;

			if (wsaEvents.lNetworkEvents & FD_CONNECT)
#else
			// got connection?
			cur_fd = fdmap.find(pWait->sock);
			if (cur_fd != fdmap.end() && (cur_fd->second->events & cur_fd->second->revents))
#endif
			{
				// remove from list
				SOCKET sock = pWait->sock; pWait->sock = 0;

#ifdef STDSCHEDULER_USE_EVENTS
				// error?
				if (wsaEvents.iErrorCode[FD_CONNECT_BIT])
				{
					// disconnect-callback
					if (pCB) pCB->OnDisconn(pWait->addr, this, GetSocketErrorMsg(wsaEvents.iErrorCode[FD_CONNECT_BIT]));
				}
				else
#else
				// get error code
				int iErrCode; socklen_t iErrCodeLen = sizeof(iErrCode);
				if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&iErrCode), &iErrCodeLen) != 0)
				{
					close(sock);
					if (pCB) pCB->OnDisconn(pWait->addr, this, GetSocketErrorMsg());
				}
				// error?
				else if (iErrCode)
				{
					close(sock);
					if (pCB) pCB->OnDisconn(pWait->addr, this, GetSocketErrorMsg(iErrCode));
				}
				else
#endif
					// accept connection, do callback
					if (!Accept(sock, pWait->addr))
						return false;
			}
		}

	}

	// last: all connected sockets
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->Open())
		{
			SOCKET sock = pPeer->GetSocket();

#ifdef STDSCHEDULER_USE_EVENTS
			// get event list
			if (::WSAEnumNetworkEvents(sock, nullptr, &wsaEvents) == SOCKET_ERROR)
				return false;

			// something to read from socket?
			if (wsaEvents.lNetworkEvents & FD_READ)
#else
			// something to read from socket?
			cur_fd = fdmap.find(sock);
			if (cur_fd != fdmap.end() && (POLLIN & cur_fd->second->revents))
#endif
				for (;;)
				{
					// how much?
#ifdef _WIN32
					DWORD iBytesToRead;
#else
					int iBytesToRead;
#endif
					if (::ioctlsocket(pPeer->GetSocket(), FIONREAD, &iBytesToRead) == SOCKET_ERROR)
					{
						pPeer->Close();
						if (pCB) pCB->OnDisconn(pPeer->GetAddr(), this, GetSocketErrorMsg());
						break;
					}
					// The following two lines of code will make sure that if the variable
					// "iBytesToRead" is zero, it will be increased by one.
					// In this case, it will hold the value 1 after the operation.
					// Note it doesn't do anything for negative values.
					// (This comment has been sponsored by Sven2)
					if (!iBytesToRead)
						++iBytesToRead;
					// get buffer
					void *pBuf = pPeer->GetRecvBuf(iBytesToRead);
					// read a buffer full of data from socket
					int iBytesRead;
					if ((iBytesRead = ::recv(sock, reinterpret_cast<char *>(pBuf), iBytesToRead, 0)) == SOCKET_ERROR)
					{
						// Would block? Ok, let's try this again later
						if (HaveWouldBlockError()) { ResetSocketError(); break; }
						// So he's serious after all...
						pPeer->Close  ();
						if (pCB) pCB->OnDisconn(pPeer->GetAddr(), this, GetSocketErrorMsg());
						break;
					}
					// nothing? this means the conection was closed, if you trust in linux manpages.
					if (!iBytesRead)
					{
						pPeer->Close();
						if (pCB) pCB->OnDisconn(pPeer->GetAddr(), this, "connection closed");
						break;
					}
					// pass to Peer::OnRecv
					pPeer->OnRecv(iBytesRead);
				}

			// socket has become writeable?
#ifdef STDSCHEDULER_USE_EVENTS
			if (wsaEvents.lNetworkEvents & FD_WRITE)
#else
			if (cur_fd != fdmap.end() && (POLLOUT & cur_fd->second->revents))
#endif
				// send remaining data
				pPeer->Send();

#ifdef STDSCHEDULER_USE_EVENTS
			// socket was closed?
			if (wsaEvents.lNetworkEvents & FD_CLOSE)
			{
				const char *szReason = wsaEvents.iErrorCode[FD_CLOSE_BIT] ? GetSocketErrorMsg(wsaEvents.iErrorCode[FD_CLOSE_BIT]) : "closed by peer";
				// close socket
				pPeer->Close();
				// do callback
				if (pCB) pCB->OnDisconn(pPeer->GetAddr(), this, szReason);
			}
#endif
		}

	// done
	return true;
}

C4NetIOTCP::Socket::~Socket()
{
	if (sock != INVALID_SOCKET)
		closesocket(sock);
}

C4NetIO::addr_t C4NetIOTCP::Socket::GetAddress()
{
	sockaddr_in6 addr;
	socklen_t address_len = sizeof addr;
	C4NetIO::addr_t result;
	if (::getsockname(sock, (sockaddr*) &addr, &address_len) != SOCKET_ERROR)
	{
		result.SetAddress((sockaddr*) &addr);
	}
	return result;
}

SOCKET C4NetIOTCP::CreateSocket(addr_t::AddressFamily family)
{
	// create new socket
	SOCKET nsock = ::socket(family == HostAddress::IPv6 ? AF_INET6 : AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
	if (nsock == INVALID_SOCKET)
	{
		SetError("socket creation failed", true);
		return INVALID_SOCKET;
	}

	if (family == HostAddress::IPv6)
		if (!InitIPv6Socket(nsock))
		{
			closesocket(nsock);
			return INVALID_SOCKET;
		}

	return nsock;
}

std::unique_ptr<C4NetIOTCP::Socket> C4NetIOTCP::Bind(const C4NetIO::addr_t &addr) // (mt-safe)
{
	SOCKET nsock = CreateSocket(addr.GetFamily());
	if (nsock == INVALID_SOCKET) return nullptr;

	// bind the socket to the given address
	if (::bind(nsock, &addr, addr.GetAddrLen()) == SOCKET_ERROR)
	{
		SetError("binding the socket failed", true);
		closesocket(nsock);
		return nullptr;
	}
	return std::unique_ptr<Socket>(new Socket(nsock));
}

bool C4NetIOTCP::Connect(const addr_t &addr, std::unique_ptr<Socket> socket) // (mt-safe)
{
	SOCKET nsock = socket->sock;
	socket->sock = INVALID_SOCKET;
	return Connect(addr, nsock);
}

bool C4NetIOTCP::Connect(const C4NetIO::addr_t &addr) // (mt-safe)
{
	// create new socket
	SOCKET nsock = CreateSocket(addr.GetFamily());
	if (nsock == INVALID_SOCKET) return false;

	return Connect(addr, nsock);
}

bool C4NetIOTCP::Connect(const C4NetIO::addr_t &addr, SOCKET nsock) // (mt-safe)
{
#ifdef STDSCHEDULER_USE_EVENTS
	// set event
	if (::WSAEventSelect(nsock, Event, FD_CONNECT) == SOCKET_ERROR)
	{
		// set error
		SetError("connect failed: could not set event", true);
		closesocket(nsock);
		return false;
	}

	// add to list
	AddConnectWait(nsock, addr);

#elif defined(HAVE_WINSOCK)
	// disable blocking
	unsigned long iBlock = 1;
	if (::ioctlsocket(nsock, FIONBIO, &iBlock) == SOCKET_ERROR)
	{
		// set error
		SetError("connect failed: could not disable blocking", true);
		close(nsock);
		return false;
	}
#else
	// disable blocking
	if (::fcntl(nsock, F_SETFL, fcntl(nsock, F_GETFL) | O_NONBLOCK) == SOCKET_ERROR)
	{
		// set error
		SetError("connect failed: could not disable blocking", true);
		close(nsock);
		return false;
	}
#endif

	// connect (async)
	if (::connect(nsock, &addr, addr.GetAddrLen()) == SOCKET_ERROR)
	{
		if (!HaveWouldBlockError()) // expected
		{
			SetError("socket connection failed", true);
			closesocket(nsock);
			return false;
		}
	}

#ifndef STDSCHEDULER_USE_EVENTS
	// add to list
	AddConnectWait(nsock, addr);
#endif

	// ok
	return true;
}

bool C4NetIOTCP::Close(const addr_t &addr) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// find connect wait
	ConnectWait *pWait = GetConnectWait(addr);
	if (pWait)
	{
		// close socket, do callback
		closesocket(pWait->sock); pWait->sock = 0;
		if (pCB) pCB->OnDisconn(pWait->addr, this, "closed");
	}
	else
	{
		// find peer
		Peer *pPeer = GetPeer(addr);
		if (pPeer)
		{
			C4NetIO::addr_t addr = pPeer->GetAddr();
			// close peer
			pPeer->Close();
			// do callback
			if (pCB) pCB->OnDisconn(addr, this, "closed");
		}
		// not found
		else
			return false;
	}
	// ok
	return true;
}

bool C4NetIOTCP::Send(const C4NetIOPacket &rPacket) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// find peer
	Peer *pPeer = GetPeer(rPacket.getAddr());
	// not found?
	if (!pPeer) return false;
	// send
	return pPeer->Send(rPacket);
}

bool C4NetIOTCP::SetBroadcast(const addr_t &addr, bool fSet) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// find peer
	Peer *pPeer = GetPeer(addr);
	if (!pPeer) return false;
	// set flag
	pPeer->SetBroadcast(fSet);
	return true;
}

bool C4NetIOTCP::Broadcast(const C4NetIOPacket &rPacket) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// just send to all clients
	bool fSuccess = true;
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->Open() && pPeer->doBroadcast())
			fSuccess &= Send(C4NetIOPacket(rPacket.getRef(), pPeer->GetAddr()));
	return fSuccess;
}

void C4NetIOTCP::UnBlock() // (mt-safe)
{
#ifdef STDSCHEDULER_USE_EVENTS
	// unblock WaitForSingleObject in C4NetIOTCP::Execute manually
	// by setting the Event
	WSASetEvent(Event);
#else
	// write one character to the pipe, this will unblock everything that
	// waits for the FD set returned by GetFDs.
	char c = 1;
	if (write(Pipe[1], &c, 1) == -1)
		SetError("write failed");
#endif
}

#ifdef STDSCHEDULER_USE_EVENTS
HANDLE C4NetIOTCP::GetEvent() // (mt-safe)
{
	return Event;
}
#else
void C4NetIOTCP::GetFDs(std::vector<struct pollfd> & fds)
{
	pollfd pfd; pfd.revents = 0;
	// add pipe
	pfd.fd = Pipe[0]; pfd.events = POLLIN;
	fds.push_back(pfd);
	// add listener
	if (lsock != INVALID_SOCKET)
	{
		pfd.fd = lsock; pfd.events = POLLIN;
		fds.push_back(pfd);
	}
	// add connect waits (wait for them to become writeable)
	CStdShareLock PeerListLock(&PeerListCSec);
	for (ConnectWait *pWait = pConnectWaits; pWait; pWait = pWait->Next)
	{
		pfd.fd = pWait->sock; pfd.events = POLLOUT;
		fds.push_back(pfd);
	}
	// add sockets
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->GetSocket())
		{
			// Wait for socket to become readable
			pfd.fd = pPeer->GetSocket(); pfd.events = POLLIN;
			// Wait for socket to become writeable, if there is data waiting
			if (pPeer->hasWaitingData())
			{
				pfd.events |= POLLOUT;
			}
			fds.push_back(pfd);
		}
}
#endif

bool C4NetIOTCP::GetStatistic(int *pBroadcastRate) // (mt-safe)
{
	// no broadcast
	if (pBroadcastRate) *pBroadcastRate = 0;
	return true;
}

bool C4NetIOTCP::GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// find peer
	Peer *pPeer = GetPeer(addr);
	if (!pPeer || !pPeer->Open()) return false;
	// return statistics
	if (pIRate) *pIRate = pPeer->GetIRate();
	if (pORate) *pORate = pPeer->GetORate();
	if (pLoss) *pLoss = 0;
	return true;
}

void C4NetIOTCP::ClearStatistic()
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// clear all peer statistics
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		pPeer->ClearStatistics();
}

C4NetIOTCP::Peer *C4NetIOTCP::Accept(SOCKET nsock, const addr_t &ConnectAddr) // (mt-safe)
{

	addr_t caddr = ConnectAddr;

	// accept incoming connection?
	C4NetIO::addr_t addr; socklen_t iAddrSize = addr.GetAddrLen();
	if (nsock == INVALID_SOCKET)
	{
		// accept from listener
#ifdef __linux__
		if ((nsock = ::accept4(lsock, &addr, &iAddrSize, SOCK_CLOEXEC)) == INVALID_SOCKET)
#else
		if ((nsock = ::accept(lsock, &addr, &iAddrSize)) == INVALID_SOCKET)
#endif
		{
			// set error
			SetError("socket accept failed", true);
			return nullptr;
		}
		// connect address unknown, so zero it
		caddr.Clear();
	}
	else
	{
		// get peer address
		if (::getpeername(nsock, &addr, &iAddrSize) == SOCKET_ERROR)
		{
#ifndef HAVE_WINSOCK
			// getpeername behaves strangely on exotic platforms. Just ignore it.
			if (errno != ENOTCONN)
			{
#endif
				// set error
				SetError("could not get peer address for connected socket", true);
				return nullptr;
#ifndef HAVE_WINSOCK
			}
#endif
		}
	}

	// check address
	if (addr.GetFamily() == addr_t::UnknownFamily)
	{
		// set error
		SetError("socket accept failed: invalid address returned");
		closesocket(nsock);
		return nullptr;
	}

	// disable nagle (yep, we know what we are doing here - I think)
	int iNoDelay = 1;
	::setsockopt(nsock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&iNoDelay), sizeof(iNoDelay));

#ifdef STDSCHEDULER_USE_EVENTS
	// set event
	if (::WSAEventSelect(nsock, Event, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
	{
		// set error
		SetError("connection accept failed: could not set event", true);
		closesocket(nsock);
		return nullptr;
	}
#elif defined(HAVE_WINSOCK)
	// disable blocking
	unsigned long iBlock = 1;
	if (::ioctlsocket(nsock, FIONBIO, &iBlock) == SOCKET_ERROR)
	{
		// set error
		SetError("connect failed: could not disable blocking", true);
		close(nsock);
		return false;
	}
#else
	// disable blocking
	if (::fcntl(nsock, F_SETFL, fcntl(nsock, F_GETFL) | O_NONBLOCK) == SOCKET_ERROR)
	{
		// set error
		SetError("connection accept failed: could not disable blocking", true);
		close(nsock);
		return nullptr;
	}
#endif


	// create new peer
	Peer *pnPeer = new Peer(addr, nsock, this);

	// get required locks to add item to list
	CStdShareLock PeerListLock(&PeerListCSec);
	CStdLock PeerListAddLock(&PeerListAddCSec);

	// add to list
	pnPeer->Next = pPeerList;
	pPeerList = pnPeer;

	// clear add-lock
	PeerListAddLock.Clear();
	
	Changed();

	// ask callback if connection should be permitted
	if (pCB && !pCB->OnConn(addr, caddr, nullptr, this))
		// close socket immediately (will be deleted later)
		pnPeer->Close();

	// ok
	return pnPeer;
}

bool C4NetIOTCP::Listen(uint16_t inListenPort)
{
	// already listening?
	if (lsock != INVALID_SOCKET)
		// close existing socket
		closesocket(lsock);
	iListenPort = addr_t::IPPORT_NONE;

	// create socket
	if ((lsock = ::socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		SetError("socket creation failed", true);
		return false;
	}
	if (!InitIPv6Socket(lsock))
		return false;
	// To be able to reuse the port after close
#if !defined(_DEBUG) && !defined(_WIN32)
	int reuseaddr = 1;
	setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuseaddr), sizeof(reuseaddr));
#endif
	// bind listen socket
	addr_t addr = addr_t::Any;
	addr.SetPort(inListenPort);
	if (::bind(lsock, &addr, addr.GetAddrLen()) == SOCKET_ERROR)
	{
		SetError("socket bind failed", true);
		closesocket(lsock); lsock = INVALID_SOCKET;
		return false;
	}

#ifdef STDSCHEDULER_USE_EVENTS
	// set event callback
	if (::WSAEventSelect(lsock, Event, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
	{
		SetError("could not set event for listen socket", true);
		closesocket(lsock); lsock = INVALID_SOCKET;
		return false;
	}
#endif

	// start listening
	if (::listen(lsock, SOMAXCONN) == SOCKET_ERROR)
	{
		SetError("socket listen failed", true);
		closesocket(lsock); lsock = INVALID_SOCKET;
		return false;
	}

	// ok
	iListenPort = inListenPort;
	Changed();
	return true;
}

C4NetIOTCP::Peer *C4NetIOTCP::GetPeer(const addr_t &addr) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->Open())
			if (pPeer->GetAddr() == addr)
				return pPeer;
	return nullptr;
}

void C4NetIOTCP::OnShareFree(CStdCSecEx *pCSec)
{
	if (pCSec == &PeerListCSec)
	{
		// clear up
		Peer *pPeer = pPeerList, *pLast = nullptr;
		while (pPeer)
		{
			// delete?
			if (!pPeer->Open())
			{
				// unlink
				Peer *pDelete = pPeer;
				pPeer = pPeer->Next;
				(pLast ? pLast->Next : pPeerList) = pPeer;
				// delete
				delete pDelete;
			}
			else
			{
				// next peer
				pLast = pPeer;
				pPeer = pPeer->Next;
			}
		}
		ConnectWait *pWait = pConnectWaits, *pWLast = nullptr;
		while (pWait)
		{
			// delete?
			if (!pWait->sock)
			{
				// unlink
				ConnectWait *pDelete = pWait;
				pWait = pWait->Next;
				(pWLast ? pWLast->Next : pConnectWaits) = pWait;
				// delete
				delete pDelete;
			}
			else
			{
				// next peer
				pWLast = pWait;
				pWait = pWait->Next;
			}
		}
	}
}

void C4NetIOTCP::AddConnectWait(SOCKET sock, const addr_t &addr) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	CStdLock PeerListAddLock(&PeerListAddCSec);
	// create new entry, add to list
	ConnectWait *pnWait = new ConnectWait;
	pnWait->sock = sock; pnWait->addr = addr;
	pnWait->Next = pConnectWaits;
	pConnectWaits = pnWait;
#ifndef STDSCHEDULER_USE_EVENTS
	// unblock, so new FD can be realized
	UnBlock();
#endif
	Changed();
}

C4NetIOTCP::ConnectWait *C4NetIOTCP::GetConnectWait(const addr_t &addr) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// search
	for (ConnectWait *pWait = pConnectWaits; pWait; pWait = pWait->Next)
		if (pWait->addr == addr)
			return pWait;
	return nullptr;
}

void C4NetIOTCP::ClearConnectWaits() // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	for (ConnectWait *pWait = pConnectWaits; pWait; pWait = pWait->Next)
		if (pWait->sock)
		{
			closesocket(pWait->sock);
			pWait->sock = 0;
		}
}

void C4NetIOTCP::PackPacket(const C4NetIOPacket &rPacket, StdBuf &rOutBuf)
{
	// packet data
	uint8_t cFirstByte = 0xff;
	uint32_t iSize = rPacket.getSize();
	uint32_t iOASize = sizeof(cFirstByte) + sizeof(iSize) + iSize;

	// enlarge buffer
	int iPos = rOutBuf.getSize();
	rOutBuf.Grow(iOASize);

	// write packet at end of outgoing buffer
	*getMBufPtr<uint8_t>(rOutBuf, iPos) = cFirstByte; iPos += sizeof(uint8_t);
	*getMBufPtr<uint32_t>(rOutBuf, iPos) = iSize; iPos += sizeof(uint32_t);
	rOutBuf.Write(rPacket, iPos);
}

size_t C4NetIOTCP::UnpackPacket(const StdBuf &IBuf, const C4NetIO::addr_t &addr)
{
	size_t iPos = 0;
	// check first byte (should be 0xff)
	if (*getBufPtr<uint8_t>(IBuf, iPos) != (uint8_t) 0xff)
		// clear buffer
		return IBuf.getSize();
	iPos += sizeof(char);
	// read packet size
	uint32_t iPacketSize;
	if (iPos + sizeof(uint32_t) > IBuf.getSize())
		return 0;
	iPacketSize = *getBufPtr<uint32_t>(IBuf, iPos);
	iPos += sizeof(uint32_t);
	// packet incomplete?
	if (iPos + iPacketSize > IBuf.getSize())
		return 0;
	// ok, call back
	if (pCB) pCB->OnPacket(C4NetIOPacket(IBuf.getPart(iPos, iPacketSize), addr), this);
	// absorbed
	return iPos + iPacketSize;
}

// * C4NetIOTCP::Peer

const unsigned int C4NetIOTCP::Peer::iTCPHeaderSize = 28 + 24; // (bytes)
const unsigned int C4NetIOTCP::Peer::iMinIBufSize = 8192; // (bytes)

// construction / destruction

C4NetIOTCP::Peer::Peer(const C4NetIO::addr_t &naddr, SOCKET nsock, C4NetIOTCP *pnParent)
		: pParent(pnParent),
		addr(naddr), sock(nsock),
		iIBufUsage(0), iIRate(0), iORate(0),
		fOpen(true), fDoBroadcast(false), Next(nullptr)
{
}

C4NetIOTCP::Peer::~Peer()
{
	// close socket
	Close();
}

// implementation

bool C4NetIOTCP::Peer::Send(const C4NetIOPacket &rPacket) // (mt-safe)
{
	CStdLock OLock(&OCSec);

	// already data pending to be sent? try to sent them first (empty buffer)
	if (!OBuf.isNull()) Send();
	bool fSend = OBuf.isNull();

	// pack packet
	pParent->PackPacket(rPacket, OBuf);

	// (try to) send
	return fSend ? Send() : true;
}

bool C4NetIOTCP::Peer::Send() // (mt-safe)
{
	CStdLock OLock(&OCSec);
	if (OBuf.isNull()) return true;

	// send as much as possibile
	int iBytesSent;
	if ((iBytesSent = ::send(sock, getBufPtr<char>(OBuf), OBuf.getSize(), 0)) == SOCKET_ERROR)
		if (!HaveWouldBlockError())
		{
			pParent->SetError("send failed", true);
			return false;
		}

	// nothin sent?
	if (iBytesSent == SOCKET_ERROR || !iBytesSent) return true;

	// increase output rate
	iORate += iBytesSent + iTCPHeaderSize;

	// data remaining?
	if (unsigned(iBytesSent) < OBuf.getSize())
	{
		// Shrink buffer
		OBuf.Move(iBytesSent, OBuf.getSize() - iBytesSent);
		OBuf.Shrink(iBytesSent);
#ifndef STDSCHEDULER_USE_EVENTS
		// Unblock parent so the FD-list can be refreshed
		pParent->UnBlock();
#endif
	}
	else
		// just delete buffer
		OBuf.Clear();

	// ok
	return true;
}

void *C4NetIOTCP::Peer::GetRecvBuf(int iSize) // (mt-safe)
{
	CStdLock ILock(&ICSec);
	// Enlarge input buffer?
	size_t iIBufSize = std::max<size_t>(iMinIBufSize, IBuf.getSize());
	while ((size_t)(iIBufUsage + iSize) > iIBufSize)
		iIBufSize *= 2;
	if (iIBufSize != IBuf.getSize())
		IBuf.SetSize(iIBufSize);
	// Return the appropriate part of the input buffer
	return IBuf.getMPtr(iIBufUsage);
}

void C4NetIOTCP::Peer::OnRecv(int iSize) // (mt-safe)
{
	CStdLock ILock(&ICSec);
	// increase input rate and input buffer usage
	iIRate += iTCPHeaderSize + iSize;
	iIBufUsage += iSize;
	// a prior call to GetRecvBuf should have ensured this
	assert(static_cast<size_t>(iIBufUsage) <= IBuf.getSize());
	// read packets
	size_t iPos = 0, iPacketPos;
	while ((iPacketPos = iPos) < (size_t)iIBufUsage)
	{
		// Try to unpack a packet
		StdBuf IBufPart = IBuf.getPart(iPos, iIBufUsage - iPos);
		int32_t iBytes = pParent->UnpackPacket(IBufPart, addr);
		// Could not unpack?
		if (!iBytes)
			break;
		// Advance
		iPos += iBytes;
	}
	// data left?
	if (iPacketPos < (size_t) iIBufUsage)
	{
		// no packet read?
		if (!iPacketPos) return;
		// move data
		IBuf.Move(iPacketPos, IBuf.getSize() - iPacketPos);
		iIBufUsage -= iPacketPos;
		// shrink buffer
		size_t iIBufSize = IBuf.getSize();
		while ((size_t) iIBufUsage <= iIBufSize / 2)
			iIBufSize /= 2;
		if (iIBufSize != IBuf.getSize())
			IBuf.Shrink(iPacketPos);
	}
	else
	{
		// the buffer is empty
		iIBufUsage = 0;
		// shrink buffer to minimum
		if (IBuf.getSize() > iMinIBufSize)
			IBuf.SetSize(iMinIBufSize);
	}
}

void C4NetIOTCP::Peer::Close() // (mt-safe)
{
	CStdLock ILock(&ICSec); CStdLock OLock(&OCSec);
	if (!fOpen) return;
	// close socket
	closesocket(sock);
	// set flag
	fOpen = false;
	// clear buffers
	IBuf.Clear(); OBuf.Clear();
	iIBufUsage = 0;
	// reset statistics
	iIRate = iORate = 0;
}

void C4NetIOTCP::Peer::ClearStatistics() // (mt-safe)
{
	CStdLock ILock(&ICSec); CStdLock OLock(&OCSec);
	iIRate = iORate = 0;
}

// *** C4NetIOSimpleUDP

C4NetIOSimpleUDP::C4NetIOSimpleUDP()
		: iPort(~0), sock(INVALID_SOCKET)
{

}

C4NetIOSimpleUDP::~C4NetIOSimpleUDP()
{
	Close();
}

bool C4NetIOSimpleUDP::Init(uint16_t inPort)
{
	// reset error
	ResetError();

	// already initialized? close first
	if (fInit) Close();

#ifdef HAVE_WINSOCK
	// init winsock
	if (!AcquireWinSock())
	{
		SetError("could not start winsock");
		return false;
	}
#endif

	// create sockets
	if ((sock = ::socket(AF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		SetError("could not create socket", true);
		return false;
	}

	if (!InitIPv6Socket(sock))
		return false;

	// set reuse socket option
	if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&fAllowReUse), sizeof fAllowReUse) == SOCKET_ERROR)
	{
		SetError("could not set reuse options", true);
		return false;
	}

	// bind socket
	iPort = inPort;
	addr_t naddr = addr_t::Any;
	naddr.SetPort(iPort);
	if (::bind(sock, &naddr, sizeof(naddr)) == SOCKET_ERROR)
	{
		SetError("could not bind socket", true);
		return false;
	}

#ifdef STDSCHEDULER_USE_EVENTS

	// create event
	if ((hEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		SetError("could not create event", true);
		return false;
	}

	// set event for socket
	if (WSAEventSelect(sock, hEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		SetError("could not select event", true);
		return false;
	}

#else

	// disable blocking
	if (::fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK) == SOCKET_ERROR)
	{
		// set error
		SetError("could not disable blocking", true);
		return false;
	}

	// create pipe
	if (pipe(Pipe) != 0)
	{
		SetError("could not create pipe", true);
		return false;
	}

#endif

	// set flags
	fInit = true;
	fMultiCast = false;

	// ok, that's all for know.
	// call InitBroadcast for more initialization fun
	return true;
}

bool C4NetIOSimpleUDP::InitBroadcast(addr_t *pBroadcastAddr)
{
	// no error... yet
	ResetError();

	// security
	if (!pBroadcastAddr) return false;

	// Init() has to be called first
	if (!fInit) return false;
	// already activated?
	if (fMultiCast) CloseBroadcast();

	// broadcast addr valid?
	if (!pBroadcastAddr->IsMulticast() || pBroadcastAddr->GetFamily() != HostAddress::IPv6)
	{
		SetError("invalid broadcast address (only IPv6 multicast addresses are supported)");
		return false;
	}
	if (pBroadcastAddr->GetPort() != iPort)
	{
		SetError("invalid broadcast address (different port)");
		return false;
	}

	// set mc ttl to somewhat about "same net"
	int TTL = 16;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<char*>(&TTL), sizeof(TTL)) == SOCKET_ERROR)
	{
		SetError("could not set mc ttl", true);
		return false;
	}

	// set up multicast group information
	this->MCAddr = *pBroadcastAddr;
	MCGrpInfo.ipv6mr_multiaddr = static_cast<sockaddr_in6>(MCAddr).sin6_addr;
	// TODO: do multicast on all interfaces?
	MCGrpInfo.ipv6mr_interface = 0; // default interface

	// join multicast group
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
	               reinterpret_cast<const char *>(&MCGrpInfo), sizeof(MCGrpInfo)) == SOCKET_ERROR)
	{
		SetError("could not join multicast group"); // to do: more error information
		return false;
	}

	// (try to) disable loopback (will set fLoopback accordingly)
	SetMCLoopback(false);

	// ok
	fMultiCast = true;
	return true;
}

bool C4NetIOSimpleUDP::Close()
{
	// should be initialized
	if (!fInit) return true;

	ResetError();

	// deactivate multicast
	if (fMultiCast)
		CloseBroadcast();

	// close sockets
	if (sock != INVALID_SOCKET)
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
	}

#ifdef STDSCHEDULER_USE_EVENTS
	// close event
	if (hEvent != nullptr)
	{
		WSACloseEvent(hEvent);
		hEvent = nullptr;
	}
#else
	// close pipes
	close(Pipe[0]);
	close(Pipe[1]);
#endif

#ifdef HAVE_WINSOCK
	// release winsock
	ReleaseWinSock();
#endif

	// ok
	fInit = false;
	return false;
}

bool C4NetIOSimpleUDP::CloseBroadcast()
{
	// multicast not active?
	if (!fMultiCast) return true;

	// leave multicast group
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
	               reinterpret_cast<const char *>(&MCGrpInfo), sizeof(MCGrpInfo)) == SOCKET_ERROR)
	{
		SetError("could not leave multicast group"); // to do: more error information
		return false;
	}

	// ok
	fMultiCast = false;
	return true;
}

bool C4NetIOSimpleUDP::Execute(int iMaxTime, pollfd *)
{
	if (!fInit) { SetError("not yet initialized"); return false; }
	ResetError();

#ifdef __APPLE__
	iMaxTime = fix_poll_timeout(iMaxTime);
#endif

	// wait for socket / timeout
	WaitResult eWR = WaitForSocket(iMaxTime);
	if (eWR == WR_Error) return false;

	// cancelled / timeout?
	if (eWR == WR_Cancelled || eWR == WR_Timeout) return true;
	assert(eWR == WR_Readable);

	// read packets from socket
	for (;;)
	{
		// how much can be read?
#ifdef _WIN32
		u_long iMaxMsgSize;
#else
		// The FIONREAD ioctl call takes an int on unix
		int iMaxMsgSize;
#endif
		if (::ioctlsocket(sock, FIONREAD, &iMaxMsgSize) == SOCKET_ERROR)
		{
			SetError("Could not determine the amount of data that can be read from socket", true);
			return false;
		}

		// nothing?
		if (!iMaxMsgSize)
			break;
		// alloc buffer
		C4NetIOPacket Pkt; Pkt.New(iMaxMsgSize);
		// read data (note: it is _not_ garantueed that iMaxMsgSize bytes are available)
		addr_t SrcAddr; socklen_t iSrcAddrLen = sizeof(sockaddr_in6);
		int iMsgSize = ::recvfrom(sock, getMBufPtr<char>(Pkt), iMaxMsgSize, 0, &SrcAddr, &iSrcAddrLen);
		// error?
		if (iMsgSize == SOCKET_ERROR)
		{
			if (HaveConnResetError())
			{
				// this is actually some kind of notification: an ICMP msg (unreachable)
				// came back, so callback and continue reading
				if (pCB) pCB->OnDisconn(SrcAddr, this, GetSocketErrorMsg());
				continue;
			}
			else
			{
				// this is the real thing, though
				SetError("could not receive data from socket", true);
				return false;
			}
		}
		// invalid address?
		if ((iSrcAddrLen != sizeof(sockaddr_in) && iSrcAddrLen != sizeof(sockaddr_in6)) || SrcAddr.GetFamily() == addr_t::UnknownFamily)
		{
			SetError("recvfrom returned an invalid address");
			return false;
		}
		// again: nothing?
		if (!iMsgSize)
			// docs say that the connection has been closed (whatever that means for a connectionless socket...)
			// let's just pretend it didn't happen, but stop reading.
			break;
		// fill in packet information
		Pkt.SetSize(iMsgSize);
		Pkt.SetAddr(SrcAddr);
		// callback
		if (pCB) pCB->OnPacket(Pkt, this);
	}

	// ok
	return true;
}

bool C4NetIOSimpleUDP::Send(const C4NetIOPacket &rPacket)
{
	if (!fInit) { SetError("not yet initialized"); return false; }

	// send it
	C4NetIO::addr_t addr = rPacket.getAddr();
	if (::sendto(sock, getBufPtr<char>(rPacket), rPacket.getSize(), 0,
	             &addr, addr.GetAddrLen())
	    != int(rPacket.getSize()) &&
	    !HaveWouldBlockError())
	{
		SetError("socket sendto failed", true);
		return false;
	}

	// ok
	ResetError();
	return true;
}

bool C4NetIOSimpleUDP::Broadcast(const C4NetIOPacket &rPacket)
{
	// just set broadcast address and send
	return C4NetIOSimpleUDP::Send(C4NetIOPacket(rPacket.getRef(), MCAddr));
}

#ifdef STDSCHEDULER_USE_EVENTS

void C4NetIOSimpleUDP::UnBlock() // (mt-safe)
{
	// unblock WaitForSingleObject in C4NetIOTCP::Execute manually
	// by setting the Event
	WSASetEvent(hEvent);
}

HANDLE C4NetIOSimpleUDP::GetEvent() // (mt-safe)
{
	return hEvent;
}

enum C4NetIOSimpleUDP::WaitResult C4NetIOSimpleUDP::WaitForSocket(int iTimeout)
{
	// wait for anything to happen
	DWORD ret = WaitForSingleObject(hEvent, iTimeout == TO_INF ? INFINITE : iTimeout);
	if (ret == WAIT_TIMEOUT)
		return WR_Timeout;
	if (ret == WAIT_FAILED)
		{ SetError("Wait for Event failed"); return WR_Error; }
	// get socket events (and reset the event)
	WSANETWORKEVENTS wsaEvents;
	if (WSAEnumNetworkEvents(sock, hEvent, &wsaEvents) == SOCKET_ERROR)
		{ SetError("could not enumerate network events!"); return WR_Error; }
	// socket readable?
	if (wsaEvents.lNetworkEvents | FD_READ)
		return WR_Readable;
	// in case the event was set without the socket beeing readable,
	// the operation has been cancelled (see Unblock())
	WSAResetEvent(hEvent);
	return WR_Cancelled;
}

#else // STDSCHEDULER_USE_EVENTS

void C4NetIOSimpleUDP::UnBlock() // (mt-safe)
{
	// write one character to the pipe, this will unblock everything that
	// waits for the FD set returned by GetFDs.
	char c = 42;
	if (write(Pipe[1], &c, 1) == -1)
		SetError("write failed");
}

void C4NetIOSimpleUDP::GetFDs(std::vector<struct pollfd> & fds)
{
	// add pipe
	pollfd pfd = { Pipe[0], POLLIN, 0 };
	fds.push_back(pfd);
	// add socket
	if (sock != INVALID_SOCKET)
	{
		pollfd pfd = { sock, POLLIN, 0 };
		fds.push_back(pfd);
	}
}

enum C4NetIOSimpleUDP::WaitResult C4NetIOSimpleUDP::WaitForSocket(int iTimeout)
{
	// get file descriptors
	std::vector<pollfd> fds;
	GetFDs(fds);
	// wait for anything to happen
	int ret = poll(&fds[0], fds.size(), iTimeout);
	// catch simple cases
	if (ret < 0)
		{ SetError("poll failed", true); return WR_Error; }
	if (!ret)
		return WR_Timeout;
	// flush pipe, if neccessary
	if (fds[0].revents & POLLIN)
	{
		char c;
		if (::read(Pipe[0], &c, 1) == -1)
			SetError("read failed");
	}
	// socket readable?
	return (sock != INVALID_SOCKET) && (fds[1].revents & POLLIN) ? WR_Readable : WR_Cancelled;
}

#endif // STDSCHEDULER_USE_EVENTS

bool C4NetIOSimpleUDP::SetMCLoopback(int fLoopback)
{
	// enable/disable MC loopback
	setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, reinterpret_cast<char *>(&fLoopback), sizeof fLoopback);
	// read result
	socklen_t iSize = sizeof(fLoopback);
	if (getsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, reinterpret_cast<char *>(&fLoopback), &iSize) == SOCKET_ERROR)
		return false;
	fMCLoopback = !! fLoopback;
	return true;
}

void C4NetIOSimpleUDP::SetReUseAddress(bool fAllow)
{
	fAllowReUse = fAllow;
}

// *** C4NetIOUDP

// * build options / constants / structures

// Check immediately when missing packets are detected?
#define C4NETIOUDP_OPT_RECV_CHECK_IMMEDIATE

// Protocol version
const unsigned int C4NetIOUDP::iVersion = 2;

// Standard timeout length
const unsigned int C4NetIOUDP::iStdTimeout = 1000; // (ms)

// Time interval for connection checks
// Equals the maximum time that C4NetIOUDP::Execute might block
const unsigned int C4NetIOUDP::iCheckInterval = 1000; // (ms)

const unsigned int C4NetIOUDP::iMaxOPacketBacklog = 10000;

const unsigned int C4NetIOUDP::iUDPHeaderSize = 8 + 24; // (bytes)

#pragma pack (push, 1)

// We need to adapt C4NetIO::addr_t to put it in our UDP packages.
// Previously, the sockaddr_in struct was just put in directly. This is
// horribly non-portable though, especially as the value of AF_INET6 differs
// between platforms.
struct C4NetIOUDP::BinAddr
{
	BinAddr() = default;
	BinAddr(const C4NetIO::addr_t& addr)
	{
		switch (addr.GetFamily())
		{
		case C4NetIO::HostAddress::IPv4:
		{
			type = 1;
			auto addr4 = static_cast<const sockaddr_in*>(&addr); 
			static_assert(sizeof(v4) == sizeof(addr4->sin_addr), "unexpected IPv4 address size");
			memcpy(&v4, &addr4->sin_addr, sizeof(v4));
			break;
		}
		case C4NetIO::HostAddress::IPv6:
		{
			type = 2;
			auto addr6 = static_cast<const sockaddr_in6*>(&addr); 
			static_assert(sizeof(v6) == sizeof(addr6->sin6_addr), "unexpected IPv6 address size");
			memcpy(&v6, &addr6->sin6_addr, sizeof(v6));
			break;
		}
		default:
			assert(!"Unexpected address family");
		}
		port = addr.GetPort();
	}

	operator C4NetIO::addr_t() const
	{
		C4NetIO::addr_t result;
		switch (type)
		{
		case 1:
		{
			sockaddr_in addr4 = sockaddr_in();
			addr4.sin_family = AF_INET;
			memcpy(&addr4.sin_addr, &v4, sizeof(v4));
			result.SetAddress(reinterpret_cast<sockaddr*>(&addr4));
			break;
		}
		case 2:
		{
			sockaddr_in6 addr6 = sockaddr_in6();
			addr6.sin6_family = AF_INET6;
			memcpy(&addr6.sin6_addr, &v6, sizeof(v6));
			result.SetAddress(reinterpret_cast<sockaddr*>(&addr6));
			break;
		}
		default:
			assert(!"Invalid address type");
		}
		result.SetPort(port);
		return result;
	}

	StdStrBuf ToString() const
	{
		return static_cast<C4NetIO::addr_t>(*this).ToString();
	}

	uint16_t port;
	uint8_t type{0};
	union
	{
		uint8_t v4[4];
		uint8_t v6[16];
	};
};

// packet structures
struct C4NetIOUDP::PacketHdr
{
	uint8_t  StatusByte;
	uint32_t Nr;    // packet nr
};

struct C4NetIOUDP::ConnPacket : public PacketHdr
{
	uint32_t ProtocolVer;
	BinAddr Addr;
	BinAddr MCAddr;
};

struct C4NetIOUDP::ConnOKPacket : public PacketHdr
{
	enum { MCM_NoMC, MCM_MC, MCM_MCOK } MCMode;
	BinAddr Addr;
};

struct C4NetIOUDP::AddAddrPacket : public PacketHdr
{
	BinAddr Addr;
	BinAddr NewAddr;
};

struct C4NetIOUDP::DataPacketHdr : public PacketHdr
{
	Packet::nr_t FNr;   // start fragment of this series
	uint32_t Size;  // packet size (all fragments)
};

struct C4NetIOUDP::CheckPacketHdr : public PacketHdr
{
	uint32_t AskCount, MCAskCount;
	uint32_t AckNr, MCAckNr; // numbers of the last packets received
};

struct C4NetIOUDP::ClosePacket : public PacketHdr
{
	BinAddr Addr;
};


struct C4NetIOUDP::TestPacket : public PacketHdr
{
	unsigned int TestNr;
};

#pragma pack (pop)

// construction / destruction

C4NetIOUDP::C4NetIOUDP()
		: PeerListCSec(this),
		iPort(~0),
		tNextCheck(C4TimeMilliseconds::PositiveInfinity),
		OPackets(iMaxOPacketBacklog)
{

}

C4NetIOUDP::~C4NetIOUDP()
{
	Close();
}

bool C4NetIOUDP::Init(uint16_t inPort)
{
	// already initialized? close first
	if (fInit) Close();

#ifdef C4NETIO_DEBUG
	// open log
	OpenDebugLog();
#endif

	// Initialize UDP
	if (!C4NetIOSimpleUDP::Init(inPort))
		return false;
	iPort = inPort;

	// set callback
	C4NetIOSimpleUDP::SetCallback(CBProxy(this));

	// set flags
	fInit = true;
	fMultiCast = false;

	tNextCheck = C4TimeMilliseconds::Now() + iCheckInterval;

	// ok, that's all for now.
	// call InitBroadcast for more initialization fun
	return true;
}

bool C4NetIOUDP::InitBroadcast(addr_t *pBroadcastAddr)
{
	// no error... yet
	ResetError();

	// security
	if (!pBroadcastAddr) return false;

	// Init() has to be called first
	if (!fInit) return false;
	// already activated?
	if (fMultiCast) CloseBroadcast();

	// set up multicast group information
	C4NetIO::addr_t MCAddr = *pBroadcastAddr;

	// broadcast addr valid?
	if (!MCAddr.IsMulticast())
	{
		// port is needed in order to search a mc address automatically
		if (!iPort)
		{
			SetError("broadcast address is not valid");
			return false;
		}
		// Set up address as unicast-prefix-based IPv6 multicast address (RFC 3306).
		sockaddr_in6 saddrgen = sockaddr_in6();
		saddrgen.sin6_family = AF_INET6;
		uint8_t *addrgen = saddrgen.sin6_addr.s6_addr;
		// ff3e ("global multicast based on network prefix") : 64 (length of network prefix)
		static const uint8_t mcast_prefix[4] = { 0xff, 0x3e, 0, 64};
		memcpy(addrgen, mcast_prefix, sizeof(mcast_prefix));
		addrgen += sizeof(mcast_prefix);
		// 64 bit network prefix
		addr_t prefixAddr;
		for (auto& addr : GetLocalAddresses())
			if (addr.GetFamily() == HostAddress::IPv6 && !addr.IsLocal())
			{
				prefixAddr.SetAddress(addr);
				break;
			}
		if (prefixAddr.IsNull())
		{
			SetError("no IPv6 unicast address available");
			return false;
		}
		static const size_t network_prefix_size = 8;
		memcpy(addrgen, &static_cast<sockaddr_in6*>(&prefixAddr)->sin6_addr, network_prefix_size);
		addrgen += network_prefix_size;
		// 32 bit group id: search for a free one
		for (int iRetries = 1000; iRetries; iRetries--)
		{
			uint32_t rnd = UnsyncedRandom();
			memcpy(addrgen, &rnd, sizeof(rnd));
			// "high-order bit of the Group ID will be the same value as the T flag"
			addrgen[0] |= 0x80;
			// create new - random - address
			MCAddr.SetAddress((sockaddr*) &saddrgen);
			MCAddr.SetPort(iPort);
			// init broadcast
			if (!C4NetIOSimpleUDP::InitBroadcast(&MCAddr))
				return false;
			// do the loopback test
			if (!DoLoopbackTest())
			{
				C4NetIOSimpleUDP::CloseBroadcast();
				if (!GetError()) SetError("multicast loopback test failed");
				return false;
			}
			// send a ping packet
			const PacketHdr PingPacket = { IPID_Ping | static_cast<uint8_t>(0x80u), 0 };
			if (!C4NetIOSimpleUDP::Broadcast(C4NetIOPacket(&PingPacket, sizeof(PingPacket))))
			{
				C4NetIOSimpleUDP::CloseBroadcast();
				return false;
			}
			bool fSuccess = false;
			for (;;)
			{
				fSavePacket = true; LastPacket.Clear();
				// wait for something to happen
				if (!C4NetIOSimpleUDP::Execute(iStdTimeout))
				{
					fSavePacket = false;
					C4NetIOSimpleUDP::CloseBroadcast();
					return false;
				}
				fSavePacket = false;
				// Timeout? So expect this address to be unused
				if (LastPacket.isNull()) { fSuccess = true; break; }
				// looped back?
				if (C4NetIOSimpleUDP::getMCLoopback() && LastPacket.getAddr() == MCLoopbackAddr)
					// ignore this one
					continue;
				// otherwise: there must be someone else in this MC group
				C4NetIOSimpleUDP::CloseBroadcast();
				break;
			}
			if (fSuccess) break;
			// no success? try again...
		}

		// return found address
		*pBroadcastAddr = MCAddr;
	}
	else
	{
		// check: must be same port
		if (MCAddr.GetPort() == iPort)
		{
			SetError("invalid multicast address: wrong port");
			return false;
		}
		// init
		if (!C4NetIOSimpleUDP::InitBroadcast(&MCAddr))
			return false;
		// do loopback test (if not delayed)
		if (!fDelayedLoopbackTest)
			if (!DoLoopbackTest())
			{
				C4NetIOSimpleUDP::CloseBroadcast();
				if (!GetError()) SetError("multicast loopback test failed");
				return false;
			}
	}

	// (try to) disable multicast loopback
	C4NetIOSimpleUDP::SetMCLoopback(false);

	// set flags
	fMultiCast = true;
	iOPacketCounter = 0;
	iBroadcastRate = 0;

	// ok
	return true;
}

bool C4NetIOUDP::Close()
{
	// should be initialized
	if (!fInit) return false;

	// close all peers
	CStdShareLock PeerListLock(&PeerListCSec);
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		pPeer->Close("owner class closed");
	PeerListLock.Clear();

	// deactivate multicast
	if (fMultiCast)
		CloseBroadcast();

	// close UDP
	bool fSuccess = C4NetIOSimpleUDP::Close();

#ifdef C4NETIO_DEBUG
	// close log
	CloseDebugLog();
#endif

	// ok
	fInit = false;
	return fSuccess;
}

bool C4NetIOUDP::CloseBroadcast()
{
	ResetError();

	// multicast not active?
	if (!fMultiCast) return true;

	// ok
	fMultiCast = false;
	return C4NetIOSimpleUDP::CloseBroadcast();
}

bool C4NetIOUDP::Execute(int iMaxTime, pollfd *) // (mt-safe)
{
	if (!fInit) { SetError("not yet initialized"); return false; }

	CStdLock ExecuteLock(&ExecuteCSec);
	CStdShareLock PeerListLock(&PeerListCSec);

	ResetError();

	// adjust maximum block time
	C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
	uint32_t iMaxBlock = std::max(tNow, GetNextTick(tNow)) - tNow;
	if (iMaxTime == TO_INF || iMaxTime > (int) iMaxBlock) iMaxTime = iMaxBlock;

	// execute subclass
	if (!C4NetIOSimpleUDP::Execute(iMaxTime))
		return false;

	// connection check needed?
	if (tNextCheck <= C4TimeMilliseconds::Now())
		DoCheck();
	// client timeout?
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (!pPeer->Closed())
			pPeer->CheckTimeout();

	// do a delayed loopback test once the incoming buffer is empty
	if (fDelayedLoopbackTest)
	{
		if (fMultiCast)
			fMultiCast = DoLoopbackTest();
		fDelayedLoopbackTest = false;
	}

	// ok
	return true;
}

bool C4NetIOUDP::Connect(const addr_t &addr) // (mt-safe)
{
	// connect
	return !! ConnectPeer(addr, true);
}

bool C4NetIOUDP::Close(const addr_t &addr) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// find peer
	Peer *pPeer = GetPeer(addr);
	if (!pPeer) return false;
	// close
	pPeer->Close("closed");
	return true;
}

bool C4NetIOUDP::Send(const C4NetIOPacket &rPacket) // (mt-safe)
{
	// find Peer class for given address
	CStdShareLock PeerListLock(&PeerListCSec);
	Peer *pPeer = GetPeer(rPacket.getAddr());
	// not found?
	if (!pPeer) return false;
	// send the packet
	return pPeer->Send(rPacket);
}

bool C4NetIOUDP::Broadcast(const C4NetIOPacket &rPacket) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// search: any client reachable via multicast?
	Peer *pPeer;
	for (pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->Open() && pPeer->MultiCast() && pPeer->doBroadcast())
			break;
	bool fSuccess = true;
	if (pPeer)
	{
		CStdLock OutLock(&OutCSec);
		// send it via multicast: encapsulate packet
		Packet *pPkt = new Packet(rPacket.Duplicate(), iOPacketCounter);
		iOPacketCounter += pPkt->FragmentCnt();
		// add to list
		OPackets.AddPacket(pPkt);
		// send it
		fSuccess &= BroadcastDirect(*pPkt);
	}
	// send to all clients connected via du, too
	for (pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->Open() && !pPeer->MultiCast() && pPeer->doBroadcast())
			pPeer->Send(rPacket);
	return true;
}

bool C4NetIOUDP::SetBroadcast(const addr_t &addr, bool fSet) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// find peer
	Peer *pPeer = GetPeer(addr);
	if (!pPeer) return false;
	// set flag
	pPeer->SetBroadcast(fSet);
	return true;
}

C4TimeMilliseconds C4NetIOUDP::GetNextTick(C4TimeMilliseconds tNow) // (mt-safe)
{
	// maximum time: check interval
	C4TimeMilliseconds tTiming = tNextCheck.IsInfinite() ? tNow : std::max(tNow, tNextCheck);
	
	// client timeouts (e.g. connection timeout)
	CStdShareLock PeerListLock(&PeerListCSec);
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (!pPeer->Closed())
			if (!pPeer->GetTimeout().IsInfinite())
				tTiming = std::min(tTiming, pPeer->GetTimeout());
	// return timing value
	return tTiming;
}

bool C4NetIOUDP::GetStatistic(int *pBroadcastRate) // (mt-safe)
{
	CStdLock StatLock(&StatCSec);
	if (pBroadcastRate) *pBroadcastRate = iBroadcastRate;
	return true;
}

bool C4NetIOUDP::GetConnStatistic(const addr_t &addr, int *pIRate, int *pORate, int *pLoss) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// find peer
	Peer *pPeer = GetPeer(addr);
	if (!pPeer || !pPeer->Open()) return false;
	// return statistics
	if (pIRate) *pIRate = pPeer->GetIRate();
	if (pORate) *pORate = pPeer->GetORate();
	if (pLoss) *pLoss = 0;
	return true;
}

void C4NetIOUDP::ClearStatistic()
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// clear all peer statistics
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		pPeer->ClearStatistics();
	// broadcast statistics
	CStdLock StatLock(&StatCSec);
	iBroadcastRate = 0;
}

void C4NetIOUDP::OnPacket(const C4NetIOPacket &Packet, C4NetIO *pNetIO)
{
	assert(pNetIO == this);
#ifdef C4NETIO_DEBUG
	// log it
	DebugLogPkt(false, Packet);
#endif
	// save packet?
	if (fSavePacket)
	{
		LastPacket.Copy(Packet);
		return;
	}
	// looped back?
	if (fMultiCast && !fDelayedLoopbackTest)
		if (Packet.getAddr() == MCLoopbackAddr)
			return;
	// loopback test packet? ignore
	if ((Packet.getStatus() & 0x7F) == IPID_Test) return;
	// address add? process directly

	// find out who's responsible
	Peer *pPeer = GetPeer(Packet.getAddr());
	// new connection?
	if (!pPeer)
	{
		// ping? answer without creating a connection
		if ((Packet.getStatus() & 0x7F) == IPID_Ping)
		{
			PacketHdr PingPacket = { uint8_t(IPID_Ping | (Packet.getStatus() & 0x80)), 0 };
			SendDirect(C4NetIOPacket(&PingPacket, sizeof(PingPacket), false, Packet.getAddr()));
			return;
		}
		// conn? create connection (du only!)
		else if (Packet.getStatus() == IPID_Conn)
		{
			pPeer = ConnectPeer(Packet.getAddr(), false);
			if (!pPeer) return;
		}
		// ignore all other packets
	}
	else
	{
		// address add?
		if (Packet.getStatus() == IPID_AddAddr)
			{ OnAddAddress(Packet.getAddr(), *getBufPtr<AddAddrPacket>(Packet)); return; }

		// forward to Peer object
		pPeer->OnRecv(Packet);
	}
}

bool C4NetIOUDP::OnConn(const addr_t &AddrPeer, const addr_t &AddrConnect, const addr_t *pOwnAddr, C4NetIO *pNetIO)
{
	// ignore
	return true;
}

void C4NetIOUDP::OnDisconn(const addr_t &AddrPeer, C4NetIO *pNetIO, const char *szReason)
{
	assert(pNetIO == this);

	// C4NetIOSimple thinks the given address is no-good and we shouldn't consider
	// any connection to this address valid.

	// So let's check wether we have some peer there
	Peer *pPeer = GetPeer(AddrPeer);
	if (!pPeer) return;

	// And close him (this will issue another callback)
	pPeer->Close(szReason);
}

void C4NetIOUDP::OnAddAddress(const addr_t &FromAddr, const AddAddrPacket &Packet)
{
	// Security (this would be strange behavior indeed...)
	if (FromAddr != Packet.Addr && FromAddr != Packet.NewAddr) return;
	// Search peer(s)
	Peer *pPeer = GetPeer(Packet.Addr);
	Peer *pPeer2 = GetPeer(Packet.NewAddr);
	// Equal or not found? Nothing to do...
	if (!pPeer || pPeer == pPeer2) return;
	// Save alternate address
	pPeer->SetAltAddr(Packet.NewAddr);
	// Close superflous connection
	// (this will generate a close-packet, which will be ignored by the peer)
	pPeer2->Close("address equivalence detected");
}

// * C4NetIOUDP::Packet

// construction / destruction

C4NetIOUDP::Packet::Packet()
		: iNr(~0),
		Data()
{

}

C4NetIOUDP::Packet::Packet(C4NetIOPacket &&rnData, nr_t inNr)
		: iNr(inNr),
		Data(rnData),
		pFragmentGot(nullptr)
{

}

C4NetIOUDP::Packet::~Packet()
{
	delete [] pFragmentGot; pFragmentGot = nullptr;
}

// implementation

const size_t C4NetIOUDP::Packet::MaxSize = 512;
const size_t C4NetIOUDP::Packet::MaxDataSize = MaxSize - sizeof(DataPacketHdr);

C4NetIOUDP::Packet::nr_t C4NetIOUDP::Packet::FragmentCnt() const
{
	return Data.getSize() ? (Data.getSize() - 1) / MaxDataSize + 1 : 1;
}

C4NetIOPacket C4NetIOUDP::Packet::GetFragment(nr_t iFNr, bool fBroadcastFlag) const
{
	assert(iFNr < FragmentCnt());
	// create buffer
	uint16_t iFragmentSize = FragmentSize(iFNr);
	StdBuf Packet; Packet.New(sizeof(DataPacketHdr) + iFragmentSize);
	// set up header
	DataPacketHdr *pnHdr = getMBufPtr<DataPacketHdr>(Packet);
	pnHdr->StatusByte = IPID_Data | (fBroadcastFlag ? 0x80 : 0x00);
	pnHdr->Nr = iNr + iFNr;
	pnHdr->FNr = iNr;
	pnHdr->Size = Data.getSize();
	// copy data
	Packet.Write(Data.getPart(iFNr * MaxDataSize, iFragmentSize),
	             sizeof(DataPacketHdr));
	// return
	return C4NetIOPacket(Packet, Data.getAddr());
}

bool C4NetIOUDP::Packet::Complete() const
{
	if (Empty()) return false;
	for (unsigned int i = 0; i < FragmentCnt(); i++)
		if (!FragmentPresent(i))
			return false;
	return true;
}

bool C4NetIOUDP::Packet::FragmentPresent(uint32_t iFNr) const
{
	return !Empty() && iFNr < FragmentCnt() && (!pFragmentGot || pFragmentGot[iFNr]);
}

bool C4NetIOUDP::Packet::AddFragment(const C4NetIOPacket &Packet, const C4NetIO::addr_t &addr)
{
	// ensure the packet is big enough
	if (Packet.getSize() < sizeof(DataPacketHdr)) return false;
	size_t iPacketDataSize = Packet.getSize() - sizeof(DataPacketHdr);
	// get header
	const DataPacketHdr *pHdr = getBufPtr<DataPacketHdr>(Packet);
	// first fragment got?
	bool fFirstFragment = Empty();
	if (fFirstFragment)
	{
		// init
		iNr = pHdr->FNr;
		Data.New(pHdr->Size); Data.SetAddr(addr);
		// fragmented? create fragment list
		if (FragmentCnt() > 1)
			memset(pFragmentGot = new bool [FragmentCnt()], false, FragmentCnt());
		// check header
		if (pHdr->Nr < iNr || pHdr->Nr >= iNr + FragmentCnt()) { Data.Clear(); return false; }
	}
	else
	{
		// check header
		if (pHdr->FNr != iNr) return false;
		if (pHdr->Size != Data.getSize()) return false;
		if (pHdr->Nr < iNr || pHdr->Nr >= iNr + FragmentCnt()) return false;
	}
	// check packet size
	nr_t iFNr = pHdr->Nr - iNr;
	if (iPacketDataSize != FragmentSize(iFNr)) return false;
	// already got this fragment? (needs check for first packet as FragmentPresent always assumes true if pFragmentGot is nullptr)
	StdBuf PacketData = Packet.getPart(sizeof(DataPacketHdr), iPacketDataSize);
	if (!fFirstFragment && FragmentPresent(iFNr))
	{
		// compare
		if (Data.Compare(PacketData, iFNr * MaxDataSize))
			return false;
	}
	else
	{
		// otherwise: copy data
		Data.Write(PacketData, iFNr * MaxDataSize);
		// set flag (if fragmented)
		if (pFragmentGot)
			pFragmentGot[iFNr] = true;
		// shouldn't happen
		else
			assert(Complete());
	}
	// ok
	return true;
}

size_t C4NetIOUDP::Packet::FragmentSize(nr_t iFNr) const
{
	assert(iFNr < FragmentCnt());
	return std::min(MaxDataSize, Data.getSize() - iFNr * MaxDataSize);
}

// * C4NetIOUDP::PacketList

// construction / destruction

C4NetIOUDP::PacketList::PacketList(unsigned int inMaxPacketCnt)
		: iMaxPacketCnt(inMaxPacketCnt)
{

}

C4NetIOUDP::PacketList::~PacketList()
{
	Clear();
}

C4NetIOUDP::Packet *C4NetIOUDP::PacketList::GetPacket(unsigned int iNr)
{
	CStdShareLock ListLock(&ListCSec);
	for (Packet *pPkt = pBack; pPkt; pPkt = pPkt->Prev)
		if (pPkt->GetNr() == iNr)
			return pPkt;
		else if (pPkt->GetNr() < iNr)
			return nullptr;
	return nullptr;
}

C4NetIOUDP::Packet *C4NetIOUDP::PacketList::GetPacketFrgm(unsigned int iNr)
{
	CStdShareLock ListLock(&ListCSec);
	for (Packet *pPkt = pBack; pPkt; pPkt = pPkt->Prev)
		if (pPkt->GetNr() <= iNr && pPkt->GetNr() + pPkt->FragmentCnt() > iNr)
			return pPkt;
		else if (pPkt->GetNr() < iNr)
			return nullptr;
	return nullptr;
}

C4NetIOUDP::Packet *C4NetIOUDP::PacketList::GetFirstPacketComplete()
{
	CStdShareLock ListLock(&ListCSec);
	return pFront && pFront->Complete() ? pFront : nullptr;
}

bool C4NetIOUDP::PacketList::FragmentPresent(unsigned int iNr)
{
	CStdShareLock ListLock(&ListCSec);
	Packet *pPkt = GetPacketFrgm(iNr);
	return pPkt ? pPkt->FragmentPresent(iNr - pPkt->GetNr()) : false;
}

bool C4NetIOUDP::PacketList::AddPacket(Packet *pPacket)
{
	CStdLock ListLock(&ListCSec);
	// find insert location
	Packet *pInsertAfter = pBack, *pInsertBefore = nullptr;
	for (; pInsertAfter; pInsertBefore = pInsertAfter, pInsertAfter = pInsertAfter->Prev)
		if (pInsertAfter->GetNr() + pInsertAfter->FragmentCnt() <= pPacket->GetNr())
			break;
	// check: enough space?
	if (pInsertBefore && pInsertBefore->GetNr() < pPacket->GetNr() + pPacket->FragmentCnt())
		return false;
	// insert
	(pInsertAfter ? pInsertAfter->Next : pFront) = pPacket;
	(pInsertBefore ? pInsertBefore->Prev : pBack) = pPacket;
	pPacket->Next = pInsertBefore;
	pPacket->Prev = pInsertAfter;
	// count packets, check limit
	++iPacketCnt;
	while (iPacketCnt > iMaxPacketCnt)
		DeletePacket(pFront);
	// ok
	return true;
}

bool C4NetIOUDP::PacketList::DeletePacket(Packet *pPacket)
{
	CStdLock ListLock(&ListCSec);
#ifdef _DEBUG
	// check: this list?
	Packet *pPos = pPacket;
	while (pPos && pPos != pFront) pPos = pPos->Prev;
	assert(pPos);
#endif
	// unlink packet
	(pPacket->Prev ? pPacket->Prev->Next : pFront) = pPacket->Next;
	(pPacket->Next ? pPacket->Next->Prev : pBack) = pPacket->Prev;
	// delete packet
	delete pPacket;
	// decrease count
	--iPacketCnt;
	// ok
	return true;
}

void C4NetIOUDP::PacketList::ClearPackets(unsigned int iUntil)
{
	CStdLock ListLock(&ListCSec);
	while (pFront && pFront->GetNr() < iUntil)
		DeletePacket(pFront);
}

void C4NetIOUDP::PacketList::Clear()
{
	CStdLock ListLock(&ListCSec);
	while (iPacketCnt)
		DeletePacket(pFront);
}

// * C4NetIOUDP::Peer

// constants

const unsigned int C4NetIOUDP::Peer::iConnectRetries = 5;
const unsigned int C4NetIOUDP::Peer::iReCheckInterval = 1000; // (ms)

// construction / destruction

C4NetIOUDP::Peer::Peer(const addr_t &naddr, C4NetIOUDP *pnParent)
		: pParent(pnParent), addr(naddr),
		eStatus(CS_None),
		fMultiCast(false), fDoBroadcast(false),
		OPackets(iMaxOPacketBacklog),
		iOPacketCounter(0),
		iIPacketCounter(0), iRIPacketCounter(0),
		iIMCPacketCounter(0), iRIMCPacketCounter(0),
		iMCAckPacketCounter(0),
		tNextReCheck(C4TimeMilliseconds::NegativeInfinity),
		iIRate(0), iORate(0), iLoss(0)
{
}

C4NetIOUDP::Peer::~Peer()
{
	// send close-packet
	Close("deleted");
}

bool C4NetIOUDP::Peer::Connect(bool fFailCallback) // (mt-safe)
{
	// initiate connection (DoConn will set status CS_Conn)
	fMultiCast = false; fConnFailCallback = fFailCallback;
	return DoConn(false);
}

bool C4NetIOUDP::Peer::Send(const C4NetIOPacket &rPacket) // (mt-safe)
{
	CStdLock OutLock(&OutCSec);
	// encapsulate packet
	Packet *pnPacket = new Packet(rPacket.Duplicate(), iOPacketCounter);
	iOPacketCounter += pnPacket->FragmentCnt();
	pnPacket->GetData().SetAddr(addr);
	// add it to outgoing packet stack
	if (!OPackets.AddPacket(pnPacket))
		return false;
	// This should be ensured by calling function anyway.
	// It is not secure to send packets before the connection
	// is etablished completly.
	if (eStatus != CS_Works) return true;
	// send it
	if (!SendDirect(*pnPacket)) {
		Close("failed to send packet");
		return false;
	}
	return true;
}

bool C4NetIOUDP::Peer::Check(bool fForceCheck)
{
	// only on working connections
	if (eStatus != CS_Works) return true;
	// prevent re-check (check floods)
	// instead, ask for other packets that are missing until recheck is allowed
	bool fNoReCheck = tNextReCheck > C4TimeMilliseconds::Now();
	if (!fNoReCheck) iLastPacketAsked = iLastMCPacketAsked = 0;
	unsigned int iStartAt = fNoReCheck ? std::max(iLastPacketAsked + 1, iIPacketCounter) : iIPacketCounter;
	unsigned int iStartAtMC = fNoReCheck ? std::max(iLastMCPacketAsked + 1, iIMCPacketCounter) : iIMCPacketCounter;
	// check if we have something to ask for
	const unsigned int iMaxAskCnt = 10;
	unsigned int i, iAskList[iMaxAskCnt], iAskCnt = 0, iMCAskCnt = 0;
	for (i = iStartAt; i < iRIPacketCounter; i++)
		if (!IPackets.FragmentPresent(i))
			if (iAskCnt < iMaxAskCnt)
				iLastPacketAsked = iAskList[iAskCnt++] = i;
	for (i = iStartAtMC; i < iRIMCPacketCounter; i++)
		if (!IMCPackets.FragmentPresent(i))
			if (iAskCnt + iMCAskCnt < iMaxAskCnt)
				iLastMCPacketAsked = iAskList[iAskCnt + iMCAskCnt++] = i;
	int iEAskCnt = iAskCnt + iMCAskCnt;
	// no re-check limit? set it
	if (!fNoReCheck)
	{
		if (iEAskCnt)
			tNextReCheck = C4TimeMilliseconds::Now() + iReCheckInterval;
		else
			tNextReCheck = C4TimeMilliseconds::NegativeInfinity;
	}
	// something to ask for? (or check forced?)
	if (iEAskCnt || fForceCheck)
		return DoCheck(iAskCnt, iMCAskCnt, iAskList);
	return true;
}

void C4NetIOUDP::Peer::OnRecv(const C4NetIOPacket &rPacket) // (mt-safe)
{
	// statistics
	{ CStdLock StatLock(&StatCSec); iIRate += rPacket.getSize() + iUDPHeaderSize; }
	// get packet header
	if (rPacket.getSize()  < sizeof(PacketHdr)) return;
	const PacketHdr *pHdr = getBufPtr<PacketHdr>(rPacket);
	bool fBroadcasted = !!(pHdr->StatusByte & 0x80);
	// save packet nr
	(fBroadcasted ? iRIMCPacketCounter : iRIPacketCounter) = std::max<unsigned int>((fBroadcasted ? iRIMCPacketCounter : iRIPacketCounter), pHdr->Nr);
#ifdef C4NETIOUDP_OPT_RECV_CHECK_IMMEDIATE
	// do check
	if (eStatus == CS_Works)
		Check(false);
#endif
	// what type of packet is it?
	switch (pHdr->StatusByte & 0x7f)
	{

	case IPID_Conn:
	{
		// check size
		if (rPacket.getSize() != sizeof(ConnPacket)) break;
		const ConnPacket *pPkt = getBufPtr<ConnPacket>(rPacket);
		// right version?
		if (pPkt->ProtocolVer != pParent->iVersion) break;
		if (!fBroadcasted)
		{
			// Second connection attempt using different address?
			if (!PeerAddr.IsNull() && PeerAddr != pPkt->Addr)
			{
				// Notify peer that he has two addresses to reach this connection.
				AddAddrPacket Pkt;
				Pkt.StatusByte = IPID_AddAddr;
				Pkt.Nr = iOPacketCounter;
				Pkt.Addr = PeerAddr;
				Pkt.NewAddr = pPkt->Addr;
				SendDirect(C4NetIOPacket(&Pkt, sizeof(Pkt), false, addr));
				// But do nothing else - don't interfere with this connection
				break;
			}
			// reinit?
			else if (eStatus == CS_Works && iIPacketCounter != pPkt->Nr)
			{
				// close (callback!) ...
				OnClose("reconnect"); eStatus = CS_Closed;
				// ... and reconnect
				Connect(false);
			}
			// save back the address the peer is using
			PeerAddr = pPkt->Addr;
		}
		// set packet counter
		if (fBroadcasted)
			iRIMCPacketCounter = iIMCPacketCounter = pPkt->Nr;
		else
			iRIPacketCounter = iIPacketCounter = pPkt->Nr;
		// clear incoming packets
		IPackets.Clear();
		IMCPackets.Clear();
		
		tNextReCheck = C4TimeMilliseconds::NegativeInfinity;

		iLastPacketAsked = iLastMCPacketAsked = 0;
		// Activate Multicast?
		if (!pParent->fMultiCast)
		{
			addr_t MCAddr = pPkt->MCAddr;
			if (!MCAddr.IsNull())
			{
				// Init Broadcast (with delayed loopback test)
				pParent->fDelayedLoopbackTest = true;
				if (!pParent->InitBroadcast(&MCAddr))
					pParent->fDelayedLoopbackTest = false;
			}
		}
		// build ConnOk Packet
		ConnOKPacket nPack;
		bool fullyConnected = false;

		nPack.StatusByte = IPID_ConnOK; // (always du, no mc experiments here)
		nPack.Nr = fBroadcasted ? pParent->iOPacketCounter : iOPacketCounter;
		nPack.Addr = addr;
		if (fBroadcasted)
			nPack.MCMode = ConnOKPacket::MCM_MCOK; // multicast send ok
		else if (pParent->fMultiCast && addr.GetPort() == pParent->iPort)
			nPack.MCMode = ConnOKPacket::MCM_MC; // du ok, try multicast next
		else
		{
			nPack.MCMode = ConnOKPacket::MCM_NoMC; // du ok
			// no multicast => we're fully connected now
			fullyConnected = true;
		}
		// send it
		SendDirect(C4NetIOPacket(&nPack, sizeof(nPack), false, addr));
		// Clients will try sending data from OnConn, so send ConnOK before that.
		if (fullyConnected) OnConn();
	}
	break;

	case IPID_ConnOK:
	{
		if (eStatus != CS_Conn) break;
		// check size
		if (rPacket.getSize() != sizeof(ConnOKPacket)) break;
		const ConnOKPacket *pPkt = getBufPtr<ConnOKPacket>(rPacket);
		// save port
		PeerAddr = pPkt->Addr;
		// Needs another Conn/ConnOK-sequence?
		switch (pPkt->MCMode)
		{
		case ConnOKPacket::MCM_MC:
			// multicast has to be active
			if (pParent->fMultiCast)
			{
				// already trying to connect via multicast?
				if (fMultiCast) break;
				// Send another Conn packet back (this time broadcasted to check if multicast works)
				fMultiCast = true; DoConn(true);
				break;
			}
			// fallthru
		case ConnOKPacket::MCM_NoMC:
			// Connection is established (no multicast support)
			fMultiCast = false; OnConn();
			break;
		case ConnOKPacket::MCM_MCOK:
			// Connection is established (multicast support)
			fMultiCast = true; OnConn();
			break;
		}
	}
	break;

	case IPID_Data:
	{
		// get the packet header
		if (rPacket.getSize() < sizeof(DataPacketHdr)) return;
		const DataPacketHdr *pHdr = getBufPtr<DataPacketHdr>(rPacket);
		// already complet?
		if (pHdr->Nr < (fBroadcasted ? iIMCPacketCounter : iIPacketCounter)) break;
		// find or create packet
		bool fAddPacket = false;
		PacketList *pPacketList = fBroadcasted ? &IMCPackets : &IPackets;
		Packet *pPkt = pPacketList->GetPacket(pHdr->FNr);
		if (!pPkt) { pPkt = new Packet(); fAddPacket = true; }
		// add the fragment
		if (pPkt->AddFragment(rPacket, addr))
		{
			// add the packet to list
			if (fAddPacket) if (!pPacketList->AddPacket(pPkt)) { delete pPkt; break; }
			// check for complete packets
			CheckCompleteIPackets();
		}
		else
			// delete the packet
			if (fAddPacket) delete pPkt;
	}
	break;

	case IPID_Check:
	{
		// get the packet header
		if (rPacket.getSize() < sizeof(CheckPacketHdr)) break;
		const CheckPacketHdr *pPkt = getBufPtr<CheckPacketHdr>(rPacket);
		// check packet size
		if (rPacket.getSize() < sizeof(CheckPacketHdr) + (pPkt->AskCount + pPkt->MCAskCount) * sizeof(int)) break;
		// clear all acknowledged packets
		CStdLock OutLock(&OutCSec);
		OPackets.ClearPackets(pPkt->AckNr);
		if (pPkt->MCAckNr > iMCAckPacketCounter)
		{
			iMCAckPacketCounter = pPkt->MCAckNr;
			pParent->ClearMCPackets();
		}
		OutLock.Clear();
		// read ask list
		const int *pAskList = getBufPtr<int>(rPacket, sizeof(CheckPacketHdr));
		// send the packets he asks for
		unsigned int i;
		for (i = 0; i < pPkt->AskCount + pPkt->MCAskCount; i++)
		{
			// packet available?
			bool fMCPacket = i >= pPkt->AskCount;
			CStdLock OutLock(fMCPacket ? &pParent->OutCSec : &OutCSec);
			Packet *pPkt2Send = (fMCPacket ? pParent->OPackets : OPackets).GetPacketFrgm(pAskList[i]);
			if (!pPkt2Send) { Close("starvation"); break; }
			// send the fragment
			if (fMCPacket)
				pParent->BroadcastDirect(*pPkt2Send, pAskList[i]);
			else
				SendDirect(*pPkt2Send, pAskList[i]);
		}
	}
	break;

	case IPID_Close:
	{
		// check packet size
		if (rPacket.getSize() < sizeof(ClosePacket)) break;
		const ClosePacket *pPkt = getBufPtr<ClosePacket>(rPacket);
		// ignore if it's for another address
		if (!PeerAddr.IsNull() && PeerAddr != pPkt->Addr)
			break;
		// close
		OnClose("connection closed by peer");
	}
	break;

	}
}

void C4NetIOUDP::Peer::Close(const char *szReason) // (mt-safe)
{
	// already closed?
	if (eStatus == CS_Closed)
		return;
	// send close-packet
	ClosePacket Pkt;
	Pkt.StatusByte = IPID_Close;
	Pkt.Nr = 0;
	Pkt.Addr = addr;
	SendDirect(C4NetIOPacket(&Pkt, sizeof(Pkt), false, addr));
	// callback
	OnClose(szReason);
}

void C4NetIOUDP::Peer::CheckTimeout()
{
	// check
	if (C4TimeMilliseconds::Now() > tTimeout)
		OnTimeout();
}

void C4NetIOUDP::Peer::ClearStatistics()
{
	CStdLock StatLock(&StatCSec);
	iIRate = iORate = 0;
	iLoss = 0;
}

bool C4NetIOUDP::Peer::DoConn(bool fMC) // (mt-safe)
{
	// set status
	eStatus = CS_Conn;
	// set timeout
	SetTimeout(iStdTimeout, iConnectRetries);
	// send packet (include current outgoing packet counter and mc addr)
	ConnPacket Pkt;
	Pkt.StatusByte = uint8_t(IPID_Conn) | (fMC ?  0x80 : 0x00);
	Pkt.ProtocolVer = pParent->iVersion;
	Pkt.Nr = fMC ? pParent->iOPacketCounter : iOPacketCounter;
	Pkt.Addr = addr;
	if (pParent->fMultiCast)
		Pkt.MCAddr = pParent->C4NetIOSimpleUDP::getMCAddr();
	else
		Pkt.MCAddr = C4NetIO::addr_t();
	return SendDirect(C4NetIOPacket(&Pkt, sizeof(Pkt), false, addr));
}

bool C4NetIOUDP::Peer::DoCheck(int iAskCnt, int iMCAskCnt, unsigned int *pAskList)
{
	// security
	if (!pAskList) iAskCnt = iMCAskCnt = 0;
	// statistics
	{ CStdLock StatLock(&StatCSec); iLoss += iAskCnt + iMCAskCnt; }
	// alloc data
	int iAskListSize = (iAskCnt + iMCAskCnt) * sizeof(*pAskList);
	StdBuf Packet; Packet.New(sizeof(CheckPacketHdr) + iAskListSize);
	CheckPacketHdr *pChkPkt = getMBufPtr<CheckPacketHdr>(Packet);
	// set up header
	pChkPkt->StatusByte = IPID_Check; // (note: always du here, see C4NetIOUDP::DoCheck)
	pChkPkt->Nr = iOPacketCounter;
	pChkPkt->AckNr = iIPacketCounter;
	pChkPkt->MCAckNr = iIMCPacketCounter;
	// copy ask list
	pChkPkt->AskCount = iAskCnt;
	pChkPkt->MCAskCount = iMCAskCnt;
	if (pAskList)
		Packet.Write(pAskList, iAskListSize, sizeof(CheckPacketHdr));
	// send packet
	return SendDirect(C4NetIOPacket(Packet, addr));
}

bool C4NetIOUDP::Peer::SendDirect(const Packet &rPacket, unsigned int iNr)
{
	// send one fragment only?
	if (iNr + 1)
		return SendDirect(rPacket.GetFragment(iNr - rPacket.GetNr()));
	// otherwise: send all fragments
	bool fSuccess = true;
	for (unsigned int i = 0; i < rPacket.FragmentCnt(); i++)
		fSuccess &= SendDirect(rPacket.GetFragment(i));
	return fSuccess;
}

bool C4NetIOUDP::Peer::SendDirect(C4NetIOPacket &&rPacket) // (mt-safe)
{
	// insert correct addr
	C4NetIO::addr_t v6Addr(addr.AsIPv6());
	if (!(rPacket.getStatus() & 0x80)) rPacket.SetAddr(v6Addr);
	// count outgoing
	{ CStdLock StatLock(&StatCSec); iORate += rPacket.getSize() + iUDPHeaderSize; }
	// forward call
	return pParent->SendDirect(std::move(rPacket));
}

void C4NetIOUDP::Peer::OnConn()
{
	// reset timeout
	SetTimeout(TO_INF);
	// set status
	eStatus = CS_Works;
	// do callback
	C4NetIO::CBClass *pCB = pParent->pCB;
	if (pCB && !pCB->OnConn(addr, addr, &PeerAddr, pParent))
	{
		Close("closed");
		return;
	}
	// do packet callback (in case the peer sent data while the connection was in progress)
	CheckCompleteIPackets();
}

void C4NetIOUDP::Peer::OnClose(const char *szReason) // (mt-safe)
{
	// do callback
	C4NetIO::CBClass *pCB = pParent->pCB;
	if (eStatus == CS_Works || (eStatus == CS_Conn && fConnFailCallback))
		if (pCB)
			pCB->OnDisconn(addr, pParent, szReason);
	// set status (this will schedule this peer for deletion)
	eStatus = CS_Closed;
}

void C4NetIOUDP::Peer::CheckCompleteIPackets()
{
	// only status CS_Works
	if (eStatus != CS_Works) return;
	// (If the status is CS_Conn, we'll have to wait until the connection in the
	//  opposite direction is etablished. There is no problem in checking for
	//  complete packets here, but the one using the interface may get very confused
	//  if he gets a callback for a connection that hasn't been announced to him
	//  yet)

	// check for complete incoming packets
	Packet *pPkt;
	while ((pPkt = IPackets.GetFirstPacketComplete()))
	{
		// missing packet?
		if (pPkt->GetNr() != iIPacketCounter) break;
		// do callback
		if (pParent->pCB)
			pParent->pCB->OnPacket(pPkt->GetData(), pParent);
		// advance packet counter
		iIPacketCounter = pPkt->GetNr() + pPkt->FragmentCnt();
		// remove packet from queue
		int iNr = pPkt->GetNr();
		IPackets.DeletePacket(pPkt);
		assert(!IPackets.GetPacketFrgm(iNr)); (void)iNr;
	}
	while ((pPkt = IMCPackets.GetFirstPacketComplete()))
	{
		// missing packet?
		if (pPkt->GetNr() != iIMCPacketCounter) break;
		// do callback
		if (pParent->pCB)
			pParent->pCB->OnPacket(pPkt->GetData(), pParent);
		// advance packet counter
		iIMCPacketCounter = pPkt->GetNr() + pPkt->FragmentCnt();
		// remove packet from queue
		int iNr = pPkt->GetNr();
		IMCPackets.DeletePacket(pPkt);
		assert(!IMCPackets.GetPacketFrgm(iNr)); (void)iNr;
	}
}

void C4NetIOUDP::Peer::SetTimeout(int iLength, int iRetryCnt) // (mt-safe)
{
	if (iLength != TO_INF)
	{
		tTimeout = C4TimeMilliseconds::Now() + iLength;
	}
	else
	{
		tTimeout = C4TimeMilliseconds::PositiveInfinity;
	}
	iRetries = iRetryCnt;
}

void C4NetIOUDP::Peer::OnTimeout()
{
	// what state?
	if (eStatus == CS_Conn)
	{
		// retries left?
		if (iRetries)
		{
			int iRetryCnt = iRetries - 1;
			// call DoConn (will set timeout)
			DoConn(fMultiCast);
			// set retry count
			iRetries = iRetryCnt;
			return;
		}
		// connection timeout: close
		Close("connection timeout");
	}
	// reset timeout
	SetTimeout(TO_INF);
}

// * C4NetIOUDP: implementation

bool C4NetIOUDP::BroadcastDirect(const Packet &rPacket, unsigned int iNr) // (mt-safe)
{
	// only one fragment?
	if (iNr + 1)
		return SendDirect(rPacket.GetFragment(iNr - rPacket.GetNr(), true));
	// send all fragments
	bool fSuccess = true;
	for (unsigned int iFrgm = 0; iFrgm < rPacket.FragmentCnt(); iFrgm++)
		fSuccess &= SendDirect(rPacket.GetFragment(iFrgm, true));
	return fSuccess;
}

bool C4NetIOUDP::SendDirect(C4NetIOPacket &&rPacket) // (mt-safe)
{
	addr_t toaddr = rPacket.getAddr();
	// packet meant to be broadcasted?
	if (rPacket.getStatus() & 0x80)
	{
		// set addr
		toaddr = C4NetIOSimpleUDP::getMCAddr();
		// statistics
		CStdLock StatLock(&StatCSec);
		iBroadcastRate += rPacket.getSize() + iUDPHeaderSize;
	}

	// debug
#ifdef C4NETIO_DEBUG
	{ C4NetIOPacket Pkt2 = rPacket; Pkt2.SetAddr(toaddr); DebugLogPkt(true, Pkt2); }
#endif

#ifdef C4NETIO_SIMULATE_PACKETLOSS
	if ((rPacket.getStatus() & 0x7F) != IPID_Test)
		if (UnsyncedRandom(100) < C4NETIO_SIMULATE_PACKETLOSS) return true;
#endif

	// send it
	return C4NetIOSimpleUDP::Send(C4NetIOPacket(rPacket.getRef(), toaddr));
}

bool C4NetIOUDP::DoLoopbackTest()
{
	// (try to) enable loopback
	C4NetIOSimpleUDP::SetMCLoopback(true);
	// ensure loopback is activate
	if (!C4NetIOSimpleUDP::getMCLoopback()) return false;

	// send test packet
	const PacketHdr TestPacket = { uint8_t(IPID_Test | 0x80), UnsyncedRandom(UINT32_MAX) };
	if (!C4NetIOSimpleUDP::Broadcast(C4NetIOPacket(&TestPacket, sizeof(TestPacket))))
		return false;

	// wait for socket to become readable (should happen immediatly, do not expect packet loss)
	fSavePacket = true;
	if (!C4NetIOSimpleUDP::Execute(iStdTimeout))
	{
		fSavePacket = false;
		if (!GetError()) SetError("Multicast disabled: loopback test failed");
		return false;
	}
	fSavePacket = false;

	// compare it to the packet that was sent
	if (LastPacket.getSize() != sizeof(TestPacket) ||
	    LastPacket.Compare(&TestPacket, sizeof(TestPacket)))
	{
		SetError("Multicast disabled: loopback test failed");
		return false;
	}

	// save the loopback addr back
	MCLoopbackAddr = LastPacket.getAddr();

	// disable loopback
	C4NetIOSimpleUDP::SetMCLoopback(false);
	// ok
	return true;
}

void C4NetIOUDP::ClearMCPackets()
{
	CStdShareLock PeerListLock(&PeerListCSec);
	CStdLock OutLock(&OutCSec);
	// clear packets if no client is present
	if (!pPeerList)
		OPackets.Clear();
	else
	{
		// find minimum acknowledged packet number
		unsigned int iAckNr = pPeerList->GetMCAckPacketCounter();
		for (Peer *pPeer = pPeerList->Next; pPeer; pPeer = pPeer->Next)
			iAckNr = std::min(iAckNr, pPeerList->GetMCAckPacketCounter());
		// clear packets
		OPackets.ClearPackets(iAckNr);
	}
}

void C4NetIOUDP::AddPeer(Peer *pPeer)
{
	// get locks
	CStdShareLock PeerListLock(&PeerListCSec);
	CStdLock PeerListAddLock(&PeerListAddCSec);
	// add
	pPeer->Next = pPeerList;
	pPeerList = pPeer;
	Changed();
}

void C4NetIOUDP::OnShareFree(CStdCSecEx *pCSec)
{
	if (pCSec == &PeerListCSec)
	{
		Peer *pPeer = pPeerList, *pLast = nullptr;
		while (pPeer)
		{
			// delete?
			if (pPeer->Closed())
			{
				// unlink
				Peer *pDelete = pPeer;
				(pLast ? pLast->Next : pPeerList) = pPeer = pPeer->Next;
				// delete
				delete pDelete;
			}
			else
			{
				// next peer
				pLast = pPeer;
				pPeer = pPeer->Next;
			}
		}
	}
}

C4NetIOUDP::Peer *C4NetIOUDP::GetPeer(const addr_t &addr)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (!pPeer->Closed())
			if (pPeer->GetAddr() == addr || pPeer->GetAltAddr() == addr)
				return pPeer;
	return nullptr;
}

C4NetIOUDP::Peer *C4NetIOUDP::ConnectPeer(const addr_t &PeerAddr, bool fFailCallback) // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// lock so no new peer can be added after this point
	CStdLock PeerListAddLock(&PeerListAddCSec);
	// recheck: address already known?
	Peer *pnPeer = GetPeer(PeerAddr);
	if (pnPeer) return pnPeer;
	// create new Peer class
	pnPeer = new Peer(PeerAddr, this);
	if (!pnPeer) return nullptr;
	// add peer to list
	AddPeer(pnPeer);
	PeerListAddLock.Clear();
	// send connection request
	if (!pnPeer->Connect(fFailCallback)) { pnPeer->Close("connect failed"); return nullptr; }
	// ok (do not wait for peer)
	return pnPeer;
}

void C4NetIOUDP::DoCheck() // (mt-safe)
{
	CStdShareLock PeerListLock(&PeerListCSec);
	// mc connection check?
	if (fMultiCast)
	{
		// only if a peer is connected via multicast
		Peer *pPeer;
		for (pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
			if (pPeer->Open() && pPeer->MultiCast())
				break;
		if (pPeer)
		{
			// set up packet
			CheckPacketHdr Pkt;
			Pkt.StatusByte = uint8_t(IPID_Check | 0x80);
			Pkt.Nr = iOPacketCounter;
			Pkt.AskCount = Pkt.MCAskCount = 0;
			// send it
			SendDirect(C4NetIOPacket(&Pkt, sizeof(Pkt)));
		}
	}
	// peer connection checks
	for (Peer *pPeer = pPeerList; pPeer; pPeer = pPeer->Next)
		if (pPeer->Open())
			pPeer->Check();

	// set time for next check
	tNextCheck = C4TimeMilliseconds::Now() + iCheckInterval;
}

// debug
#ifdef C4NETIO_DEBUG
#ifndef _WIN32
#define _O_SEQUENTIAL 0
#define _O_TEXT 0
#endif
void C4NetIOUDP::OpenDebugLog()
{
	const char *szFileBase = "NetIOUDP%d.log";
	char szFilePath[_MAX_PATH + 1];
	for (int i = 0; i < 1000; i++)
	{
		sprintf(szFilePath, szFileBase, i);
		hDebugLog = open(szFilePath, O_CREAT | O_EXCL | O_TRUNC | _O_SEQUENTIAL | _O_TEXT | O_WRONLY, S_IREAD | S_IWRITE);
		if (hDebugLog != -1) break;
	}
	// initial timestamp
	if (hDebugLog != -1)
	{
		char O[1024+1];
		time_t tTime; time(&tTime);
		struct tm *pLocalTime;
		pLocalTime=localtime(&tTime);
		if (pLocalTime)
			sprintf(O, "C4NetIOUDP debuglog starting at %d/%d/%d  %d:%2d:%2d - (Daylight %d)\n",
			        pLocalTime->tm_mon+1,
			        pLocalTime->tm_mday,
			        pLocalTime->tm_year+1900,
			        pLocalTime->tm_hour,
			        pLocalTime->tm_min,
			        pLocalTime->tm_sec,
			        pLocalTime->tm_isdst);
		else sprintf(O, "C4NetIOUDP debuglog; time not available\n");
		write(hDebugLog, O, strlen(O));
	}
}

void C4NetIOUDP::CloseDebugLog()
{
	close(hDebugLog);
}

void C4NetIOUDP::DebugLogPkt(bool fOut, const C4NetIOPacket &Pkt)
{
	StdStrBuf O;
	O.Format("%s %s %s:", fOut ? "out" : "in ",
	         C4TimeMilliseconds::Now().AsString().getData(),
			 Pkt.getAddr().ToString().getData());

	// header?
	if (Pkt.getSize() >= sizeof(PacketHdr))
	{
		const PacketHdr &Hdr = *getBufPtr<PacketHdr>(Pkt);

		switch (Hdr.StatusByte & 0x07f)
		{
		case IPID_Ping:   O.Append(" PING"); break;
		case IPID_Test:   O.Append(" TEST"); break;
		case IPID_Conn:   O.Append(" CONN"); break;
		case IPID_ConnOK: O.Append(" CONO"); break;
		case IPID_Data:   O.Append(" DATA"); break;
		case IPID_Check:  O.Append(" CHCK"); break;
		case IPID_Close:  O.Append(" CLSE"); break;
		default:          O.Append(" UNKN"); break;
		}
		O.AppendFormat(" %s %04d", (Hdr.StatusByte & 0x80) ? "MC" : "DU", Hdr.Nr);

#define UPACK(type) \
    const type &P = *getBufPtr<type>(Pkt);

		switch (Hdr.StatusByte)
		{
		case IPID_Test:   { UPACK(TestPacket); O.AppendFormat(" (%d)", P.TestNr); break; }
		case IPID_Conn:   { UPACK(ConnPacket); O.AppendFormat(" (Ver %d, MC: %s)", P.ProtocolVer, P.MCAddr.ToString().getData()); break; }
		case IPID_ConnOK:
		{
			UPACK(ConnOKPacket);
			switch (P.MCMode)
			{
			case ConnOKPacket::MCM_NoMC:  O.Append(" (NoMC)"); break;
			case ConnOKPacket::MCM_MC:    O.Append(" (MC)"); break;
			case ConnOKPacket::MCM_MCOK:  O.Append(" (MCOK)"); break;
			default:                      O.Append(" (??""?)");
			}
			break;
		}
		case IPID_Data:
		{
			UPACK(DataPacketHdr); O.AppendFormat(" (f: %d s: %d)", P.FNr, P.Size);
			for (int iPos = sizeof(DataPacketHdr); iPos < std::min<int>(Pkt.getSize(), sizeof(DataPacketHdr) + 16); iPos++)
				O.AppendFormat(" %02x", *getBufPtr<unsigned char>(Pkt, iPos));
			break;
		}
		case IPID_Check:
		{
			UPACK(CheckPacketHdr);
			O.AppendFormat(" (ack: %d, mcack: %d, ask: %d mcask: %d, ", P.AckNr, P.MCAckNr, P.AskCount, P.MCAskCount);
			if (Pkt.getSize() < sizeof(CheckPacketHdr) + sizeof(unsigned int) * (P.AskCount + P.MCAskCount))
				O.AppendFormat("too small)");
			else
			{
				O.Append("[");
				for (unsigned int i = 0; i < P.AskCount + P.MCAskCount; i++)
					O.AppendFormat("%s%d", i ? ", " : "", *getBufPtr<unsigned int>(Pkt, sizeof(CheckPacketHdr) + i * sizeof(unsigned int)));
				O.Append("])");
			}
			break;
		}
		}
	}
	O.AppendFormat(" (%d bytes)\n", Pkt.getSize());
	write(hDebugLog, O.getData(), O.getLength());
}
#endif

// *** C4NetIOMan

C4NetIOMan::C4NetIOMan()
		: StdSchedulerThread()

{
}

C4NetIOMan::~C4NetIOMan()
{
	Clear();
}

void C4NetIOMan::Clear()
{
	delete[] ppNetIO; ppNetIO = nullptr;
	iNetIOCnt = iNetIOCapacity = 0;
	StdSchedulerThread::Clear();
}

void C4NetIOMan::AddIO(C4NetIO *pNetIO, bool fSetCallback)
{
	// Set callback
	if (fSetCallback)
		pNetIO->SetCallback(this);
	// Add to i/o list
	if (iNetIOCnt + 1 > iNetIOCapacity)
		EnlargeIO(1);
	ppNetIO[iNetIOCnt++] = pNetIO;
	// Register with scheduler
	Add(pNetIO);
}

void C4NetIOMan::RemoveIO(C4NetIO *pNetIO)
{
	// Search
	int i;
	for (i = 0; i < iNetIOCnt; i++)
		if (ppNetIO[i] == pNetIO)
			break;
	// Not found?
	if (i >= iNetIOCnt) return;
	// Remove
	for (i++; i < iNetIOCnt; i++)
		ppNetIO[i-1] = ppNetIO[i];
	iNetIOCnt--;
}

void C4NetIOMan::OnError(StdSchedulerProc *pProc)
{
	for (int i = 0; i < iNetIOCnt; i++)
		if (pProc == ppNetIO[i])
			OnError(ppNetIO[i]->GetError(), ppNetIO[i]);
}

void C4NetIOMan::EnlargeIO(int iBy)
{
	iNetIOCapacity += iBy;
	// Realloc
	C4NetIO **ppnNetIO = new C4NetIO *[iNetIOCapacity];
	// Set data
	for (int i = 0; i < iNetIOCnt; i++)
		ppnNetIO[i] = ppNetIO[i];
	delete[] ppNetIO;
	ppNetIO = ppnNetIO;
}

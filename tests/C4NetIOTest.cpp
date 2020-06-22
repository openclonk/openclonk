/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2017, The OpenClonk Team and contributors
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

#include <C4Include.h>
#include "network/C4NetIO.h"

#include <gtest/gtest.h>

class C4NetIOTest : public ::testing::Test
{
protected:
	C4NetIOTest()
	{
#ifdef HAVE_WINSOCK
		AcquireWinSock();
#endif
	}

	~C4NetIOTest()
	{
#ifdef HAVE_WINSOCK
		ReleaseWinSock();
#endif
	}
};

TEST_F(C4NetIOTest, HostAddressCategories)
{
	static struct TestAddr
	{
		const char *addr;
		bool null, multicast, loopback, linklocal, priv;
	} addrs[] =
	{
		//                  null   mc     loopb  ll     priv
		{"0.0.0.0",         true,  false, false, false, false},
		{"192.168.0.1",     false, false, false, false, true},
		{"10.168.0.1",      false, false, false, false, true},
		{"172.16.10.1",     false, false, false, false, true},
		{"127.0.0.1",       false, false, true,  false, false},
		{"239.1.1.1",       false, true,  false, false, false},

		//                           null   mc     loopb  ll     priv
		{"::",                       true,  false, false, false, false},
		{"::1",                      false, false, true,  false, false},
		{"ff02::1",                  false, true,  false, false, false},
		{"ff15::1234",               false, true,  false, false, false},
		{"fe80::1234:abcd:def:1234", false, false, false, true,  false},
		{"fe80::1234:abcd:def:1234", false, false, false, true,  false},
		{"fd12::1234:abcd:def:1234", false, false, false, false, true},
		{"fc12::1234:abcd:def:1234", false, false, false, false, true},

		{nullptr, false, false, false, false, false},
	};

	for (auto t = addrs; t->addr; t++)
	{
		// TODO: While this produces better error messages than EXPECT_EQ, failures are still super confusing.
		auto check = [&](bool a, bool b)
		{
			if (a == b)
				return ::testing::AssertionSuccess();
			else
				return ::testing::AssertionFailure() << "addr = " << t->addr << " expected: " << b;
		};
		C4NetIO::HostAddress addr(StdStrBuf(t->addr));
		EXPECT_TRUE(check(addr.IsNull(), t->null));
		EXPECT_TRUE(check(addr.IsMulticast(), t->multicast));
		EXPECT_TRUE(check(addr.IsLoopback(), t->loopback));
		EXPECT_TRUE(check(addr.IsLocal(), t->linklocal));
		EXPECT_TRUE(check(addr.IsPrivate(), t->priv));
	}
}


// Tests C4NetIOTCP::Bind
TEST_F(C4NetIOTest, TCPBind)
{
	if (getenv("SKIP_IPV6_TEST")) {
		printf("Skipping C4NetIOTest.TCPBind...\n");
		return;
	}

	C4NetIOTCP NetIO;
	ASSERT_TRUE(NetIO.Init());

	C4NetIO::addr_t addr(StdStrBuf("[::1]:0"));
	auto socket = NetIO.Bind(addr);
	ASSERT_NE(socket, nullptr);
	auto bound_addr = socket->GetAddress();
	EXPECT_TRUE(bound_addr.IsLoopback());
	EXPECT_EQ(bound_addr.GetFamily(), C4NetIO::addr_t::IPv6);
	EXPECT_NE(bound_addr.GetPort(), 0);

	NetIO.Close();
}

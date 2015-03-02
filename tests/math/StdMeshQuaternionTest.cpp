/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2015, The OpenClonk Team and contributors
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

// Tests StdMeshQuaternion

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "C4Include.h"
#include "lib/StdMeshMath.h"

const float pi = 3.14159265359f;

void PrintTo(const StdMeshQuaternion &quat, ::std::ostream *os)
{
	*os << "{ " << quat.x << ", " << quat.y << ", " << quat.z << ", w=" << quat.w << " }";
}

auto QuatEq = [](float w, float x, float y, float z) {
	return ::testing::AllOf(
		::testing::Field(&StdMeshQuaternion::w, ::testing::FloatEq(w)),
		::testing::Field(&StdMeshQuaternion::x, ::testing::FloatEq(x)),
		::testing::Field(&StdMeshQuaternion::y, ::testing::FloatEq(y)),
		::testing::Field(&StdMeshQuaternion::z, ::testing::FloatEq(z))
		);
};

TEST(StdMeshQuaternion, InitializerTest)
{
	const StdMeshQuaternion z = StdMeshQuaternion::Zero();
	EXPECT_THAT(z, QuatEq(0.0f, 0.0f, 0.0f, 0.0f));

	const StdMeshQuaternion aa1 = StdMeshQuaternion::AngleAxis(0.5f, StdMeshVector::Translate(1.0f, 2.0f, 3.0f));
	EXPECT_THAT(aa1, QuatEq(0.968912422f, 0.247403964f, 0.494807929f, 0.742211878f));

	const StdMeshQuaternion aa2 = StdMeshQuaternion::AngleAxis(pi, StdMeshVector::Translate(1.0f, 2.0f, 3.0f));
	EXPECT_THAT(aa2, QuatEq(-4.371139e-08f, 1.0f, 2.0f, 3.0f));
}

TEST(StdMeshQuaternion, MathTest)
{
	const StdMeshQuaternion z = StdMeshQuaternion::Zero();
	EXPECT_FLOAT_EQ(0.0f, z.LenSqr());

	const StdMeshQuaternion q1{ 1.0f, 1.0f, 1.0f, 1.0f };
	EXPECT_FLOAT_EQ(4.0f, q1.LenSqr());

	StdMeshQuaternion q2{ 1.0f, 1.0f, 1.0f, 1.0f };
	q2.Normalize();
	EXPECT_THAT(q2, QuatEq(0.5f, 0.5f, 0.5f, 0.5f));

	StdMeshQuaternion q3 = StdMeshQuaternion::Nlerp(z, q1, 0.5f);
	EXPECT_THAT(q3, QuatEq(0.5f, 0.5f, 0.5f, 0.5f));

	StdMeshQuaternion q4{ 1.0f, 2.0f, 3.0f, 4.0f };
	EXPECT_FLOAT_EQ(30.0f, q4.LenSqr());

	StdMeshQuaternion q5 = StdMeshQuaternion::Nlerp(z, q4, 0.4f);
	EXPECT_FLOAT_EQ(1.0f, q5.LenSqr());
	EXPECT_THAT(q5, QuatEq(0.1825741858f, 0.365148372f, 0.547722558f, 0.730296743f));

	StdMeshQuaternion q6{ 1.0f, -3.0f, -4.0f, -5.0f };
	StdMeshQuaternion q7 = StdMeshQuaternion::Nlerp(q6, q1, 0.25f);
	// lerp: w=1.0, x=-2.0, y=-2.75, z=-3.5, len=4.981214711
	EXPECT_FLOAT_EQ(1.0f, q7.LenSqr());
	EXPECT_THAT(q7, QuatEq(0.0869565234f, -0.434782594f, -0.565217376f, -0.695652187f));

	// Any quaternion added to ZERO stays the same
	EXPECT_THAT(z + q1, QuatEq(1.0f, 1.0f, 1.0f, 1.0f));
	EXPECT_THAT(q4 + z, QuatEq(1.0f, 2.0f, 3.0f, 4.0f));

	EXPECT_THAT(q4 + q1, QuatEq(2.0f, 3.0f, 4.0f, 5.0f));
	EXPECT_THAT(q6 + q4, QuatEq(2.0f, -1.0f, -1.0f, -1.0f));

	EXPECT_THAT(q6 - q1, QuatEq(0.0f, -4.0f, -5.0f, -6.0f));
	EXPECT_THAT(q1 - q4, QuatEq(0.0f, -1.0f, -2.0f, -3.0f));

	EXPECT_THAT(4.0f * q4, QuatEq(4.0f, 8.0f, 12.0f, 16.0f));
	EXPECT_THAT(q4 * 4.0f, QuatEq(4.0f, 8.0f, 12.0f, 16.0f));

	// Any quaternion multiplied by ZERO becomes ZERO
	EXPECT_THAT(z * q1, QuatEq(0.0f, 0.0f, 0.0f, 0.0f));
	EXPECT_THAT(q3 * z, QuatEq(0.0f, 0.0f, 0.0f, 0.0f));

	// Any quaternion multiplied by IDENTITY stays the same
	StdMeshQuaternion id = StdMeshQuaternion{ 1.0f, 0.0f, 0.0f, 0.0f };
	EXPECT_THAT(q6 * id, QuatEq(1.0f, -3.0f, -4.0f, -5.0f));
	EXPECT_THAT(id * q6, QuatEq(1.0f, -3.0f, -4.0f, -5.0f));

	EXPECT_THAT(q1 * q6, QuatEq(13.0f, -3.0f, -1.0f, -5.0f));
	EXPECT_THAT(q6 * q1, QuatEq(13.0f, -1.0f, -5.0f, -3.0f));

	EXPECT_THAT(q1 * q2, QuatEq(-1.0f, 1.0f, 1.0f, 1.0f));
}

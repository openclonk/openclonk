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

// Tests StdMeshMath classes

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "C4Include.h"
#include "lib/StdMeshMath.h"

void PrintTo(const StdMeshVector &vec, ::std::ostream *os)
{
	*os << "{ " << vec.x << ", " << vec.y << ", " << vec.z << " }";
}

auto VectorEq = [](float x, float y, float z) {
	return ::testing::AllOf(
		::testing::Field(&StdMeshVector::x, ::testing::FloatEq(x)),
		::testing::Field(&StdMeshVector::y, ::testing::FloatEq(y)),
		::testing::Field(&StdMeshVector::z, ::testing::FloatEq(z))
		);
};

TEST(StdMeshVector, InitializerTest)
{
	StdMeshVector z = StdMeshVector::Zero();
	EXPECT_THAT(z, VectorEq(0.0f, 0.0f, 0.0f));

	StdMeshVector id = StdMeshVector::UnitScale();
	EXPECT_THAT(id, VectorEq(1.0f, 1.0f, 1.0f));

	StdMeshVector xl = StdMeshVector::Translate(4.0f, 29.0f, 15.0f);
	EXPECT_THAT(xl, VectorEq(4.0f, 29.0f, 15.0f));
}

TEST(StdMeshVector, VecOpVecTest)
{
	const StdMeshVector v1 = StdMeshVector::Translate(1.0f, 3.0f, 7.0f);
	const StdMeshVector v2 = StdMeshVector::Translate(2.0f, 4.0f, 8.0f);

	EXPECT_THAT(v1 + v2, VectorEq(3.0f, 7.0f, 15.0f));
	EXPECT_THAT(v2 + v1, VectorEq(3.0f, 7.0f, 15.0f));
	
	EXPECT_THAT(v1 * v2, VectorEq(2.0f, 12.0f, 56.0f));
	EXPECT_THAT(v2 * v1, VectorEq(2.0f, 12.0f, 56.0f));

	EXPECT_THAT(v1 / v2, VectorEq(0.5f, 0.75f, 0.875f));
	EXPECT_THAT(v2 / v1, VectorEq(2.0f, 1.33333333f, 1.14285719f));
}

TEST(StdMeshVector, VecOpFloatTest)
{
	const StdMeshVector v1 = StdMeshVector::Translate(1.0f, 3.0f, 7.0f);
	const StdMeshVector v2 = StdMeshVector::Translate(2.0f, 4.0f, 8.0f);

	EXPECT_THAT(v1 * 2.0f, VectorEq(2.0f, 6.0f, 14.0f));
	EXPECT_THAT(3.0f * v2, VectorEq(6.0f, 12.0f, 24.0f));
	EXPECT_THAT(v2 * 0.5f, VectorEq(1.0f, 2.0f, 4.0f));

	EXPECT_THAT(v2 / 2.0f, VectorEq(1.0f, 2.0f, 4.0f));
	EXPECT_THAT(2.0f / v2, VectorEq(1.0f, 0.5f, 0.25f));
	EXPECT_THAT(v1 / v1, VectorEq(1.0f, 1.0f, 1.0f));
	
	EXPECT_THAT(v1 * 0.5f, VectorEq(0.5f, 1.5f, 3.5f));
	EXPECT_THAT(0.5f * v1, VectorEq(0.5f, 1.5f, 3.5f));

	EXPECT_THAT(v2 / 8.0f, VectorEq(0.25f, 0.5f, 1.0f));
	EXPECT_THAT(8.0f / v2, VectorEq(4.0f, 2.0f, 1.0f));

	StdMeshVector v3 = v2;
	v3 *= 2.0f;
	EXPECT_THAT(v3, VectorEq(4.0f, 8.0f, 16.0f));

	StdMeshVector v4 = v2;
	v4 += v1;
	EXPECT_THAT(v4, VectorEq(3.0f, 7.0f, 15.0f));

	StdMeshVector c12 = StdMeshVector::Cross(v1, v2);
	EXPECT_THAT(c12, VectorEq(-4.0f, 6.0f, -2.0f));
	StdMeshVector c21 = StdMeshVector::Cross(v2, v1);
	EXPECT_THAT(c21, VectorEq(4.0f, -6.0f, 2.0f));

	StdMeshVector c11 = StdMeshVector::Cross(v1, v1);
	EXPECT_THAT(c11, VectorEq(0.0f, 0.0f, 0.0f));

	EXPECT_THAT(-v1, VectorEq(-1.0f, -3.0f, -7.0f));
}

/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

/* Issue #973: {Find, Sort}_* with no origin should use the FindObject call's
 * context object instead of 0, 0 */

func Initialize()
{
	// Setup testing environment: one object for context, a second object at the context object, one object at 0, 0
	var ctx = CreateObjectAbove(Dummy, 100, 100, NO_OWNER);
	ctx->SetName("ctx");
	var at_origin = CreateObjectAbove(Dummy, 0, 0, NO_OWNER);
	at_origin->SetName("at_origin");
	var at_object = CreateObjectAbove(Dummy, ctx->GetX(), ctx->GetY(), NO_OWNER);
	at_object->SetName("at_object");

	TEST("object context Find_Distance");
	EXPECT_EQ(at_object, ctx->FindObject(Find_Exclude(ctx), Find_Distance(1)));

	TEST("global context Find_Distance");
	EXPECT_EQ(at_origin,      FindObject(Find_Exclude(ctx), Find_Distance(1, at_origin->GetX(), at_origin->GetY())));
	EXPECT_EQ(at_object,      FindObject(Find_Exclude(ctx), Find_Distance(1, at_object->GetX(), at_object->GetY())));

	TEST("object context Find_InRect");
	EXPECT_EQ(at_object, ctx->FindObject(Find_Exclude(ctx), Find_InRect(0, 0, 1, 1)));
	EXPECT_EQ(at_origin, ctx->FindObject(Find_Exclude(ctx), Find_InRect(-ctx->GetX(), -ctx->GetY(), 1, 1)));

	TEST("global context Find_InRect");
	EXPECT_EQ(at_origin,      FindObject(Find_Exclude(ctx), Find_InRect(at_origin->GetX(), at_origin->GetY(), 1, 1)));
	EXPECT_EQ(at_object,      FindObject(Find_Exclude(ctx), Find_InRect(at_object->GetX(), at_object->GetY(), 1, 1)));

	TEST("object context Sort_Distance");
	EXPECT_EQ(at_object, ctx->FindObject(Find_Exclude(ctx),              Sort_Distance() ));
	EXPECT_EQ(at_origin, ctx->FindObject(Find_Exclude(ctx), Sort_Reverse(Sort_Distance())));

	TEST("global context Sort_Distance");
	EXPECT_EQ(at_object,      FindObject(Find_Exclude(ctx),              Sort_Distance(at_object->GetX(), at_object->GetY()) ));
	EXPECT_EQ(at_origin,      FindObject(Find_Exclude(ctx),              Sort_Distance(at_origin->GetX(), at_origin->GetY()) ));
	EXPECT_EQ(at_object,      FindObject(Find_Exclude(ctx), Sort_Reverse(Sort_Distance(at_origin->GetX(), at_origin->GetY()))));

	END_TEST();
	GameOver();
}

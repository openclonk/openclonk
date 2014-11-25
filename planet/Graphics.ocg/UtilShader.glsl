
// #ifdef NO_TEXTURE_LOD_IN_FRAGMENT
#define texture1DLod(t,c,l) texture1D(t,c)
#define texture2DLod(t,c,l) texture2D(t,c)
// #endif

vec3 extend_normal(vec2 v)
{
	return normalize(vec3(v, 0.3));
}

// Converts the pixel range 0.0..1.0 into the integer range 0..255
int f2i(float x) {
	return int(x * 255.9);
}


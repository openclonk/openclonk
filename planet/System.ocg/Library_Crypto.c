/**
 * Experimental cryptographic function library.
 * Contains a lot less stuff than you expect. May also change at any time.
 */

// I'm choosing to implement it this way because this means we can change the
// underlying implementation without having to give too much consideration to
// existing scripts. You shouldn't use the native functions in _Crypto directly
// because we might change them if necessary; the current implementation of 
// engine-provided libraries is VERY experimental and might not survive in its
// current form.
static const Crypto = new _Crypto
{
	/**
	 * Calculates a cryptographically secure hash over [data]. Will return an
	 * encoded string safe for embedding in a script without further escaping.
	 */
	ComputeHash = func(string data) { return _ComputeHash(data, 25); }
};

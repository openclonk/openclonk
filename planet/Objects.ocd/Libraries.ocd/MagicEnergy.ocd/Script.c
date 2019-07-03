/**
	Library_MagicEnergy

	Contains the logic for an object that has magic energy.
*/

/* Magic */

local magic_energy;

public func GetMagicEnergy(int precision)
{
	if (precision == nil) precision = 1000;

	if (precision)
		return magic_energy / precision;
	else
		return magic_energy;
}

public func GetMaxMagicEnergy(int precision)
{
	if (precision == nil) precision = 1000;

	if (precision)
		return this.MaxMagic / precision;
	else
		return this.MaxMagic;
}

public func SetMagicEnergy(int val, int precision)
{
	if (precision == nil) precision = 1000;

	magic_energy = BoundBy(val * precision, 0, this.MaxMagic);
	this->~OnMagicEnergyChange(val);

	return true;
}

// Adjusts the magic energy but only if change can be applied completely. Returns true if successful, false otherwise.
// Use partial to bypass the completeness check
public func DoMagicEnergy(int change, bool partial, int precision)
{
	if (precision == nil) precision = 1000;
	change = change * precision;

	// Can't apply fully?
	if (!Inside(magic_energy + change, 0, this.MaxMagic) && !partial)
		return false;

	magic_energy = BoundBy(magic_energy + change, 0, this.MaxMagic);
	this->~OnMagicEnergyChange(change);
	return true;
}

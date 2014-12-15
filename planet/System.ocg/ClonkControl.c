/*--
	Global functions used in correlation to Objects.ocd\Libraries.ocd\ClonkControl.ocd
	
	Authors: Newton
--*/

// disable ShiftContents for objects with ClonkControl.ocd

global func ShiftContents(...)
{
	if (this)
		if (this->~HandObjects() != nil)
			return false;
	return _inherited(...);
}

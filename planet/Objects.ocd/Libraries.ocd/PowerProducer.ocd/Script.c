/**
	Power producer
	Should be included by all power generators.
*/

func Destruction()
{
	MakePowerProducer(0);
	return _inherited(...);
}

// Method to hang a wipf on a balloon.

#appendto Balloon

public func HangWipfOnBalloon(object wipf)
{
	// Create the balloon and set its speed and rider.
	var balloon = CreateObjectAbove(BalloonDeployed, 0, 5);
	balloon->SetRider(wipf);
	balloon->SetParent(this);

	// Sound.
	Sound("Objects::Balloon::Inflate");

	// Make the wipf ride the balloon.
	wipf->SetAction("Hang", balloon);
	return true;
}
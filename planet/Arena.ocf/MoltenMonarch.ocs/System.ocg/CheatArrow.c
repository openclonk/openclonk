#appendto Arrow

// Arrows do not tumble

public func HitObject(object obj)
{
	// Determine damage to obj from speed and arrow strength.
	var relx = GetXDir() - obj->GetXDir();
	var rely = GetYDir() - obj->GetYDir();
	var speed = Sqrt(relx * relx + rely * rely);
	var dmg = ArrowStrength() * speed / 100;
	ProjectileHit(obj, dmg, 0);
	// Stick does something unwanted to controller.
	if (this) 
		Stick();
	return;
}
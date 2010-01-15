/*-- Musket --*/

#strict 2
#include PROJ

public func BarrelLength() { return 25; }
public func MuzzleVelocity() { return 300; }
public func MagazineSize() { return 1; }
public func ProjectileType() { return MBLL; }
public func Accuracy() { return 3; }
public func MuzzleFlashID() { return FLSH; }
public func FiringSound() { return "Blast3"; }
public func WeaponReloadAction() { return "Walk"; } 
public func WeaponReloadSound() { return "MusketReload"; }

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
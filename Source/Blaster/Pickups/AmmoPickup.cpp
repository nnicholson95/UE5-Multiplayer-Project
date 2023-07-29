#include "AmmoPickup.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

/*
* Override function triggered when the character overlapps with the collision sphere of the pickup
*/
void AAmmoPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool BFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, BFromSweep, SweepResult);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		UCombatComponent* Combat = BlasterCharacter->GetCombat();
		if (Combat)
		{
			//Change carried ammo for the weapon type on this character
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();
}

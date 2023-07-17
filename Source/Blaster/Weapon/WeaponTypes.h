#pragma once
/*
* Since Weapon types are so integral to a shooter game it makes sense to keep them in a central location
* Weapon.h may seem like an attractive place but I put them here because including weapon.h in all places
* necessary JUST to get the enum is unnecessary
*/

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "AssaultRifle"),

	EWT_MAX UMETA(DisplayName = "DefaultMax")
};
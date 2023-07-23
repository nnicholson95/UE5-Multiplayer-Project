#pragma once
/*
* Since Weapon types are so integral to a shooter game it makes sense to keep them in a central location
* Weapon.h may seem like an attractive place but I put them here because including weapon.h in all places
* necessary JUST to get the enum is unnecessary
*/

#define TRACE_LENGTH 80000

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "Submachine Gun"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayName = "Sniper Rifle"),

	EWT_MAX UMETA(DisplayName = "DefaultMax")
};
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

/*
* Struct to fill in the location rotation and boxextent of a given hitbox
*/
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

/*
* This struct maps hitbox information structs to hitboxes and the time the information was recorded.
* 
* This will be useful to use for server side rewind as it establishes a lightweight way to trace hits
* against hitboxes.
*/
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	//Map Hitbox name to its information struct
	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadshot;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class ABlasterCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor Color);
	FServerSideRewindResult ServerSideRewind(class ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime);
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, class AWeapon* DamageCauser);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);
	void CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisonEnabled);
	void SaveFramePackage();

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;

public:	
		
};

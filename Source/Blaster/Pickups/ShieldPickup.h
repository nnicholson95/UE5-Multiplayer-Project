#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "ShieldPickup.generated.h"

UCLASS()
class BLASTER_API AShieldPickup : public APickup
{
	GENERATED_BODY()
public:

protected:
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool BFromSweep,
		const FHitResult& SweepResult
	) override;

private:
	UPROPERTY(EditAnywhere)
	float ShieldReplenishAmount = 200.f;

	UPROPERTY(EditAnywhere)
	float ShieldReplenishTime = 5.f;

	UPROPERTY(EditAnywhere)
	float BuffTime = 10.f;

	UPROPERTY(EditAnywhere)
	float BuffMaxShield = 200.f;

	UPROPERTY()
	class ABlasterCharacter* Character;
	
};

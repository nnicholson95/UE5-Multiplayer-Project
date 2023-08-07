#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Flag.generated.h"

UCLASS()
class BLASTER_API AFlag : public AWeapon
{
	GENERATED_BODY()
public:
	AFlag();
	virtual void Dropped() override;
	void ResetFlag();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void OnEquipped() override;
	virtual void OnDropped() override;
	virtual void BeginPlay() override;
private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* FlagMesh;

	FTransform InitialTransform;

	UPROPERTY(EditAnywhere)
	float KillZThreshold = -5000.f;
public:
	FORCEINLINE FTransform GetInitialTransform() const { return InitialTransform; }
};
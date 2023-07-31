#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	friend class ABlasterCharacter;
	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float ShieldAmount, float ReplenishTime, float BuffTime, bool bBuffShield, float BuffMaxShield);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffJump(float BuffJumpVelocity, float BuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);
	void SetInitialMaxShield(float MaxShield);

protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	/*
	* Heal buff
	*/

	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	/*
	* Shield buff
	*/

	FTimerHandle ShieldDamageCooldownTimer;
	void ResetMaxShield();

	bool bReplenishingShield = false;
	bool bShieldIsBuffed = false;
	float InitialMaxShield;
	float ShieldReplenishRate = 5.f;
	float ShieldReplenishAmount = 100.f;

	/*
	* Speed Buff
	*/

	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();
	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/*
	* Jump buff
	*/

	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};

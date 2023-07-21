#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//returns the pawn that owns the weapon that fired the rocket
	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, //World context object
				Damage, //Base Damage
				10.f, //Minimum Damage
				GetActorLocation(), //Origin
				200.f, //DamageInnerRadius TODO: make blueprint exposed variable
				500.f, //DamageOuterRadius TODO: make blueprint exposed variable
				1.f, //DamageFalloff
				UDamageType::StaticClass(), //DamageTypeClass
				TArray<AActor*>(), //Actors to ignore
				this, //Damage Causer -- The rocket in this case
				FiringController //Instigator Controller
			);
		}
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

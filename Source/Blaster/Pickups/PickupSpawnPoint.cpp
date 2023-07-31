#include "PickupSpawnPoint.h"
#include "Pickup.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	SpawnPickup();
}

void APickupSpawnPoint::SpawnPickup()
{
	if (PickupClass)
	{
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(PickupClass, GetActorTransform());

		//We want to maintain server authority for pickup spawns
		if (HasAuthority() && SpawnedPickup)
		{
			//Bind this to AActor's OnDestroyed delegate -- This way it starts the spawn timer when the pickup is destroyed
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = SpawnPickupTime;
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinished,
		SpawnTime
	);
}

void APickupSpawnPoint::SpawnPickupTimerFinished()
{
	if (HasAuthority())
	{
		SpawnPickup();
	}
}
void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


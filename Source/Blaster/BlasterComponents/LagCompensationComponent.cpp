#include "LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

/*
* Interpolation gets a current and target value
*
* The idea is to first take the distance between the two values
* Using this distance move the value by DeltaMove, = Distance * FMath::Clamp(DeltaTime * InterpSpeed, 0, 1)
* The clamp is a fraction value between 0 and 1 determined by deltatime * interpspeed to give us a frame independent movement
* Since distance gets smaller the movement is smooth
* 
* @param OlderFrame - The frame with the older time we are interpolating between
* @param YoungerFram - The frame with the younger time we are interpolating betwee
* @param HitTime - The target time 
*/
FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	//Values for interpolation
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	//Loop over every HitBoxInfo
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		//Get the name of the pair this iteration
		const FName& BoxInfoName = YoungerPair.Key;

		//Get the references to the olderbox and younger hitbox with the above name using our map in the frame package
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		//Local box info
		FBoxInformation InterpBoxInfo;

		//Fill in the local box with interpolated values between the older and younger box
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		//Fill out interpolated values for each hitbox
		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

/*
* The idea of this function is for the server to move the hitboxes of the hit character back to the authoritative position and then
* perform the same line trace that the aggressor performed and confirm or deny the hit
* 
* This happens in a single frame and as such has no affect on gameplay - there is obviously a calculation and thus more work the server
* has to do, especially if it is also playing the game.
*/
FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	//Temporary storage for hitbox positions 
	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// enable collision for the head shot check first
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECollisionChannel::ECC_Visibility
		);
		if (ConfirmHitResult.bBlockingHit) //Headshot, move boxes back and return
		{
			//Reset using our saved current frame
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::PhysicsOnly);
			return FServerSideRewindResult{ true, true };
		}
		else //Not a headshot, check the rest of the hitboxes
		{
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
				}
			}

			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECollisionChannel::ECC_Visibility
			);
			if (ConfirmHitResult.bBlockingHit)
			{
				//Reset using our saved current frame
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::PhysicsOnly);
				// not a headshot so second bool false
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	//Reset using our saved current frame
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::PhysicsOnly);
	// not a confirmed hit or headshot
	return FServerSideRewindResult{ false, false };
}

/*
* Shotguns can hit multiple people so we have a confirm hit for shotguns
*/
FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramPackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	return FShotgunServerSideRewindResult();
}

/*
* Helper function that caches the original hitbox position before we rewind so we can restore the position after
*/
void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

/*
* Helper function that moves the hitboxes 
*/
void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	//loop over hit character's hitboxes
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

/*
* Helper function that returns hit boxes to normal
*/
void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr) return;

	//loop over hit character's hitboxes
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

/*
* Helper to change character mesh collision
*/
void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisonEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisonEnabled);
	}
}

/*
* Debug function that draws the hitboxs of a character that persist for 4 seconds
*/
void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f
		);
	}
}

/*
* Rewinds the server side version of a shot character to ensure that hits are accurate in real world latency situations
* 
* Called by the character that did the shooting -- HitCharacter is the character that did the shooting
*/
FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

/*
* Since Shotguns can hit multiple characters we have to adjust our rewind logic accordingly
*/
FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}

	return FShotgunServerSideRewindResult();
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;

	if (bReturn) return FFramePackage();

	//Local Frame package we check to verify a hit
	FFramePackage FrameToCheck;

	//Most of the time we will need to interpolate
	bool bShouldInterpolate = true;

	// Frame history of the HitCharacter
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (OldestHistoryTime > HitTime)
	{
		// too far back - too laggy to do server side rewind
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		//We found the frame as well but it will be the tail
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime)
	{
		//This means we found the frame to check 
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	while (Older->GetValue().Time > HitTime) //Is Older still younger than HItTime?
	{
		//March back until OlderTime < HitTime < YoungerTime
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode();
		//We want to move younger to the same node as older but only if it is before hit time
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}
	//Highly unlikely, but after me moved our pointer we found out frame to check 
	if (Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		//Interpolate between younger and older
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}

	return FrameToCheck;
}

/*
* Server RPC that the client will call to request the rewind and confirm hits
*/
void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	// HitConfirmed will be true if the server says we got a hit in the rewind
	if (Character && HitCharacter && DamageCauser && Confirm.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageCauser->GetDamage(),
			Character->Controller, //We know that the character that owns this lagcompensationComponent is the one shooting
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

/*
* Helper function that saves the hitbox positions in the recent frames
*/
void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority()) return;
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		//Time of the newest frame - time of the oldest frame
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		//Remove tail until frame history is desired length
		while (HistoryLength > MaxRecordTime)
		{
			//Remove the oldest frame -- but doesn't necessarily mean that our history length < MaxRecordTime
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);

		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}

/*
* Overload of SaveFramePackage
*/
void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;

	if (Character)
	{
		//This is only on the server so we can use the server time and know its correct
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}
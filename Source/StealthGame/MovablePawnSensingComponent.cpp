// Copyright Epic Games, Inc. All Rights Reserved.

#include "MovablePawnSensingComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "EngineGlobals.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "AIController.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/ArrowComponent.h"

DECLARE_CYCLE_STAT(TEXT("Sensing"), STAT_AI_Sensing, STATGROUP_AI);

#define FARSIGHTTHRESHOLD 8000.f
#define FARSIGHTTHRESHOLDSQUARED (FARSIGHTTHRESHOLD*FARSIGHTTHRESHOLD)

#define NEARSIGHTTHRESHOLD 2000.f
#define NEARSIGHTTHRESHOLDSQUARED (NEARSIGHTTHRESHOLD * NEARSIGHTTHRESHOLD)

UMovablePawnSensingComponent::UMovablePawnSensingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LOSHearingThreshold = 2800.f;
	HearingThreshold = 1400.0f;
	SightRadius = 5000.0f;
	PeripheralVisionAngle = 90.f;
	PeripheralVisionCosine = FMath::Cos(FMath::DegreesToRadians(PeripheralVisionAngle));

	SensingInterval = 0.5f;
	HearingMaxSoundAge = 1.f;

	bOnlySensePlayers = true;
	bHearNoises = true;
	bSeePawns = true;

	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	bAutoActivate = false;

	bEnableSensingUpdates = true;

	FacingDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingDirection"));
	FacingDirection->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);


	//Move the arrow a bit ahead of the start of the viewcone, so we can use it to get a facing direction vector later.
	FacingDirection->AddRelativeLocation(FVector(100, 0, 0));

	bIsDebug = false;
}

void UMovablePawnSensingComponent::SetPeripheralVisionAngle(const float NewPeripheralVisionAngle)
{
	PeripheralVisionAngle = NewPeripheralVisionAngle;
	PeripheralVisionCosine = FMath::Cos(FMath::DegreesToRadians(PeripheralVisionAngle));
}

void UMovablePawnSensingComponent::InitializeComponent()
{
	Super::InitializeComponent();
	SetPeripheralVisionAngle(PeripheralVisionAngle);

	if (bEnableSensingUpdates)
	{
		bEnableSensingUpdates = false; // force an update
		SetSensingUpdatesEnabled(true);
	}
}

void UMovablePawnSensingComponent::SetSensingUpdatesEnabled(const bool bEnabled)
{
	if (bEnableSensingUpdates != bEnabled)
	{
		bEnableSensingUpdates = bEnabled;

		if (bEnabled && SensingInterval > 0.f)
		{
			// Stagger initial updates so all sensors do not update at the same time (to avoid hitches).
			const float InitialDelay = (SensingInterval * FMath::SRand()) + KINDA_SMALL_NUMBER;
			SetTimer(InitialDelay);
		}
		else
		{
			SetTimer(0.f);
		}
	}
}

void UMovablePawnSensingComponent::SetTimer(const float TimeInterval)
{
	// Only necessary to update if we are the server
	AActor* const Owner = GetOwner();
	if (IsValid(Owner) && GEngine->GetNetMode(GetWorld()) < NM_Client)
	{
		Owner->GetWorldTimerManager().SetTimer(TimerHandle_OnTimer, this, &UMovablePawnSensingComponent::OnTimer, TimeInterval, false);
	}
}

void UMovablePawnSensingComponent::SetSensingInterval(const float NewSensingInterval)
{
	if (SensingInterval != NewSensingInterval)
	{
		SensingInterval = NewSensingInterval;

		AActor* const Owner = GetOwner();
		if (IsValid(Owner))
		{
			if (SensingInterval <= 0.f)
			{
				SetTimer(0.f);
			}
			else if (bEnableSensingUpdates)
			{
				float CurrentElapsed = Owner->GetWorldTimerManager().GetTimerElapsed(TimerHandle_OnTimer);
				CurrentElapsed = FMath::Max(0.f, CurrentElapsed);

				if (CurrentElapsed < SensingInterval)
				{
					// Extend lifetime by remaining time.
					SetTimer(SensingInterval - CurrentElapsed);
				}
				else if (CurrentElapsed > SensingInterval)
				{
					// Basically fire next update, because time has already expired.
					// Don't want to fire immediately in case an update tries to change the interval, looping endlessly.
					SetTimer(KINDA_SMALL_NUMBER);
				}
			}
		}
	}
}

void UMovablePawnSensingComponent::OnTimer()
{

	SCOPE_CYCLE_COUNTER(STAT_AI_Sensing);

	AActor* const Owner = GetOwner();
	if (!IsValid(Owner) || !IsValid(Owner->GetWorld()))
	{
		return;
	}
	if (CanSenseAnything())
	{
		UpdateAISensing();
	}

	if (bEnableSensingUpdates)
	{
		SetTimer(SensingInterval);
	}

};

AActor* UMovablePawnSensingComponent::GetSensorActor() const
{
	AActor* SensorActor = GetOwner();
	AController* Controller = Cast<AController>(SensorActor);
	if (IsValid(Controller))
	{
		// In case the owner is a controller, use the controlled pawn as the sensing location.
		SensorActor = Controller->GetPawn();
	}

	if (!IsValid(SensorActor))
	{
		return NULL;
	}

	return SensorActor;
}

bool UMovablePawnSensingComponent::IsSensorActor(const AActor* Actor) const
{
	return (Actor == GetSensorActor());
}

bool UMovablePawnSensingComponent::HasLineOfSightTo(const AActor* Other) const
{
	if (!Other)
	{
		return false;
	}

	FCollisionQueryParams CollisionParms(SCENE_QUERY_STAT(LineOfSight), true, Other);
	CollisionParms.AddIgnoredActor(this->GetOwner());
	FVector TargetLocation = Other->GetTargetLocation(GetOwner());

	FVector ViewPoint = GetComponentLocation();

	bool bHit = GetWorld()->LineTraceTestByChannel(ViewPoint, TargetLocation, ECC_Visibility, CollisionParms);
	if (!bHit)
	{
		return true;
	}

	// if other isn't using a cylinder for collision and isn't a Pawn (which already requires an accurate cylinder for AI)
	// then don't go any further as it likely will not be tracing to the correct location
	if (!Cast<const APawn>(Other) && Cast<UCapsuleComponent>(Other->GetRootComponent()) == NULL)
	{
		return false;
	}
	float distSq = (Other->GetActorLocation() - ViewPoint).SizeSquared();
	if (distSq > FARSIGHTTHRESHOLDSQUARED)
	{
		return false;
	}
	if (!Cast<const APawn>(Other) && (distSq > NEARSIGHTTHRESHOLDSQUARED))
	{
		return false;
	}

	float OtherRadius, OtherHeight;
	Other->GetSimpleCollisionCylinder(OtherRadius, OtherHeight);

	//try viewpoint to head
	bHit = GetWorld()->LineTraceTestByChannel(ViewPoint, Other->GetActorLocation() + FVector(0.f, 0.f, OtherHeight), ECC_Visibility, CollisionParms);
	return !bHit;
}

bool UMovablePawnSensingComponent::CanSenseAnything() const
{
	return ((bHearNoises && OnHearNoise.IsBound()) ||
		(bSeePawns && OnSeePawn.IsBound()));
}

void UMovablePawnSensingComponent::UpdateAISensing()
{
	AActor* const Owner = GetOwner();

	check(IsValid(Owner));
	check(IsValid(Owner->GetWorld()));

	if (bOnlySensePlayers)
	{
		for (FConstPlayerControllerIterator Iterator = Owner->GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (IsValid(PC))
			{
				APawn* Pawn = PC->GetPawn();
				if (IsValid(Pawn) && !IsSensorActor(Pawn))
				{
					SensePawn(*Pawn);
				}
			}
		}
	}
	else
	{
		for (APawn* Pawn : TActorRange<APawn>(Owner->GetWorld()))
		{
			if (!IsSensorActor(Pawn))
			{
				SensePawn(*Pawn);
			}
		}
	}
}

void UMovablePawnSensingComponent::SensePawn(APawn& Pawn)
{
	// Visibility checks
	bool bHasSeenPawn = false;
	bool bHasFailedLineOfSightCheck = false;
	if (bSeePawns && ShouldCheckVisibilityOf(&Pawn))
	{
		if (CouldSeePawn(&Pawn, true))
		{
			if (HasLineOfSightTo(&Pawn))
			{
				bHadLoSToPawn = true;
				BroadcastOnSeePawn(Pawn);
				bHasSeenPawn = true;
				if (bIsDebug)
				{
					if (GEngine)
						GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, TEXT("Pawn has been seen!"));

				}
			}
			else
			{
				if (bIsDebug)
				{
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, TEXT("No line of sight to pawn!"));

					}
				}
				
				if (bHadLoSToPawn) 
				{
					//If we had LoS but not anymore, it means we just lost track of the pawn
					BroadcastOnUnSeePawn(Pawn);
					bHadLoSToPawn = false;


					if (bIsDebug) 
					{
						if (GEngine)
							GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Black, TEXT("Pawn has stopped being seen!"));
					}

				}
				bHasFailedLineOfSightCheck = true;
				
			}
		}
		else 
		{
			if (bIsDebug) 
			{
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("CouldSeePawn() has returned FALSE"));
			}

		}
	}
	else 
	{
		if (bIsDebug)
		{
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("SensePawn() is not checking for visibility."));
		}
	}

	// Sound checks
	// No need to 'hear' something if you've already seen it!
	if (bHasSeenPawn)
	{
		return;
	}

	// Might not be able to hear or react to the sound at all...
	if (!bHearNoises || !OnHearNoise.IsBound())
	{
		return;
	}

	const UPawnNoiseEmitterComponent* NoiseEmitterComponent = Pawn.GetPawnNoiseEmitterComponent();
	if (NoiseEmitterComponent && ShouldCheckAudibilityOf(&Pawn))
	{
		// ToDo: This should all still be refactored more significantly.  There's no reason that we should have to
		// explicitly check "local" and "remote" (i.e. Pawn-emitted and other-source-emitted) sounds separately here.
		// The noise emitter should handle all of those details for us so the sensing component doesn't need to know about
		// them at all!
		if (IsNoiseRelevant(Pawn, *NoiseEmitterComponent, true) && CanHear(Pawn.GetActorLocation(), NoiseEmitterComponent->GetLastNoiseVolume(true), bHasFailedLineOfSightCheck))
		{
			BroadcastOnHearLocalNoise(Pawn, Pawn.GetActorLocation(), NoiseEmitterComponent->GetLastNoiseVolume(true));
		}
		else if (IsNoiseRelevant(Pawn, *NoiseEmitterComponent, false) && CanHear(NoiseEmitterComponent->LastRemoteNoisePosition, NoiseEmitterComponent->GetLastNoiseVolume(false), false))
		{
			BroadcastOnHearRemoteNoise(Pawn, NoiseEmitterComponent->LastRemoteNoisePosition, NoiseEmitterComponent->GetLastNoiseVolume(false));
		}
	}
}

void UMovablePawnSensingComponent::BroadcastOnSeePawn(APawn& Pawn)
{
	OnSeePawn.Broadcast(&Pawn);
}

void UMovablePawnSensingComponent::BroadcastOnUnSeePawn(APawn& Pawn)
{
	OnUnSeePawn.Broadcast(&Pawn);
}

void UMovablePawnSensingComponent::BroadcastOnHearLocalNoise(APawn& Instigator, const FVector& Location, float Volume)
{
	OnHearNoise.Broadcast(&Instigator, Location, Volume);
}

void UMovablePawnSensingComponent::BroadcastOnHearRemoteNoise(APawn& Instigator, const FVector& Location, float Volume)
{
	OnHearNoise.Broadcast(&Instigator, Location, Volume);
}

bool UMovablePawnSensingComponent::CouldSeePawn(const APawn* Other, bool bMaySkipChecks)
{
	if (!Other)
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	
	FVector const OtherLoc = Other->GetActorLocation();
	FVector const SensorLoc = GetSensorLocation();
	FVector const SelfToOther = OtherLoc - SensorLoc;

	// check max sight distance
	float const SelfToOtherDistSquared = SelfToOther.SizeSquared();
	if (SelfToOtherDistSquared > FMath::Square(SightRadius))
	{
		return false;
	}

	// may skip if more than some fraction of maxdist away (longer time to acquire)
	if (bMaySkipChecks && (FMath::Square(FMath::FRand()) * SelfToOtherDistSquared > FMath::Square(0.4f * SightRadius)))
	{
		return false;
	}

	if(bIsDebug)
		UE_LOG(LogPath, Warning, TEXT("DistanceToOtherSquared = %f, SightRadiusSquared: %f"), SelfToOtherDistSquared, FMath::Square(SightRadius));

		// check field of view
	FVector const SelfToOtherDir = SelfToOther.GetSafeNormal();
	FVector const MyFacingDir = GetSensorRotation().GetSafeNormal();

	SensorLocationVector = SensorLoc;
	SelfToOtherDirection = OtherLoc;
	FacingDirectionVectorDebug = MyFacingDir;

	if (bIsDebug)
		UE_LOG(LogPath, Warning, TEXT("DotProductFacing: %f, PeripheralVisionCosine: %f"), SelfToOtherDir | MyFacingDir, PeripheralVisionCosine);

	return ((SelfToOtherDir | MyFacingDir) >= PeripheralVisionCosine);
}

bool UMovablePawnSensingComponent::IsNoiseRelevant(const APawn& Pawn, const UPawnNoiseEmitterComponent& NoiseEmitterComponent, bool bSourceWithinNoiseEmitter) const
{
	// If sound has no volume, it's not relevant.
	if (NoiseEmitterComponent.GetLastNoiseVolume(bSourceWithinNoiseEmitter) <= 0.f)
	{
		return false;
	}

	float LastNoiseTime = NoiseEmitterComponent.GetLastNoiseTime(bSourceWithinNoiseEmitter);
	// If the sound is too old, it's not relevant.
	if (Pawn.GetWorld()->TimeSince(LastNoiseTime) > HearingMaxSoundAge)
	{
		return false;
	}

	// If there's no sensor actor or sensor was created since the noise was emitted, it's irrelevant.
	AActor* SensorActor = GetSensorActor();
	if (!SensorActor || (LastNoiseTime < SensorActor->CreationTime))
	{
		return false;
	}

	return true;
}

FVector UMovablePawnSensingComponent::GetSensorLocation() const
{
	FVector SensorLocation(FVector::ZeroVector);
	SensorLocation = GetComponentLocation();
	return SensorLocation;
	
}

FVector UMovablePawnSensingComponent::GetSensorRotation() const
{
	FVector SensorRotation(FVector::ZeroVector);

	//If we have a facing direction, we construct the vector from it.
	if (FacingDirection != nullptr) {
		return (FacingDirection->GetComponentLocation() - GetComponentLocation());
	}

	//otherwise, we get the rotation the way the original UPawnSensingComponent did it.
	const AActor* SensorActor = GetSensorActor();

	if (SensorActor != NULL)
	{
		SensorRotation = SensorActor->GetActorRotation().Vector();
	}
	return SensorRotation;
}

bool UMovablePawnSensingComponent::CanHear(const FVector& NoiseLoc, float Loudness, bool bFailedLOS) const
{
	if (Loudness <= 0.f)
	{
		return false;
	}

	const AActor* const Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return false;
	}

	FVector const HearingLocation = GetSensorLocation();

	float const LoudnessAdjustedDistSq = (HearingLocation - NoiseLoc).SizeSquared() / (Loudness * Loudness);
	if (LoudnessAdjustedDistSq <= FMath::Square(HearingThreshold))
	{
		// Hear even occluded sounds within HearingThreshold
		return true;
	}

	// check if sound close enough to do LOS check, and LOS hasn't already failed
	if (bFailedLOS || (LoudnessAdjustedDistSq > FMath::Square(LOSHearingThreshold)))
	{
		return false;
	}

	// check if sound is occluded
	return !Owner->GetWorld()->LineTraceTestByChannel(HearingLocation, NoiseLoc, ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(CanHear), true, Owner));
}

bool UMovablePawnSensingComponent::ShouldCheckVisibilityOf(APawn* Pawn) const
{
	const bool bPawnIsPlayer = (Pawn->Controller && Pawn->Controller->PlayerState);
	if (!bSeePawns || (bOnlySensePlayers && !bPawnIsPlayer))
	{
		return false;
	}

	if (bPawnIsPlayer && AAIController::AreAIIgnoringPlayers())
	{
		return false;
	}

	if (Pawn->IsHidden())
	{
		return false;
	}

	return true;
}

bool UMovablePawnSensingComponent::ShouldCheckAudibilityOf(APawn* Pawn) const
{
	const bool bPawnIsPlayer = (Pawn->Controller && Pawn->Controller->PlayerState);
	if (!bHearNoises || (bOnlySensePlayers && !bPawnIsPlayer))
	{
		return false;
	}

	if (bPawnIsPlayer && AAIController::AreAIIgnoringPlayers())
	{
		return false;
	}

	return true;
}
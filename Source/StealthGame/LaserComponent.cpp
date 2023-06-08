// Fill out your copyright notice in the Description page of Project Settings.


#include "LaserComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

#define ECC_LineOfSight ECC_GameTraceChannel2

// Sets default values for this component's properties
ULaserComponent::ULaserComponent()
{
	LaserDistance = 500.f;
	LaserColor = FColor::Magenta;
	
	NiagaraLaser = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraLaser"));
	NiagaraLaser->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	
	NiagaraLaserImpact = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraLaserImpact"));
	NiagaraLaserImpact->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	
	bWantsOnUpdateTransform = true;
	bAutoActivate = true;
	bIsLaserTouchingPlayer = false;
	
}

// Called when the game starts
void ULaserComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void ULaserComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult result;
	FCollisionQueryParams params = FCollisionQueryParams::DefaultQueryParam;
	params.AddIgnoredActor(GetOwner()); // ignore collision with self
	//trace a line along the laser direction
	if (GetWorld()->LineTraceSingleByChannel(result, GetComponentLocation(),GetComponentLocation() + GetComponentRotation().Vector() * LaserDistance, ECC_LineOfSight, params)) 
	{
		//If we hit something, then cut the laser off at that place and draw the impact shape
		NiagaraLaser->SetVectorParameter(TEXT("Laser End"), result.Location);
		NiagaraLaserImpact->SetWorldLocation(result.Location);
		NiagaraLaserImpact->SetVisibility(true);

		//if we start touching the player, notify (but only if we werent already touching the player)

		APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

		if (result.Actor == Player && !bIsLaserTouchingPlayer)
		{
			bIsLaserTouchingPlayer = true;
			OnLaserStartInterceptPlayer.Broadcast(Player);
		}
		else if (result.Actor != Player && bIsLaserTouchingPlayer)
		{
			bIsLaserTouchingPlayer = false;
			OnLaserStopInterceptPlayer.Broadcast(Player);
		}
	}
	else 
	{
		//otherwise, set the laser end to be its max distance and dont show an impact shape
		NiagaraLaser->SetVectorParameter(TEXT("Laser End"), GetComponentLocation() + GetComponentRotation().Vector() * LaserDistance);
		NiagaraLaserImpact->SetVisibility(false);
		
		// if we were touching the player but now we touch nothing, then notify that we no longer intercept the player
		if (bIsLaserTouchingPlayer)
		{
			APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
			OnLaserStopInterceptPlayer.Broadcast(Player);
			bIsLaserTouchingPlayer = false;
		}
	}

}

#if WITH_EDITOR
void ULaserComponent::PostEditChangeProperty(FPropertyChangedEvent& e) {
	
	if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ULaserComponent, NiagaraLaserSystem)) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Setting Laser Asset"));
		NiagaraLaser->SetAsset(NiagaraLaserSystem);
	}

	if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ULaserComponent, NiagaraLaserImpactSystem)) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Setting Impact Asset"));
		NiagaraLaserImpact->SetAsset(NiagaraLaserImpactSystem);
	}
	
	if (e.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ULaserComponent, LaserColor)) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Setting Color"));
		NiagaraLaser->SetColorParameter("Color", LaserColor);
		NiagaraLaserImpact->SetColorParameter("Color", LaserColor);
	}
	Super::PostEditChangeProperty(e);
}

void ULaserComponent::OnUpdateTransform(EUpdateTransformFlags Flags, ETeleportType Teleport)
{
	if (!NiagaraLaser) return;

	const FVector rot = GetComponentLocation() + GetComponentRotation().Vector() * LaserDistance;
	NiagaraLaser->SetVectorParameter("Laser End", rot);
}

#endif
void ULaserComponent::OnRegister()
{
	Super::OnRegister();

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	if (!NiagaraLaser) return;

	const FVector rot = GetComponentLocation() + GetComponentRotation().Vector() * LaserDistance;

	//UE_LOG(LogTemp, Warning, TEXT("Updated: %s %f %f %f"), *GetName(), rot.X, rot.Y, rot.Z);
	NiagaraLaser->SetVectorParameter("Laser End", rot);
	
}

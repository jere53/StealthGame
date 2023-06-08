// Fill out your copyright notice in the Description page of Project Settings.


#include "SecurityCamera.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "MovablePawnSensingComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#define LineOfSight ECC_GameTraceChannel2

// Sets default values
ASecurityCamera::ASecurityCamera()
{
	CameraBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CameraBase"));

	Camera = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Camera"));
	Camera->AttachToComponent(CameraBase, FAttachmentTransformRules::KeepRelativeTransform);

	Viewcone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Viewcone"));
	Viewcone->AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);

	ViewCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ViewCapsule"));
	ViewCapsule->AttachToComponent(Viewcone, FAttachmentTransformRules::KeepRelativeTransform);

	AudioScanner = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioScanner"));
	AudioScanner->AttachToComponent(Viewcone, FAttachmentTransformRules::KeepRelativeTransform);

	PawnSensing = CreateDefaultSubobject<UMovablePawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);

	Spotlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLight"));

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HasTarget = false;
}


// Called when the game starts or when spawned
void ASecurityCamera::BeginPlay()
{
	Super::BeginPlay();

	//We can do this because there's only ever gonna be one player.
	Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	PawnSensing->OnSeePawn.AddDynamic(this, &ASecurityCamera::OnPawnSeen);
	PawnSensing->OnUnSeePawn.AddDynamic(this, &ASecurityCamera::OnPawnUnSeen);
}

void ASecurityCamera::OnPawnSeen(APawn* Pawn)
{
	//If we were already seeing the player, we do nothing
	if (HasTarget) return;

	//make sure we only react to seeing the player
	if (!PawnSensing->bOnlySensePlayers)
	{
		if (Pawn != Player) return;
	}
	OnPlayerSeen();
}

void ASecurityCamera::OnPawnUnSeen(APawn* Pawn) 
{

	//make sure we only react to seeing the player
	if (!PawnSensing->bOnlySensePlayers)
	{
		if (Pawn != Player) return;
	}
		OnPlayerUnSeen();
}

// Called every frame
void ASecurityCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASecurityCamera::SetAlarmState(bool bAlarmState)
{
	HasTarget = bAlarmState;
}


void ASecurityCamera::OnPlayerSeen_Implementation() 
{
	HasTarget = true;
}

void ASecurityCamera::OnPlayerUnSeen_Implementation() 
{
	HasTarget = false;
}

void ASecurityCamera::SetCameraAsAlert() 
{
	//Show that camera is "alerted" by changing sounds and visuals.
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ScannerLockOnSoundcue, Camera->GetComponentLocation());

	AudioScanner->SetSound(ScannerAlarmSoundcue);
	AudioScanner->Activate();

	Spotlight->SetLightColor(LightColorAlarmOn);

	LensMaterial->SetScalarParameterValue("Alarm State", 1);
	ViewconeMaterial->SetScalarParameterValue("Cone State", 1);
}

void ASecurityCamera::SetCameraAsNotAlert() 
{
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ScannerLockOffSoundcue, Camera->GetComponentLocation());

	AudioScanner->SetSound(ScannerResetSoundcue);
	AudioScanner->Activate();

	Spotlight->SetLightColor(LightColorAlarmOff);

	LensMaterial->SetScalarParameterValue("Alarm State", 0);
	ViewconeMaterial->SetScalarParameterValue("Cone State", 0);

}


void ASecurityCamera::NotifyAlarmObservers()
{

	for (int i = 0; i < AlarmObservers.Num(); i++) 
	{
		IAlarmInterface* AlarmObserver = Cast<IAlarmInterface>(AlarmObservers[i]);
		if (AlarmObserver) 
		{
			AlarmObserver->SetAlarmState(HasTarget);
		}
	}
}
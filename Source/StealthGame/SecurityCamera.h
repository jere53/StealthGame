// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlarmInterface.h"
#include "SecurityCamera.generated.h"

UCLASS()
class STEALTHGAME_API ASecurityCamera : public AActor, public IAlarmInterface
{

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanning", meta = (AllowPrivateAccess = "true"))
		FRotator DefaultRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		class UMaterialInstanceDynamic* ViewconeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		class UMaterialInstanceDynamic* LensMaterial;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		AActor* Player;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		AActor* AlarmInteractors;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
		bool InRange;

	//Components

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USceneCaptureComponent2D* SceneCapture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UAudioComponent* AudioScanner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* Viewcone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* CameraBase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class USpotLightComponent* Spotlight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UCapsuleComponent* ViewCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UMovablePawnSensingComponent* PawnSensing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class AController* CameraController;
	
public:	
	// Sets default values for this actor's properties
	ASecurityCamera();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanning")
		float CameraMaxYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanning")
		float CameraPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot Light")
		float LightAttenuationRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot Light")
		float SpotlightBrightness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot Light")
		FLinearColor LightColorAlarmOn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot Light")
		FLinearColor LightColorAlarmOff;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
		bool HasTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
		bool TargetLost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
		bool TestAlarmMaterialSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alarm Interface")
		TArray<AActor*> AlarmObservers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Target")
		class UTextureRenderTarget2D* SceneCaptureRender;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render Target")
		float SceneCaptureViewDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		bool MusicChange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* ScannerSoundcue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* ScannerAlarmSoundcue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* ScannerResetSoundcue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* ScannerLockOnSoundcue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
		class USoundBase* ScannerLockOffSoundcue;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION()
		void OnPawnSeen(APawn* Pawn);
	
	UFUNCTION()
		void OnPawnUnSeen(APawn* Pawn);

	UFUNCTION(BlueprintNativeEvent)
		void OnPlayerSeen();

	UFUNCTION(BlueprintNativeEvent)
		void OnPlayerUnSeen();

	UFUNCTION(BlueprintCallable)
		void SetCameraAsAlert();

	UFUNCTION(BlueprintCallable)
		void SetCameraAsNotAlert();
	
	void NotifyAlarmObservers();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void SetAlarmState(bool bAlarmState) override;
};

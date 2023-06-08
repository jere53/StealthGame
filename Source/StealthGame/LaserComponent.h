// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LaserComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STEALTHGAME_API ULaserComponent : public USceneComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLaserInterceptPlayerDelegate, APawn*, Pawn);

public:	
	// Sets default values for this component's properties
	ULaserComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& e) override;
	virtual void OnUpdateTransform(EUpdateTransformFlags Flags, ETeleportType Teleport) override;
#endif
	virtual void OnRegister() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FLinearColor LaserColor;

	//The particle systems used to define the niagara components
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class UNiagaraSystem* NiagaraLaserSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class UNiagaraSystem* NiagaraLaserImpactSystem;

	//how far does the laser reach
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float LaserDistance;

	UPROPERTY(BlueprintReadOnly)
		class UNiagaraComponent* NiagaraLaser;

	UPROPERTY(BlueprintReadOnly)
		class UNiagaraComponent* NiagaraLaserImpact;

	UPROPERTY()
	//Used to only notify player detection on the first frame that the player touches laser.
	bool bIsLaserTouchingPlayer;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable)
		FLaserInterceptPlayerDelegate OnLaserStartInterceptPlayer;

	UPROPERTY(BlueprintAssignable)
		FLaserInterceptPlayerDelegate OnLaserStopInterceptPlayer;
};

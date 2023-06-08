// Fill out your copyright notice in the Description page of Project Settings.


#include "FMPawnSensingComponentVisualizer.h"
#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "Engine/EngineTypes.h"
#include "Math/Color.h"
#include "Math/Transform.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "StealthGame/MovablePawnSensingComponent.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "ShowFlags.h"
#include "Templates/Casts.h"

void FMPawnSensingComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (!View->Family->EngineShowFlags.VisualizeSenses) return;

	// Cast the incoming component to our UConnector class
	const UMovablePawnSensingComponent* MPSComponent = Cast<const UMovablePawnSensingComponent>(Component);

	if (MPSComponent == NULL) return;

	const FTransform Transform = FTransform(MPSComponent->GetComponentRotation(), MPSComponent->GetComponentLocation());

	if (MPSComponent->LOSHearingThreshold > 0.0f) 
	{
		DrawWireSphere(PDI,Transform,FColor::Yellow, MPSComponent->LOSHearingThreshold, 16, SDPG_World);
	}

	if (MPSComponent->HearingThreshold > 0.0f)
	{
		DrawWireSphere(PDI, Transform, FColor::Cyan, MPSComponent->HearingThreshold, 16, SDPG_World);
	}

	if (MPSComponent->SightRadius > 0.0f) 
	{
		TArray<FVector> Verts;
		DrawWireCone(PDI, Verts, Transform, MPSComponent->SightRadius, MPSComponent->GetPeripheralVisionAngle(), 10, FColor::Green, SDPG_World);
	}

}

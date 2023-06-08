#include "StealthGameEditor.h"
#include "UnrealEd.h"
#include "FMPawnSensingComponentVisualizer.h"
#include "StealthGame/MovablePawnSensingComponent.h"

IMPLEMENT_GAME_MODULE(FStealthGameEditorModule, StealthGameEditor);

void FStealthGameEditorModule::StartupModule()
{

	if (GUnrealEd)
	{
		UE_LOG(LogTemp, Warning, TEXT("Another warning message"));
		// Make a new instance of the visualizer
		TSharedPtr<FComponentVisualizer> visualizer = MakeShareable(new FMPawnSensingComponentVisualizer());

		// Register it to our specific component class
		GUnrealEd->RegisterComponentVisualizer(UMovablePawnSensingComponent::StaticClass()->GetFName(), visualizer);
		visualizer->OnRegister();
	}
}

void FStealthGameEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		// Unregister when the module shuts down
		GUnrealEd->UnregisterComponentVisualizer(UMovablePawnSensingComponent::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
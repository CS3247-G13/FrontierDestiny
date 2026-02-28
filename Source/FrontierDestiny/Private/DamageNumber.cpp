#include "DamageNumber.h"
#include "Components/TextRenderComponent.h"

ADamageNumber::ADamageNumber()
{
	// We need Tick enabled to handle the scaling animation
	PrimaryActorTick.bCanEverTick = true;

	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
	RootComponent = TextRender;

	TextRender->SetText(FText::FromString("-"));
	TextRender->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TextRender->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);

	Lifetime = 1.0f;
	Progress = 0.0f;
}

void ADamageNumber::BeginPlay()
{
	Super::BeginPlay();

	// Reset progress on spawn
	Progress = 0.0f;

	TextRender->SetText(FText::AsNumber(DamageNumber));
}

void ADamageNumber::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Lifetime > 0.0f)
	{
		// Update progress based on elapsed time
		Progress += DeltaTime;

		// Calculate a value that goes from 1.0 down to 0.0
		float ScaleValue = FMath::Clamp(1.0f - (Progress / Lifetime), 0.0f, 1.0f);

		// Apply to X and Y scale
		// TextRender components use these parameters for the mesh scaling
		TextRender->SetRelativeScale3D(FVector(ScaleValue, ScaleValue, 1.0f));

		// Destroy the actor once it's no longer visible
		if (Progress >= Lifetime)
		{
			Destroy();
		}
	}
}
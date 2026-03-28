#include "DamageNumber.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ADamageNumber::ADamageNumber()
{
	PrimaryActorTick.bCanEverTick = true;

	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
	RootComponent = TextRender;

	TextRender->SetText(FText::FromString("-"));
	TextRender->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TextRender->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
}

void ADamageNumber::SetDamageNumber(int32 Value)
{
	DamageNumber = Value;
	TextRender->SetText(FText::AsNumber(DamageNumber));
}

void ADamageNumber::BeginPlay()
{
	Super::BeginPlay();

	Progress = 0.f;
}

void ADamageNumber::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Progress += DeltaTime;

	// Rise upward
	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaTime));

	// Fade out — eased so it stays visible longer then fades at the end
	const float Alpha = FMath::Clamp(1.f - (Progress / Lifetime), 0.f, 1.f);
	const float Opacity = FMath::Pow(Alpha, 0.5f);
	TextRender->SetTextRenderColor(FColor(255, 255, 255, FMath::RoundToInt(Opacity * 255.f)));

	// Face the player camera
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

		const FRotator LookAt = (CameraLocation - GetActorLocation()).Rotation();
		SetActorRotation(LookAt);
	}

	if (Progress >= Lifetime)
	{
		Destroy();
	}
}

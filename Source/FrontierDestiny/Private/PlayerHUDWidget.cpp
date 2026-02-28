// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUDWidget.h"
#include "CoreHPWidget.h"

void UPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CoreSlots = {
		Core0,
		Core1,
		Core2,
		Core3,
		Core4
	};

	for (UCoreHPWidget* Core : CoreSlots)
	{
		if (Core)
		{
			Core->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UPlayerHUDWidget::SetAmmo(int32 NewAmmo)
{
	if (AmmoText)
	{
		AmmoText->SetText(FText::AsNumber(NewAmmo));
	}
}

void UPlayerHUDWidget::InitializeCores(const TArray<FCoreData>& CoreList)
{
	if (!CoreHPContainer) return;
	CoreHPContainer->ClearChildren();

	for (const FCoreData& Core : CoreList)
	{
		UCoreHPWidget* CoreBar = CreateWidget<UCoreHPWidget>(
			GetWorld(),
			CoreHPClass
		);

		if (CoreBar)
		{
			CoreBar->InitializeCoreBar(Core);
			CoreHPContainer->AddChild(CoreBar);
		}
	}
}

void UPlayerHUDWidget::ActivateCore(int32 Index, const FCoreData& CoreData)
{
	if (!CoreSlots.IsValidIndex(Index)) return;

	UCoreHPWidget* Core = CoreSlots[Index];

	if (!Core) return;

	Core->InitializeCoreBar(CoreData);
	Core->SetVisibility(ESlateVisibility::Visible);
}

void UPlayerHUDWidget::UpdateCore(int32 Index, const FCoreData& CoreData)
{
	if (!CoreSlots.IsValidIndex(Index)) return;

	UCoreHPWidget* Core = CoreSlots[Index];

	if (Core)
	{
		Core->UpdateHP(CoreData);
	}
}

void UPlayerHUDWidget::ShowInteractText(const FString& Text)
{
	if (InteractText)
	{
		InteractText->SetText(FText::FromString(Text));
		InteractText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPlayerHUDWidget::HideInteractText()
{
	if (InteractText)
	{
		InteractText->SetVisibility(ESlateVisibility::Hidden);
	}
}
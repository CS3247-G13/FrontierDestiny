// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreHPWidget.h"

void UCoreHPWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UCoreHPWidget::InitializeCoreBar(const FCoreData& CoreData)
{
    if (HP_Bar)
    {
        HealthBarMat = HP_Bar->GetDynamicMaterial();
        HealthBarMat->SetScalarParameterValue(TEXT("HPPercent"), CoreData.GetHPPercent());
        HealthBarMat->SetVectorParameterValue(TEXT("Color"), CoreData.CoreColor);
    }

    if (HP_Text)
    {
        float Percent = CoreData.GetHPPercent() * 100.0f;
        FText HPText = FText::AsNumber(FMath::RoundToInt(Percent));
        HP_Text->SetText(
            FText::Format(
                FText::FromString("{0}%"),
                HPText
            )
        );
    }

    if (Icon && CoreData.CoreIcon)
    {
        Icon->SetBrushFromTexture(CoreData.CoreIcon);
    }
}

void UCoreHPWidget::UpdateHP(const FCoreData& CoreData)
{
    if (HP_Bar)
    {
        HealthBarMat->SetScalarParameterValue(TEXT("HPPercent"), CoreData.GetHPPercent());
    }

    if (HP_Text)
    {
        float Percent = CoreData.GetHPPercent() * 100.0f;
        FText HPText = FText::AsNumber(FMath::RoundToInt(Percent));
        HP_Text->SetText(
            FText::Format(
                FText::FromString("{0}%"),
                HPText
            )
        );
    }
}
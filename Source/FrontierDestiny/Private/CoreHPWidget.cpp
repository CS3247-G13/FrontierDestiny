// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreHPWidget.h"

void UCoreHPWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UCoreHPWidget::InitializeCoreBar(const FCoreData& CoreData)
{
    if (HPBar)
    {
        HPBar->SetFillColorAndOpacity(CoreData.CoreColor);
        HPBar->SetPercent(CoreData.GetHPPercent());
    }
}

void UCoreHPWidget::UpdateHP(const FCoreData& CoreData)
{
    if (HPBar)
    {
        HPBar->SetPercent(CoreData.GetHPPercent());
    }
}
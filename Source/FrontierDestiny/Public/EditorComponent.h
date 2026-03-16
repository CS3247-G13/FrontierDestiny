// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EditorComponent.generated.h"

// Forward declarations to keep the header clean
class ATowerActor;
class ATowerDemoPlayerController;
class UEconomySubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTowerSelected, FName, TowerID);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRONTIERDESTINY_API UEditorComponent : public UActorComponent
{
	GENERATED_BODY()
	
};
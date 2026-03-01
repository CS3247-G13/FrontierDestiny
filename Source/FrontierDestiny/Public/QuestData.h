// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "QuestData.generated.h"

UENUM(BlueprintType)
enum class EObjectiveType : uint8
{
	ReachDestination,
	Interact,
	KillEnemies,
};

USTRUCT(BlueprintType)
struct FQuestData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	TArray<FText> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	int32 CurrentObjectiveIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	AActor* Target = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	bool IsCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName NextQuestID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EObjectiveType ObjectiveType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RequiredKillCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentKillCount = 0;

	// Not Implemented
	// In enemy component, UQuestSubsystem* QuestSystem = GetGameInstance()->GetSubsystem<UQuestSubsytem>();
	// QuestSystem->RegisterEnemyKilled();
	//UFUNCTION(BlueprintCallable)
	//void RegisterEnemyKilled();
};
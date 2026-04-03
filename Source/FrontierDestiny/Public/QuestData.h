// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DataTable.h"
#include "QuestData.generated.h"

UENUM(BlueprintType)
enum class EObjectiveType : uint8
{
	ReachDestination,
	Interact,
	KillEnemies,
	Timer,
	JustMessage,
};

USTRUCT(BlueprintType)
struct FQuestMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Faction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Delay = 5.0f;
};

USTRUCT(BlueprintType)
struct FQuestData : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	TArray<FQuestMessage> Messages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
	FText Prompt;

	UPROPERTY(BlueprintReadWrite, Category="Quest")
	int32 CurrentMessageIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName TargetTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Quest")
	bool bIsCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bIsStarted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FName> NextQuestIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EObjectiveType ObjectiveType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredDistance = 1000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RequiredKillCount = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentKillCount = 0;

	UPROPERTY(BlueprintReadWrite)
	float TimeLeft = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bAutoStartNext = false;
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QuestData.h"
#include "QuestSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestUpdated);

UCLASS()
class FRONTIERDESTINY_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TMap<FName, FQuestData> AllQuests;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FQuestData CurrentQuest;

	UPROPERTY(BlueprintAssignable)
	FOnQuestUpdated OnQuestUpdated;

	UFUNCTION(BlueprintCallable)
	void StartQuest(FName QuestID);

	UFUNCTION(BlueprintCallable)
	void CompleteObjective();

	void CompleteQuest();

	UFUNCTION(BlueprintCallable)
	void RegisterEnemyKilled();

	UFUNCTION(BlueprintCallable)
	void UpdateTimeLeft(float seconds);
};

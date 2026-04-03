// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "QuestData.h"
#include "QuestSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestUpdated);

UCLASS()
class FRONTIERDESTINY_API UQuestSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual TStatId GetStatId() const override;

	virtual void Tick(float DeltaTime) override
	{
		UpdateTimeLeft(DeltaTime);
		CheckReachDestination();
	}

	virtual bool IsTickable() const override
	{
		return true;
	}
	
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
	void UpdateTimeLeft(float DeltaTime);

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TSet<FName> UnlockedQuests;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TSet<FName> CompletedQuests;

	UFUNCTION()
	AActor* FindTargetActorByTag(FName Tag);

	UPROPERTY()
	AActor* CachedTargetActor = nullptr;

	UFUNCTION()
	void CheckReachDestination();

	FTimerHandle MessageTimerHandle;
	void StartMessages();
	void PlayMessage();
	void NextMessage();

	float GetMessageDisplayTime();

	UPROPERTY(BlueprintReadOnly, Category = "QuestMessages")
	FText CurrentMessage = FText::GetEmpty();	
	
	UPROPERTY(BlueprintReadOnly, Category = "QuestMessages")
	FText CurrentFaction = FText::GetEmpty();
	
	UPROPERTY(BlueprintReadOnly, Category = "QuestMessages")
	FText CurrentPrompt = FText::GetEmpty();
};

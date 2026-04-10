// Fill out your copyright notice in the Description page of Project Settings.


#include "QuestSubsystem.h"
#include "GlobalTowerSettings.h"
#include <Kismet/GameplayStatics.h>


TStatId UQuestSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UQuestSubsystem, STATGROUP_Tickables);
}

void UQuestSubsystem::StartQuest(FName QuestID)
{
	UE_LOG(LogTemp, Warning, TEXT("[Quest] Trying to start next quest: %s"), *QuestID.ToString());

	if (UnlockedQuests.Contains(QuestID) && !AllQuests[QuestID].bIsStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Quest] Starting next quest: %s"), *QuestID.ToString());
		CurrentQuest = AllQuests[QuestID];

		CurrentQuest.CurrentMessageIndex = 0;
		CurrentQuest.CurrentKillCount = 0;
		AllQuests[QuestID].bIsCompleted = false;
		AllQuests[QuestID].bIsStarted = true;

		CurrentPrompt = CurrentQuest.Prompt;

		CachedTargetActor = nullptr;

		if (!CurrentQuest.TargetTag.IsNone())
		{
			CachedTargetActor = FindTargetActorByTag(CurrentQuest.TargetTag);
		}

		for (const FName& NextID : CurrentQuest.NextQuestIDs)
		{
			if (!NextID.IsNone())
			{
				UnlockedQuests.Add(NextID);
			}
		}

		StartMessages();
	}
}

void UQuestSubsystem::CompleteObjective()
{
	CompleteQuest();
}

void UQuestSubsystem::CompleteQuest()
{
	CurrentQuest.bIsCompleted = true;
	CompletedQuests.Add(CurrentQuest.QuestID);

	GetWorld()->GetTimerManager().ClearTimer(MessageTimerHandle);

	CurrentPrompt = FText::GetEmpty();

	TArray<FName> ValidNextQuests;

	if (CurrentQuest.bAutoStartNext)
	{
		for (const FName& NextID : CurrentQuest.NextQuestIDs)
		{
			if (!NextID.IsNone())
			{
				ValidNextQuests.Add(NextID);
			}
		}

		// autostart if only one valid
		if (ValidNextQuests.Num() == 1)
		{
			FName NextQuestID = ValidNextQuests[0];

			UE_LOG(LogTemp, Warning, TEXT("[Quest] Auto-starting next quest: %s"), *NextQuestID.ToString());

			StartQuest(NextQuestID);
			return;
		}
	}

	OnQuestUpdated.Broadcast();
}

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UGlobalTowerSettings* Settings = UGlobalTowerSettings::Get();

	if (!Settings)
	{
		return;
	}

	UDataTable* DataTable = Settings->QuestDataTable.LoadSynchronous();
	if (!DataTable) return;

	AllQuests.Empty();
	TArray<FQuestData*> Rows;
	DataTable->GetAllRows<FQuestData>(TEXT("LoadQuestData"), Rows);

	for (FQuestData* Row : Rows)
	{
		if (Row)
		{
			AllQuests.Add(Row->QuestID, *Row);
		}
	}

	UnlockedQuests = {
		"Onboard_1"
	};

	StartQuest("Onboard_1");
}

void UQuestSubsystem::RegisterEnemyKilled()
{
	if (CurrentQuest.ObjectiveType != EObjectiveType::KillEnemies) return;
	
	// Add checking if enemy has a certain tag if you want quests to track certain tags

	CurrentQuest.CurrentKillCount++;
	CurrentQuest.Prompt = FText::Format(
		FText::FromString("Enemies defeated: {0} / {1}"),
		FText::AsNumber(CurrentQuest.CurrentKillCount),
		FText::AsNumber(CurrentQuest.RequiredKillCount)
	);

	OnQuestUpdated.Broadcast();
	if (CurrentQuest.CurrentKillCount >= CurrentQuest.RequiredKillCount)
	{
		CompleteObjective();
	}
	
}

void UQuestSubsystem::UpdateTimeLeft(float DeltaTime)
{
	if (CurrentQuest.ObjectiveType == EObjectiveType::Timer)
	{
		CurrentQuest.TimeLeft -= DeltaTime;
		UE_LOG(LogTemp, Warning, TEXT("Time Left: %.2f"), CurrentQuest.TimeLeft);
		CurrentQuest.
			Prompt = FText::Format(
			FText::FromString("{0} seconds left!"),
			FText::AsNumber(FMath::CeilToInt(CurrentQuest.TimeLeft))
		);
		OnQuestUpdated.Broadcast();
		if (CurrentQuest.TimeLeft <= 0)
		{
			CompleteObjective();
		}
	}
}

AActor* UQuestSubsystem::FindTargetActorByTag(FName Tag)
{
	// Returns first actor found with matching tag
	if (!GetWorld()) return nullptr;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, FoundActors);

	return FoundActors.Num() > 0 ? FoundActors[0] : nullptr;
}

void UQuestSubsystem::CheckReachDestination()
{
	if (CurrentQuest.ObjectiveType != EObjectiveType::ReachDestination) return;
	
	if (!CachedTargetActor)
	{
		CachedTargetActor = FindTargetActorByTag(CurrentQuest.TargetTag);
		return;
	}

	APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (!PlayerPawn) return;

	float Dist = FVector::Dist(
		PlayerPawn->GetActorLocation(),
		CachedTargetActor->GetActorLocation()
	);

	if (Dist <= CurrentQuest.RequiredDistance)
	{
		CompleteObjective();
	}
}

void UQuestSubsystem::StartMessages()
{
	GetWorld()->GetTimerManager().ClearTimer(MessageTimerHandle);

	CurrentMessage = FText::GetEmpty();
	CurrentFaction = FText::GetEmpty();

	PlayMessage();
}

void UQuestSubsystem::PlayMessage()
{
	if (!CurrentQuest.Messages.IsValidIndex(CurrentQuest.CurrentMessageIndex)) return;

	const FQuestMessage& Msg = CurrentQuest.Messages[CurrentQuest.CurrentMessageIndex];

	CurrentMessage = Msg.Text;
	CurrentFaction = Msg.Faction;

	OnQuestUpdated.Broadcast();

	GetWorld()->GetTimerManager().SetTimer(
		MessageTimerHandle,
		this,
		&UQuestSubsystem::NextMessage,
		GetMessageDisplayTime(),
		false
	);
}

void UQuestSubsystem::NextMessage()
{
	if (CurrentQuest.bIsCompleted)
	{
		CurrentMessage = FText::GetEmpty();
		CurrentFaction = FText::GetEmpty();

		OnQuestUpdated.Broadcast();

		GetWorld()->GetTimerManager().ClearTimer(MessageTimerHandle);
		return;
	}

	CurrentQuest.CurrentMessageIndex++;

	if (CurrentQuest.Messages.IsValidIndex(CurrentQuest.CurrentMessageIndex))
	{
		PlayMessage();
	}
	else
	{
		CurrentMessage = FText::GetEmpty();
		CurrentFaction = FText::GetEmpty();
			
		OnQuestUpdated.Broadcast();

		GetWorld()->GetTimerManager().ClearTimer(MessageTimerHandle);
		if (CurrentQuest.ObjectiveType == EObjectiveType::JustMessage) CompleteObjective();
	}
}

float UQuestSubsystem::GetMessageDisplayTime()
{
	const FQuestMessage& Msg = CurrentQuest.Messages[CurrentQuest.CurrentMessageIndex];

	if (Msg.Delay > 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MessageTiming] Using explicit delay: %.2f seconds | Message: %s"),
			Msg.Delay,
			*Msg.Text.ToString()
		);

		return Msg.Delay;
	}

	const FString MsgString = Msg.Text.ToString();
	const int32 CharCount = MsgString.Len();

	// reading speed of 15 char per second?
	float Time = CharCount / 15.0f + 5.0f;

	UE_LOG(LogTemp, Warning, TEXT("[MessageTiming] Calculated delay: %.2f seconds (Chars: %d) | Message: %s"),
		Time,
		CharCount,
		*MsgString
	);

	return FMath::Max(Time, 3.0f);
}
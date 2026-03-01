// Fill out your copyright notice in the Description page of Project Settings.


#include "QuestSubsystem.h"

void UQuestSubsystem::StartQuest(FName QuestID)
{
	if (AllQuests.Contains(QuestID))
	{
		CurrentQuest = AllQuests[QuestID];
		UE_LOG(LogTemp, Warning, TEXT("Started Quest: %s"), *CurrentQuest.Title.ToString());
		OnQuestUpdated.Broadcast(CurrentQuest);
	}
}

void UQuestSubsystem::CompleteObjective()
{
	UE_LOG(LogTemp, Warning, TEXT("Objective Index: %d"), CurrentQuest.CurrentObjectiveIndex);
	if (CurrentQuest.IsCompleted)
	{
		return;
	}

	CurrentQuest.CurrentObjectiveIndex++;

	if (CurrentQuest.CurrentObjectiveIndex >= CurrentQuest.Objectives.Num())
	{
		CompleteQuest();
	}
	else
	{
		OnQuestUpdated.Broadcast(CurrentQuest);
	}
}

void UQuestSubsystem::CompleteQuest()
{
	UE_LOG(LogTemp, Warning, TEXT("Completed Quest: %s"), *CurrentQuest.Title.ToString());
	CurrentQuest.IsCompleted = true;
	if (!CurrentQuest.NextQuestID.IsNone())
	{
		StartQuest(CurrentQuest.NextQuestID);
	}
}

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("Quest_One Initialized"));
	FQuestData QuestOne;
	QuestOne.QuestID = "Quest_One";
	QuestOne.Title = FText::FromString("Find the core");
	QuestOne.Description = FText::FromString("Use WASD to move and your mouse to look around and reach the core.");
	QuestOne.Objectives = {
		FText::FromString("Move towards the waypoint"),
	};
	QuestOne.NextQuestID = "Quest_Two";

	AllQuests.Add(QuestOne.QuestID, QuestOne);

	UE_LOG(LogTemp, Warning, TEXT("Quest_Two Initialized"));
	FQuestData QuestTwo;
	QuestTwo.QuestID = "Quest_Two";
	QuestTwo.Title = FText::FromString("Activate the core");
	QuestTwo.Description = FText::FromString("Locate and activate the core");
	QuestTwo.Objectives = {
		FText::FromString("Press E to Interact with the core")
	};
	QuestTwo.NextQuestID = "Quest_Three";

	AllQuests.Add(QuestTwo.QuestID, QuestTwo);

	UE_LOG(LogTemp, Warning, TEXT("Quest_Three Initialized"));
	FQuestData QuestThree;
	QuestThree.QuestID = "Quest_Three";
	QuestThree.Title = FText::FromString("No quests left for now!");
	QuestThree.Description = FText::FromString("Please wait for further updates");
	QuestThree.Objectives = {
		FText::FromString("Nothing to see here...")
	};

	AllQuests.Add(QuestThree.QuestID, QuestThree);
}
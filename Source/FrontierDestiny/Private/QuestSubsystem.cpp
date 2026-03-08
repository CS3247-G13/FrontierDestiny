// Fill out your copyright notice in the Description page of Project Settings.


#include "QuestSubsystem.h"

void UQuestSubsystem::StartQuest(FName QuestID)
{
	if (AllQuests.Contains(QuestID))
	{
		CurrentQuest = AllQuests[QuestID];
		UE_LOG(LogTemp, Warning, TEXT("Started Quest: %s"), *CurrentQuest.Title.ToString());
		OnQuestUpdated.Broadcast();
	}
}

void UQuestSubsystem::CompleteObjective()
{
	UE_LOG(LogTemp, Warning, TEXT("Objective Index: %d"), CurrentQuest.CurrentObjectiveIndex);
	CurrentQuest.CurrentObjectiveIndex++;

	if (CurrentQuest.CurrentObjectiveIndex >= CurrentQuest.Objectives.Num())
	{
		CompleteQuest();
	}
	else
	{
		OnQuestUpdated.Broadcast();
	}
}

void UQuestSubsystem::CompleteQuest()
{
	UE_LOG(LogTemp, Warning, TEXT("Completed Quest: %s"), *CurrentQuest.Title.ToString());
	CurrentQuest.bIsCompleted = true;
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
	QuestOne.RequiredDistance = 900.f;
	QuestOne.Title = FText::FromString("Find the core");
	QuestOne.Description = FText::FromString("Use WASD to move and your mouse to look around and reach the core.");
	QuestOne.Objectives = {
		FText::FromString("Move towards the waypoint"),
	};
	QuestOne.NextQuestID = "Quest_Two";
	QuestOne.ObjectiveType = EObjectiveType::ReachDestination;
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
	QuestTwo.ObjectiveType = EObjectiveType::Interact;
	AllQuests.Add(QuestTwo.QuestID, QuestTwo);

	FQuestData QuestThree;
	QuestThree.QuestID = "Quest_Three";
	QuestThree.Title = FText::FromString("Enemies are coming!");
	QuestThree.Description = FText::FromString("Build towers, Defend core.");
	QuestThree.TimeLeft = 60;
	QuestThree.Objectives = {
		FText::Format(
			FText::FromString("Enemies coming in {0} seconds"),
			FText::AsNumber(QuestThree.TimeLeft)
		)
	};

	QuestThree.NextQuestID = "Quest_Four";
	QuestThree.ObjectiveType = EObjectiveType::Timer;
	AllQuests.Add(QuestThree.QuestID, QuestThree);

	UE_LOG(LogTemp, Warning, TEXT("Quest_Four Initialized"));
	FQuestData QuestFour;
	QuestFour.QuestID = "Quest_Four";
	QuestFour.Title = FText::FromString("Kill them all!");
	QuestFour.Description = FText::FromString("Fight!");
	QuestFour.RequiredKillCount = 40;
	QuestFour.Objectives = {
		FText::Format(
			FText::FromString("Enemies defeated: {0} / {1}"),
			FText::AsNumber(QuestFour.CurrentKillCount),
			FText::AsNumber(QuestFour.RequiredKillCount)
		)
	};

	QuestFour.NextQuestID = "Quest_Four";
	QuestFour.ObjectiveType = EObjectiveType::KillEnemies;
	AllQuests.Add(QuestFour.QuestID, QuestFour);
}

void UQuestSubsystem::RegisterEnemyKilled()
{
	if(CurrentQuest.ObjectiveType == EObjectiveType::KillEnemies)
	{
		CurrentQuest.CurrentKillCount++;
		CurrentQuest.Objectives[CurrentQuest.CurrentObjectiveIndex] = FText::Format(
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
}

void UQuestSubsystem::UpdateTimeLeft(float seconds)
{
	if (CurrentQuest.ObjectiveType == EObjectiveType::Timer)
	{
		CurrentQuest.TimeLeft -= seconds;
		UE_LOG(LogTemp, Warning, TEXT("Time Left: %.2f"), CurrentQuest.TimeLeft);
		CurrentQuest.Objectives[CurrentQuest.CurrentObjectiveIndex] = FText::Format(
			FText::FromString("Enemies coming in {0} seconds"),
			FText::AsNumber(FMath::CeilToInt(CurrentQuest.TimeLeft))
		);
		OnQuestUpdated.Broadcast();
		if (CurrentQuest.TimeLeft <= 0)
		{
			CompleteObjective();
		}
	}
}
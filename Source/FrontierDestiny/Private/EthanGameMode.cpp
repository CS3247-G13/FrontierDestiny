// Fill out your copyright notice in the Description page of Project Settings.


#include "EthanGameMode.h"

void AEthanGameMode::StartPlay()
{
	Super::StartPlay();
	check(GEngine != nullptr);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Hello World, this is EthanGameMode speaking!"));
}
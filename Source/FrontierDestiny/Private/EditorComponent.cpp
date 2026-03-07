// Fill out your copyright notice in the Description page of Project Settings.

#include "EditorComponent.h"
#include "EconomyComponent.h"
#include "TowerActor.h"
#include "TowerDemoPlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "TowerData.h"
#include "GridActor.h"

#define LOG(Message) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Message)
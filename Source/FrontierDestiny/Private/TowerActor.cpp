// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerActor.h"

// Sets default values
ATowerActor::ATowerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

bool ATowerActor::SelectTarget()
{
	return true;
}

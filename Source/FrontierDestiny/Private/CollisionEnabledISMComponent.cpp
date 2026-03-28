// Fill out your copyright notice in the Description page of Project Settings.


#include "CollisionEnabledISMComponent.h"

void UCollisionEnabledISMComponent::OnRegister()
{
	Super::OnRegister();
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCollisionObjectType(ECC_GameTraceChannel4);
	SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Overlap);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "CollisionEnabledISMComponent.h"
#include "CustomChannels.h"

void UCollisionEnabledISMComponent::OnRegister()
{
	Super::OnRegister();
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCollisionObjectType(CC_Enemy);
	SetCollisionResponseToChannel(CC_Enemy, ECR_Overlap);
}

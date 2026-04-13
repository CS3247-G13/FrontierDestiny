#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Shootable.generated.h"

UINTERFACE(MinimalAPI)
class UShootable : public UInterface
{
	GENERATED_BODY()
};

class FRONTIERDESTINY_API IShootable
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Shooting")
	void Shoot(const FHitResult& Hit, int32 Damage);

};
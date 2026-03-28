#pragma once

#include "MassNavigationTypes.h"
#include "MassStateTreeTypes.h"
#include "MassNearestCoreTargetTask.generated.h"

struct FAgentRadiusFragment;
struct FStateTreeExecutionContext;
struct FTransformFragment;

USTRUCT(BlueprintType)
struct FMassNearestCoreTargetTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation;
};

/**
 * Sets TargetLocation to the nearest active core for NavMesh path following.
 */
USTRUCT(meta = (DisplayName = "NavMesh Nearest Core Target"))
struct FMassNearestCoreTargetTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassNearestCoreTargetTaskInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FAgentRadiusFragment> AgentRadiusHandle;
};

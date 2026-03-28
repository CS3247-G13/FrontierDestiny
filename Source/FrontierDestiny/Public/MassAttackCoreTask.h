#pragma once

#include "MassStateTreeTypes.h"
#include "StatusEffectFragments.h"
#include "MassAttackCoreTask.generated.h"

struct FTransformFragment;

USTRUCT(BlueprintType)
struct FMassAttackCoreTaskInstanceData
{
	GENERATED_BODY()
};

/**
 * Damages the nearest active core and destroys the entity.
 * Place this in the state reached after the NavMesh move task succeeds.
 */
USTRUCT(meta = (DisplayName = "Attack Core And Die"))
struct FMassAttackCoreTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassAttackCoreTaskInstanceData;

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FStatsFragment, EStateTreeExternalDataRequirement::Optional> StatsHandle;
};

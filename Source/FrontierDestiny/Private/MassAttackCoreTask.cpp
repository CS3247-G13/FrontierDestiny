#include "MassAttackCoreTask.h"
#include "MassCommonFragments.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "CoreManagerSubsystem.h"
#include "CoreActor.h"
#include "EnemyManagerSubsystem.h"
#include "HordeIDFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassAttackCoreTask)

bool FMassAttackCoreTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(StatsHandle);
	return true;
}

EStateTreeRunStatus FMassAttackCoreTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassContext = static_cast<const FMassStateTreeExecutionContext&>(Context);
	UWorld* World = MassContext.GetEntityManager().GetWorld();
	if (!World) return EStateTreeRunStatus::Failed;

	UCoreManagerSubsystem* CoreManager = World->GetSubsystem<UCoreManagerSubsystem>();
	if (!CoreManager) return EStateTreeRunStatus::Failed;

	const FTransformFragment& TransformFragment = Context.GetExternalData(TransformHandle);
	const FVector EntityLocation = TransformFragment.GetTransform().GetLocation();

	float Damage = 10.f;
	if (const FStatsFragment* Stats = Context.GetExternalDataPtr(StatsHandle))
	{
		Damage = Stats->BaseDamage;
	}

	UE_LOG(LogTemp, Warning, TEXT("MassAttackCoreTask: EnterState called, damage=%.1f"), Damage);

	if (ACoreActor* Core = CoreManager->GetNearestActiveCore(EntityLocation))
	{
		Core->ApplyDamage(Damage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MassAttackCoreTask: No active core found"));
	}

	const FMassEntityHandle Entity = MassContext.GetEntity();

	// TODO: death notification logic is duplicated from EnemyDamageMassProcessor — consider a kill fragment
	// so all death paths go through one place instead of manually mirroring this here.
	if (UEnemyManagerSubsystem* EnemyManager = World->GetGameInstance()->GetSubsystem<UEnemyManagerSubsystem>())
	{
		if (const FHordeIDFragment* HordeID = MassContext.GetEntityManager().GetFragmentDataPtr<FHordeIDFragment>(Entity))
		{
			EnemyManager->NotifyHordeEnemyDeath(HordeID->HordeID);
		}
		EnemyManager->NotifyEnemyDeath(Entity);
	}

	MassContext.GetEntityManager().Defer().DestroyEntity(Entity);

	return EStateTreeRunStatus::Succeeded;
}

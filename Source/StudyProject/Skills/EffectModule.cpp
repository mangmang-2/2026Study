#include "EffectModule.h"

void UEffectModule::Execute(const FSkillExecutionContext& Ctx)
{
}

FText UEffectModule::GetSummary() const
{
    return GetClass()->GetDisplayNameText();
}

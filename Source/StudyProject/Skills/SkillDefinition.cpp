#include "SkillDefinition.h"

#if WITH_EDITOR
#include "EffectModule.h"
#include "Modules/DamageEffectModule.h"
#include "Modules/PullEffectModule.h"
#include "Modules/PushEffectModule.h"
#include "Modules/StunEffectModule.h"
#include "Modules/SlowEffectModule.h"
#include "Modules/KnockupEffectModule.h"
#include "AssetToolsModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
    void AppendModule(USkillDefinition* Skill, UClass* ModuleClass)
    {
        if (Skill == nullptr || ModuleClass == nullptr)
        {
            return;
        }
        UEffectModule* NewModule = NewObject<UEffectModule>(Skill, ModuleClass, NAME_None, RF_Transactional);
        Skill->EffectModules.Add(NewModule);
        Skill->MarkPackageDirty();
    }
}

void USkillDefinition::AddDamageModule()
{
    AppendModule(this, UDamageEffectModule::StaticClass());
}

void USkillDefinition::AddPullModule()
{
    AppendModule(this, UPullEffectModule::StaticClass());
}

void USkillDefinition::AddPushModule()
{
    AppendModule(this, UPushEffectModule::StaticClass());
}

void USkillDefinition::AddStunModule()
{
    AppendModule(this, UStunEffectModule::StaticClass());
}

void USkillDefinition::AddSlowModule()
{
    AppendModule(this, USlowEffectModule::StaticClass());
}

void USkillDefinition::AddKnockupModule()
{
    AppendModule(this, UKnockupEffectModule::StaticClass());
}

void USkillDefinition::ClearModules()
{
    EffectModules.Empty();
    MarkPackageDirty();
}

void USkillDefinition::DuplicateThisSkill()
{
    FAssetToolsModule& ATModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = ATModule.Get();

    const FString PackageName = GetOutermost()->GetName();
    const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

    FString OutPackageName;
    FString OutAssetName;
    AssetTools.CreateUniqueAssetName(PackageName, TEXT("_Copy"), OutPackageName, OutAssetName);

    AssetTools.DuplicateAsset(OutAssetName, PackagePath, this);
}
#endif

#include "SkillForgeLibrary.h"

#if WITH_EDITOR
#include "Skills/SkillDefinition.h"
#include "Skills/EffectModule.h"
#include "Skills/SkillPreviewActor.h"
#include "LevelEditorViewport.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Skills/Modules/DamageEffectModule.h"
#include "Skills/Modules/PullEffectModule.h"
#include "Skills/Modules/PushEffectModule.h"
#include "Skills/Modules/StunEffectModule.h"
#include "Skills/Modules/SlowEffectModule.h"
#include "GAS/GE_Damage.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "LevelEditorSubsystem.h"
#endif

TArray<USkillDefinition*> USkillForgeLibrary::GetAllSkills()
{
    TArray<USkillDefinition*> Result;

#if WITH_EDITOR
    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    FARFilter Filter;
    Filter.PackagePaths.Add(FName(TEXT("/Game/Skills")));
    Filter.ClassPaths.Add(USkillDefinition::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;
    Registry.GetAssets(Filter, Assets);

    for (const FAssetData& Asset : Assets)
    {
        if (USkillDefinition* Skill = Cast<USkillDefinition>(Asset.GetAsset()))
        {
            Result.Add(Skill);
        }
    }
#endif

    return Result;
}

USkillDefinition* USkillForgeLibrary::CreateSkill(const FString& SkillName)
{
#if WITH_EDITOR
    FString Clean = SkillName.IsEmpty() ? TEXT("NewSkill") : SkillName;
    Clean = Clean.Replace(TEXT(" "), TEXT(""));

    FAssetToolsModule& ATModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = ATModule.Get();

    const FString PackagePath = TEXT("/Game/Skills");
    const FString Desired = PackagePath / (TEXT("DA_Skill_") + Clean);

    FString OutPackageName;
    FString OutAssetName;
    AssetTools.CreateUniqueAssetName(Desired, TEXT(""), OutPackageName, OutAssetName);

    UObject* NewAsset = AssetTools.CreateAsset(OutAssetName, PackagePath, USkillDefinition::StaticClass(), nullptr);
    return Cast<USkillDefinition>(NewAsset);
#else
    return nullptr;
#endif
}

USkillDefinition* USkillForgeLibrary::DuplicateSkill(USkillDefinition* Source, const FString& NewName)
{
#if WITH_EDITOR
    if (Source == nullptr)
    {
        return nullptr;
    }

    FString Clean = NewName.IsEmpty() ? (Source->GetName() + TEXT("_Copy")) : NewName;
    Clean = Clean.Replace(TEXT(" "), TEXT(""));

    FAssetToolsModule& ATModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = ATModule.Get();

    const FString PackagePath = TEXT("/Game/Skills");
    const FString Desired = PackagePath / (TEXT("DA_Skill_") + Clean);

    FString OutPackageName;
    FString OutAssetName;
    AssetTools.CreateUniqueAssetName(Desired, TEXT(""), OutPackageName, OutAssetName);

    UObject* Dup = AssetTools.DuplicateAsset(OutAssetName, PackagePath, Source);
    return Cast<USkillDefinition>(Dup);
#else
    return nullptr;
#endif
}

bool USkillForgeLibrary::DeleteSkill(USkillDefinition* Skill)
{
#if WITH_EDITOR
    if (Skill == nullptr)
    {
        return false;
    }
    return UEditorAssetLibrary::DeleteLoadedAsset(Skill);
#else
    return false;
#endif
}

void USkillForgeLibrary::SaveSkill(USkillDefinition* Skill)
{
#if WITH_EDITOR
    if (Skill == nullptr)
    {
        return;
    }
    UEditorAssetLibrary::SaveLoadedAsset(Skill, false);
#endif
}

UEffectModule* USkillForgeLibrary::AddModule(USkillDefinition* Skill, ESkillModuleType ModuleType)
{
#if WITH_EDITOR
    if (Skill == nullptr)
    {
        return nullptr;
    }

    UClass* ModuleClass = nullptr;
    switch (ModuleType)
    {
    case ESkillModuleType::Damage:
        ModuleClass = UDamageEffectModule::StaticClass();
        break;
    case ESkillModuleType::Pull:
        ModuleClass = UPullEffectModule::StaticClass();
        break;
    case ESkillModuleType::Push:
        ModuleClass = UPushEffectModule::StaticClass();
        break;
    case ESkillModuleType::Stun:
        ModuleClass = UStunEffectModule::StaticClass();
        break;
    case ESkillModuleType::Slow:
        ModuleClass = USlowEffectModule::StaticClass();
        break;
    default:
        return nullptr;
    }

    UEffectModule* NewModule = NewObject<UEffectModule>(Skill, ModuleClass, NAME_None, RF_Transactional);
    Skill->EffectModules.Add(NewModule);
    Skill->MarkPackageDirty();
    return NewModule;
#else
    return nullptr;
#endif
}

void USkillForgeLibrary::RemoveModuleAt(USkillDefinition* Skill, int32 Index)
{
#if WITH_EDITOR
    if (Skill == nullptr)
    {
        return;
    }
    if (Skill->EffectModules.IsValidIndex(Index) == false)
    {
        return;
    }
    Skill->EffectModules.RemoveAt(Index);
    Skill->MarkPackageDirty();
#endif
}

void USkillForgeLibrary::OpenSkillInEditor(USkillDefinition* Skill)
{
#if WITH_EDITOR
    if (Skill == nullptr || GEditor == nullptr)
    {
        return;
    }
    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Skill);
#endif
}

void USkillForgeLibrary::StopPreview()
{
#if WITH_EDITOR
    if (GEditor == nullptr)
    {
        return;
    }
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World == nullptr)
    {
        return;
    }

    TArray<AActor*> Existing;
    UGameplayStatics::GetAllActorsOfClass(World, ASkillPreviewActor::StaticClass(), Existing);
    for (AActor* Old : Existing)
    {
        if (Old != nullptr)
        {
            Old->Destroy();
        }
    }
#endif
}

void USkillForgeLibrary::PreviewSkill(USkillDefinition* Skill)
{
#if WITH_EDITOR
    if (Skill == nullptr || GEditor == nullptr)
    {
        return;
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World == nullptr)
    {
        return;
    }

    // 이전 프리뷰(액터+그 VFX) 먼저 제거 — 새 이펙트로 교체
    StopPreview();

    // 틱이 돌도록 뷰포트 리얼타임 ON
    for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
    {
        if (Client != nullptr)
        {
            Client->SetRealtime(true);
        }
    }

    // 레벨에 시전자/타격 큐브가 있으면 그 위치로 재생(없으면 카메라 정면 폴백)
    AActor* CasterCube = nullptr;
    AActor* TargetCube = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* A = *It;
        if (A == nullptr)
        {
            continue;
        }
        if (A->ActorHasTag(TEXT("SkillPreviewCaster")))
        {
            CasterCube = A;
        }
        else if (A->ActorHasTag(TEXT("SkillPreviewTarget")))
        {
            TargetCube = A;
        }
    }

    FVector CasterLoc;
    FVector Center;
    if (CasterCube != nullptr && TargetCube != nullptr)
    {
        CasterLoc = CasterCube->GetActorLocation();
        Center = TargetCube->GetActorLocation();
    }
    else
    {
        FVector CamLoc(0.f, 0.f, 500.f);
        FRotator CamRot(-30.f, 0.f, 0.f);
        for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
        {
            if (Client != nullptr)
            {
                CamLoc = Client->GetViewLocation();
                CamRot = Client->GetViewRotation();
                break;
            }
        }
        const FVector Forward = CamRot.Vector();
        Center = CamLoc + Forward * 900.f;
        CasterLoc = CamLoc + Forward * 300.f;
    }

    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ASkillPreviewActor* Preview = World->SpawnActor<ASkillPreviewActor>(
        ASkillPreviewActor::StaticClass(), Center, FRotator::ZeroRotator, Params);
    if (Preview != nullptr)
    {
        Preview->InitPreview(Skill, Center, CasterLoc);
    }
#endif
}

void USkillForgeLibrary::OpenPreviewLevel()
{
#if WITH_EDITOR
    if (GEditor == nullptr)
    {
        return;
    }

    const FString LevelPath = TEXT("/Game/Skills/Tools/L_SkillForgePreview");
    if (UEditorAssetLibrary::DoesAssetExist(LevelPath) == false)
    {
        return;
    }

    if (ULevelEditorSubsystem* LevelSub = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>())
    {
        LevelSub->LoadLevel(LevelPath);
    }
#endif
}

FString USkillForgeLibrary::GetSkillSummary(USkillDefinition* Skill)
{
    if (Skill == nullptr)
    {
        return TEXT("(none)");
    }

    const UEnum* DeliveryEnum = StaticEnum<ESkillDeliveryType>();
    const FString Delivery = (DeliveryEnum != nullptr)
        ? DeliveryEnum->GetNameStringByValue(static_cast<int64>(Skill->DeliveryType))
        : TEXT("?");

    FString Name = Skill->SkillName.IsEmpty() ? Skill->GetName() : Skill->SkillName.ToString();

    return FString::Printf(TEXT("%s  [%s]  ·  모듈 %d"),
        *Name, *Delivery, Skill->EffectModules.Num());
}

#if WITH_EDITOR
namespace
{
    // GE CDO에서 부여 태그/에셋 태그/지속을 풀어 한 줄씩 덧붙인다.
    void AppendGELines(FString& S, TSubclassOf<UGameplayEffect> GEClass, float DurationOverride)
    {
        if (GEClass == nullptr)
        {
            S += TEXT("       GE: (미지정 → 모듈 기본값)\n");
            return;
        }

        S += FString::Printf(TEXT("       GE: %s\n"), *GEClass->GetName());

        const UGameplayEffect* CDO = GEClass.GetDefaultObject();
        if (CDO != nullptr)
        {
            const FGameplayTagContainer& Granted = CDO->GetGrantedTags();
            if (Granted.Num() > 0)
            {
                S += FString::Printf(TEXT("       부여 태그: %s\n"), *Granted.ToStringSimple());
            }
            const FGameplayTagContainer& Asset = CDO->GetAssetTags();
            if (Asset.Num() > 0)
            {
                S += FString::Printf(TEXT("       에셋 태그: %s\n"), *Asset.ToStringSimple());
            }
        }

        if (DurationOverride > 0.f)
        {
            S += FString::Printf(TEXT("       지속: %.1fs (모듈에서 덮어씀)\n"), DurationOverride);
        }
    }
}
#endif

FString USkillForgeLibrary::GetGASInfo(USkillDefinition* Skill)
{
#if WITH_EDITOR
    if (Skill == nullptr)
    {
        return TEXT("(스킬을 선택하세요)");
    }

    const UEnum* DeliveryEnum = StaticEnum<ESkillDeliveryType>();
    const UEnum* TargetEnum = StaticEnum<ESkillTargetingMode>();
    const FString Delivery = (DeliveryEnum != nullptr)
        ? DeliveryEnum->GetNameStringByValue(static_cast<int64>(Skill->DeliveryType)) : TEXT("?");
    const FString Targeting = (TargetEnum != nullptr)
        ? TargetEnum->GetNameStringByValue(static_cast<int64>(Skill->TargetingMode)) : TEXT("?");

    FString S;

    // ── 실행 어빌리티 ───────────────────────────────
    S += TEXT("● 실행 어빌리티 (GA)\n");
    S += TEXT("   GA_SkillExecutor  (ServerOnly · 데이터 구동)\n");
    S += FString::Printf(TEXT("   타겟팅 %s  ·  전달 %s\n"), *Targeting, *Delivery);
    S += FString::Printf(TEXT("   시전 %.2fs  ·  쿨다운 %.1fs"), Skill->CastTime, Skill->Cooldown);
    if (Skill->Duration > 0.f)
    {
        S += FString::Printf(TEXT("  ·  지속 %.1fs (%.2fs마다 재판정)"), Skill->Duration, Skill->TickInterval);
    }
    if (Skill->DeliveryType == ESkillDeliveryType::Rain)
    {
        S += FString::Printf(TEXT("\n   낙하 %d회 / %.1fs  ·  착탄반경 %.0f"),
            Skill->RainStrikeCount, Skill->RainDuration, Skill->RainStrikeRadius);
    }
    S += TEXT("\n\n");

    // ── 모듈별 GE / 태그 ────────────────────────────
    S += FString::Printf(TEXT("● 적용 GameplayEffect / 태그   (모듈 %d)\n"), Skill->EffectModules.Num());
    if (Skill->EffectModules.Num() == 0)
    {
        S += TEXT("   (효과 모듈 없음 — ＋추가로 조합)\n");
    }

    for (int32 i = 0; i < Skill->EffectModules.Num(); ++i)
    {
        UEffectModule* M = Skill->EffectModules[i];
        if (M == nullptr)
        {
            S += FString::Printf(TEXT("   [%d] (빈 슬롯)\n"), i);
            continue;
        }

        if (UDamageEffectModule* Dmg = Cast<UDamageEffectModule>(M))
        {
            S += FString::Printf(TEXT("   [%d] 데미지 %.0f\n"), i, Dmg->DamageAmount);
            TSubclassOf<UGameplayEffect> GE = Dmg->DamageGEClass
                ? Dmg->DamageGEClass : TSubclassOf<UGameplayEffect>(UGE_Damage::StaticClass());
            S += FString::Printf(TEXT("       GE: %s  ·  Instant\n"), *GE->GetName());
            S += TEXT("       SetByCaller: Data.Damage → Damage 속성 차감\n");
        }
        else if (UStunEffectModule* Stun = Cast<UStunEffectModule>(M))
        {
            S += FString::Printf(TEXT("   [%d] 스턴(감전)\n"), i);
            AppendGELines(S, Stun->StatusGEClass, Stun->StunDuration);
        }
        else if (USlowEffectModule* Slow = Cast<USlowEffectModule>(M))
        {
            S += FString::Printf(TEXT("   [%d] 둔화\n"), i);
            AppendGELines(S, Slow->StatusGEClass, Slow->SlowDuration);
        }
        else if (Cast<UPullEffectModule>(M) != nullptr)
        {
            S += FString::Printf(TEXT("   [%d] 당기기  ·  물리 힘(LaunchCharacter), GE 없음\n"), i);
        }
        else if (Cast<UPushEffectModule>(M) != nullptr)
        {
            S += FString::Printf(TEXT("   [%d] 밀치기  ·  물리 힘(LaunchCharacter), GE 없음\n"), i);
        }
        else
        {
            S += FString::Printf(TEXT("   [%d] %s\n"), i, *M->GetSummary().ToString());
        }
    }

    return S;
#else
    return FString();
#endif
}

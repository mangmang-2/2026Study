#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SkillForgeLibrary.generated.h"

class USkillDefinition;
class UEffectModule;

UENUM(BlueprintType)
enum class ESkillModuleType : uint8
{
    Damage,
    Pull,
    Push,
    Stun,
    Slow,
};

// 에디터 전용 — EUW_SkillForge 에서 호출. 스킬 DataAsset 저작/조합 헬퍼.
UCLASS()
class STUDYPROJECT_API USkillForgeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // /Game/Skills 의 모든 스킬 DataAsset
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static TArray<USkillDefinition*> GetAllSkills();

    // 새 빈 스킬 DataAsset 생성(/Game/Skills/DA_Skill_<Name>)
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static USkillDefinition* CreateSkill(const FString& SkillName);

    // 기존 스킬 복제
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static USkillDefinition* DuplicateSkill(USkillDefinition* Source, const FString& NewName);

    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static bool DeleteSkill(USkillDefinition* Skill);

    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static void SaveSkill(USkillDefinition* Skill);

    // 효과 모듈을 조합 배열에 추가하고 추가된 모듈을 반환(Details에서 값 편집)
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static UEffectModule* AddModule(USkillDefinition* Skill, ESkillModuleType ModuleType);

    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static void RemoveModuleAt(USkillDefinition* Skill, int32 Index);

    // 에셋 에디터 창 열기(네이티브 Details — 모든 옵션 편집)
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static void OpenSkillInEditor(USkillDefinition* Skill);

    // 현재 에디터 뷰포트 정면에 스킬 VFX(시전/투사체/착탄) + 범위를 즉석 표시(게임플레이 없이 비주얼만)
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static void PreviewSkill(USkillDefinition* Skill);

    // 재생 중인 모든 프리뷰(액터+VFX) 즉시 제거
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static void StopPreview();

    // 시전자/타격 큐브가 놓인 전용 프리뷰 레벨(/Game/Skills/Tools/L_SkillForgePreview)을 연다
    UFUNCTION(BlueprintCallable, Category = "SkillForge", meta = (DevelopmentOnly))
    static void OpenPreviewLevel();

    // 리스트 버튼 라벨용 한 줄 요약
    UFUNCTION(BlueprintPure, Category = "SkillForge")
    static FString GetSkillSummary(USkillDefinition* Skill);

    // 이 스킬이 실제로 부여하는 GE·태그를 읽기전용으로 풀어준 여러 줄 요약
    // (실행 GA / 모듈별 GE 클래스 + 부여 태그 + SetByCaller).
    UFUNCTION(BlueprintPure, Category = "SkillForge", meta = (DevelopmentOnly))
    static FString GetGASInfo(USkillDefinition* Skill);
};

#pragma once

#include "CoreMinimal.h"
#include "SkillTypes.h"
#include "EffectModule.generated.h"

/**
 * 합성 가능한 이펙트 모듈의 추상 베이스.
 * USkillDefinition이 이 모듈들의 배열을 들고, 판정 후 순서대로 Execute를 호출한다.
 *
 * EditInlineNew + DefaultToInstanced 덕분에 DataAsset 에디터에서 +로 모듈을 골라
 * 파라미터를 직접 입력해 조합할 수 있다(데이터 기반 스킬 제작).
 * Execute는 서버 권위에서만 호출된다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, CollapseCategories)
class STUDYPROJECT_API UEffectModule : public UObject
{
    GENERATED_BODY()

public:
    // 판정된 HitActors에 효과를 적용한다(서버 권위).
    virtual void Execute(const FSkillExecutionContext& Ctx);

    // 툴팁/스킬트리 UI 표시용 한 줄 요약(예: "데미지 80").
    virtual FText GetSummary() const;
};

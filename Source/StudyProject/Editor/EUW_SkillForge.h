#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Types/SlateEnums.h"
#include "EUW_SkillForge.generated.h"

class UComboBoxString;
class UEditableTextBox;
class UTextBlock;
class UButton;
class UWidget;
class UPanelWidget;
class UDetailsView;
class USkillDefinition;

/**
 * 스킬 포지 — 레이아웃은 UMG 디자이너(블루프린트)에서 구성하고, C++는 로직만 담당.
 *
 * 위젯은 전부 meta=(BindWidgetOptional) — 디자이너에 같은 이름으로 위젯을 배치하면 자동 바인딩된다.
 * 버튼 OnClicked/콤보 변경은 NativeConstruct에서 C++가 직접 묶으므로 BP 그래프 배선이 필요 없다.
 * (없는 위젯은 널 가드로 건너뛰므로, 일부만 배치해도 동작한다.)
 *
 * ── 디자이너에 둘 위젯 이름(타입) ─────────────────────────────
 *  스킬 선택   : SkillCombo(ComboBoxString), NameBox(EditableTextBox), SummaryText(TextBlock)
 *  스킬 버튼   : NewButton, DuplicateButton, DeleteButton, OpenButton, SaveButton, RefreshButton (Button)
 *  프리뷰      : PreviewButton, StopButton, OpenLevelButton (Button)
 *  모듈        : ModuleTypeCombo(ComboBoxString), AddModuleButton(Button), SkillDetails(DetailsView)
 *  GA 속성     : GADetails(DetailsView)
 *  GAS 정보    : GASInfoText(TextBlock)
 *  접이식 섹션 : <섹션>HeaderButton(Button) + <섹션>Body(아무 패널) — Skill/Preview/Module/GA/GASInfo
 */
UCLASS()
class STUDYPROJECT_API UEUW_SkillForge : public UEditorUtilityWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

private:
    void RefreshList();
    void UpdateSummary();
    void SelectSkill(USkillDefinition* Skill);
    void RefreshModuleList();

    // 디자이너에 DetailsView가 없으면 코드로 생성해 배치용 박스(또는 RootScroll)에 넣는다
    void EnsureDetailsViews();

    // 버튼 색 구분(섹션 헤더 vs 일반 버튼) + 새 이름 입력칸 크게
    void ApplyStyling();

    // 헤더 버튼으로 섹션 body 접기/펴기
    void ToggleSection(UWidget* Body);

    UFUNCTION() void OnToggleSkill();
    UFUNCTION() void OnTogglePreview();
    UFUNCTION() void OnToggleModule();
    UFUNCTION() void OnToggleGA();
    UFUNCTION() void OnToggleGASInfo();

    UFUNCTION() void OnComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION() void OnNewClicked();
    UFUNCTION() void OnDuplicateClicked();
    UFUNCTION() void OnDeleteClicked();
    UFUNCTION() void OnOpenClicked();
    UFUNCTION() void OnSaveClicked();
    UFUNCTION() void OnRefreshClicked();
    UFUNCTION() void OnPreviewClicked();
    UFUNCTION() void OnStopClicked();
    UFUNCTION() void OnOpenPreviewLevelClicked();
    UFUNCTION() void OnAddModuleClicked();

    // ── 디자이너 바인딩 위젯(전부 Optional) ───────────────────────
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UComboBoxString> SkillCombo = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UEditableTextBox> NameBox = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UTextBlock> SummaryText = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UComboBoxString> ModuleTypeCombo = nullptr;

    // DetailsView는 UMG 디자이너/MCP 팔레트에서 직접 추가가 까다로워, 디자이너엔 배치용 박스만 두고
    // 실제 DetailsView는 NativeConstruct에서 생성해 그 박스에 넣는다(없으면 RootScroll 폴백).
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UDetailsView> SkillDetails = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UDetailsView> GADetails = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UPanelWidget> ModuleDetailsBox = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UPanelWidget> GADetailsBox = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UTextBlock> GASInfoText = nullptr;

    // 스킬 버튼
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> NewButton = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> DuplicateButton = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> DeleteButton = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> OpenButton = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> SaveButton = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> RefreshButton = nullptr;

    // 프리뷰 버튼
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> PreviewButton = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> StopButton = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> OpenLevelButton = nullptr;

    // 모듈 추가
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> AddModuleButton = nullptr;

    // 접이식 섹션 헤더 버튼 + body(아무 패널)
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> SkillHeaderButton = nullptr;
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UWidget> SkillBody = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> PreviewHeaderButton = nullptr;
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UWidget> PreviewBody = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> ModuleHeaderButton = nullptr;
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UWidget> ModuleBody = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> GAHeaderButton = nullptr;
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UWidget> GABody = nullptr;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UButton> GASInfoHeaderButton = nullptr;
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
    TObjectPtr<UWidget> GASInfoBody = nullptr;

    // ── 로직 상태 ─────────────────────────────────────────────────
    UPROPERTY()
    TArray<TObjectPtr<USkillDefinition>> Skills;

    UPROPERTY()
    TObjectPtr<USkillDefinition> SelectedSkill = nullptr;

    bool bEventsBound = false;
};

#include "EUW_SkillForge.h"
#include "SkillForgeLibrary.h"
#include "Skills/SkillDefinition.h"
#include "Skills/EffectModule.h"
#include "Components/DetailsView.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

void UEUW_SkillForge::EnsureDetailsViews()
{
    if (WidgetTree == nullptr)
    {
        return;
    }

    if (SkillDetails == nullptr)
    {
        SkillDetails = WidgetTree->ConstructWidget<UDetailsView>(UDetailsView::StaticClass());
        UPanelWidget* TargetPanel = (ModuleDetailsBox != nullptr) ? ModuleDetailsBox.Get() : Cast<UPanelWidget>(GetRootWidget());
        if (TargetPanel != nullptr && SkillDetails != nullptr)
        {
            TargetPanel->AddChild(SkillDetails);
        }
    }

    if (GADetails == nullptr)
    {
        GADetails = WidgetTree->ConstructWidget<UDetailsView>(UDetailsView::StaticClass());
        UPanelWidget* TargetPanel = (GADetailsBox != nullptr) ? GADetailsBox.Get() : Cast<UPanelWidget>(GetRootWidget());
        if (TargetPanel != nullptr && GADetails != nullptr)
        {
            TargetPanel->AddChild(GADetails);
        }
    }
}

void UEUW_SkillForge::ApplyStyling()
{
    // 섹션 헤더(접기/펴기) = 남색 / 일반 버튼 = 중성 회색 / 의미별 강조색
    const FLinearColor HeaderColor(0.10f, 0.22f, 0.42f, 1.f);
    const FLinearColor ActionColor(0.20f, 0.22f, 0.26f, 1.f);
    const FLinearColor DangerColor(0.45f, 0.13f, 0.13f, 1.f);
    const FLinearColor PlayColor(0.13f, 0.36f, 0.20f, 1.f);
    const FLinearColor StopColor(0.42f, 0.24f, 0.09f, 1.f);

    auto Tint = [](UButton* B, const FLinearColor& C)
    {
        if (B != nullptr)
        {
            B->SetBackgroundColor(C);
        }
    };

    // 접이식 섹션 헤더
    Tint(SkillHeaderButton, HeaderColor);
    Tint(PreviewHeaderButton, HeaderColor);
    Tint(ModuleHeaderButton, HeaderColor);
    Tint(GAHeaderButton, HeaderColor);
    Tint(GASInfoHeaderButton, HeaderColor);

    // 일반/의미 버튼
    Tint(NewButton, ActionColor);
    Tint(DuplicateButton, ActionColor);
    Tint(DeleteButton, DangerColor);
    Tint(OpenButton, ActionColor);
    Tint(SaveButton, ActionColor);
    Tint(RefreshButton, ActionColor);
    Tint(PreviewButton, PlayColor);
    Tint(StopButton, StopColor);
    Tint(OpenLevelButton, ActionColor);
    Tint(AddModuleButton, ActionColor);

    // 새 이름 입력칸을 더 크게 — 가로로 꽉 채우고 최소 폭 확보
    if (NameBox != nullptr)
    {
        NameBox->SetMinDesiredWidth(280.f);
        if (UHorizontalBoxSlot* NameSlot = Cast<UHorizontalBoxSlot>(NameBox->Slot))
        {
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            NameSlot->SetPadding(FMargin(4.f, 2.f));
        }
    }
}

void UEUW_SkillForge::NativeConstruct()
{
    Super::NativeConstruct();

    // 디자이너에 DetailsView가 없으면 코드로 만들어 배치용 박스에 넣음
    EnsureDetailsViews();

    // 버튼 색 구분 + 입력칸 크기
    ApplyStyling();

    // 모듈 타입 콤보 옵션(비어 있으면 채움)
    if (ModuleTypeCombo != nullptr && ModuleTypeCombo->GetOptionCount() == 0)
    {
        ModuleTypeCombo->AddOption(TEXT("Damage"));
        ModuleTypeCombo->AddOption(TEXT("Pull"));
        ModuleTypeCombo->AddOption(TEXT("Push"));
        ModuleTypeCombo->AddOption(TEXT("Stun"));
        ModuleTypeCombo->AddOption(TEXT("Slow"));
        ModuleTypeCombo->SetSelectedIndex(0);
    }

    // DetailsView 카테고리 필터 — 모듈 배열 / GA 속성 분리
    if (SkillDetails != nullptr)
    {
        SkillDetails->CategoriesToShow.Empty();
        SkillDetails->CategoriesToShow.Add(FName(TEXT("Modules")));
    }
    if (GADetails != nullptr)
    {
        GADetails->CategoriesToShow.Empty();
        const TCHAR* GACats[] = { TEXT("Display"), TEXT("Delivery"), TEXT("Cast"), TEXT("Duration"),
            TEXT("Shape"), TEXT("Rain"), TEXT("VFX"), TEXT("Projectile") };
        for (const TCHAR* Cat : GACats)
        {
            GADetails->CategoriesToShow.Add(FName(Cat));
        }
    }

    // 이벤트 바인딩(한 번만 — 재구성 시 중복 방지)
    if (bEventsBound == false)
    {
        bEventsBound = true;

        if (SkillCombo != nullptr)
        {
            SkillCombo->OnSelectionChanged.AddDynamic(this, &UEUW_SkillForge::OnComboChanged);
        }
        if (NewButton != nullptr)        { NewButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnNewClicked); }
        if (DuplicateButton != nullptr)  { DuplicateButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnDuplicateClicked); }
        if (DeleteButton != nullptr)     { DeleteButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnDeleteClicked); }
        if (OpenButton != nullptr)       { OpenButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnOpenClicked); }
        if (SaveButton != nullptr)       { SaveButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnSaveClicked); }
        if (RefreshButton != nullptr)    { RefreshButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnRefreshClicked); }
        if (PreviewButton != nullptr)    { PreviewButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnPreviewClicked); }
        if (StopButton != nullptr)       { StopButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnStopClicked); }
        if (OpenLevelButton != nullptr)  { OpenLevelButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnOpenPreviewLevelClicked); }
        if (AddModuleButton != nullptr)  { AddModuleButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnAddModuleClicked); }

        if (SkillHeaderButton != nullptr)   { SkillHeaderButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnToggleSkill); }
        if (PreviewHeaderButton != nullptr) { PreviewHeaderButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnTogglePreview); }
        if (ModuleHeaderButton != nullptr)  { ModuleHeaderButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnToggleModule); }
        if (GAHeaderButton != nullptr)      { GAHeaderButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnToggleGA); }
        if (GASInfoHeaderButton != nullptr) { GASInfoHeaderButton->OnClicked.AddDynamic(this, &UEUW_SkillForge::OnToggleGASInfo); }
    }

    RefreshList();
}

void UEUW_SkillForge::ToggleSection(UWidget* Body)
{
    if (Body == nullptr)
    {
        return;
    }
    const bool bVisible = Body->GetVisibility() != ESlateVisibility::Collapsed;
    Body->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UEUW_SkillForge::OnToggleSkill()    { ToggleSection(SkillBody); }
void UEUW_SkillForge::OnTogglePreview()  { ToggleSection(PreviewBody); }
void UEUW_SkillForge::OnToggleModule()   { ToggleSection(ModuleBody); }
void UEUW_SkillForge::OnToggleGA()       { ToggleSection(GABody); }
void UEUW_SkillForge::OnToggleGASInfo()  { ToggleSection(GASInfoBody); }

void UEUW_SkillForge::RefreshList()
{
    if (SkillCombo != nullptr)
    {
        SkillCombo->ClearOptions();
    }
    Skills.Reset();

    TArray<USkillDefinition*> Found = USkillForgeLibrary::GetAllSkills();
    for (USkillDefinition* Skill : Found)
    {
        Skills.Add(Skill);
        if (SkillCombo != nullptr)
        {
            SkillCombo->AddOption(USkillForgeLibrary::GetSkillSummary(Skill));
        }
    }

    if (Skills.Num() > 0)
    {
        if (SkillCombo != nullptr)
        {
            SkillCombo->SetSelectedIndex(0);
        }
        SelectedSkill = Skills[0];
    }
    else
    {
        SelectedSkill = nullptr;
    }

    UpdateSummary();
}

void UEUW_SkillForge::UpdateSummary()
{
    if (SummaryText != nullptr)
    {
        SummaryText->SetText(FText::FromString(USkillForgeLibrary::GetSkillSummary(SelectedSkill)));
    }
    if (GASInfoText != nullptr)
    {
        GASInfoText->SetText(FText::FromString(USkillForgeLibrary::GetGASInfo(SelectedSkill)));
    }
    RefreshModuleList();
}

void UEUW_SkillForge::SelectSkill(USkillDefinition* Skill)
{
    SelectedSkill = Skill;
    const int32 Index = Skills.IndexOfByKey(Skill);
    if (Index != INDEX_NONE && SkillCombo != nullptr)
    {
        SkillCombo->SetSelectedIndex(Index);
    }
    UpdateSummary();
}

void UEUW_SkillForge::OnComboChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SkillCombo == nullptr)
    {
        return;
    }
    const int32 Index = SkillCombo->GetSelectedIndex();
    if (Skills.IsValidIndex(Index))
    {
        SelectedSkill = Skills[Index];
    }
    UpdateSummary();
}

void UEUW_SkillForge::OnNewClicked()
{
    const FString Name = (NameBox != nullptr) ? NameBox->GetText().ToString() : FString();
    USkillDefinition* NewSkill = USkillForgeLibrary::CreateSkill(Name);
    RefreshList();
    if (NewSkill != nullptr)
    {
        SelectSkill(NewSkill);
        USkillForgeLibrary::OpenSkillInEditor(NewSkill);
    }
}

void UEUW_SkillForge::OnDuplicateClicked()
{
    if (SelectedSkill == nullptr)
    {
        return;
    }
    const FString Name = (NameBox != nullptr) ? NameBox->GetText().ToString() : FString();
    USkillDefinition* Dup = USkillForgeLibrary::DuplicateSkill(SelectedSkill, Name);
    RefreshList();
    if (Dup != nullptr)
    {
        SelectSkill(Dup);
    }
}

void UEUW_SkillForge::OnDeleteClicked()
{
    if (SelectedSkill == nullptr)
    {
        return;
    }
    USkillForgeLibrary::DeleteSkill(SelectedSkill);
    SelectedSkill = nullptr;
    RefreshList();
}

void UEUW_SkillForge::OnOpenClicked()
{
    USkillForgeLibrary::OpenSkillInEditor(SelectedSkill);
}

void UEUW_SkillForge::OnSaveClicked()
{
    USkillForgeLibrary::SaveSkill(SelectedSkill);
}

void UEUW_SkillForge::OnRefreshClicked()
{
    RefreshList();
}

void UEUW_SkillForge::OnPreviewClicked()
{
    if (SelectedSkill != nullptr)
    {
        USkillForgeLibrary::PreviewSkill(SelectedSkill);
    }
}

void UEUW_SkillForge::OnStopClicked()
{
    // 초기화: 진행 중 프리뷰(액터/데모큐브/VFX) 제거 + 시전자/타격 큐브를 원위치로
    USkillForgeLibrary::StopPreview();

    if (GEditor == nullptr)
    {
        return;
    }
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (World == nullptr)
    {
        return;
    }

    TArray<AActor*> Props;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* A = *It;
        if (A == nullptr)
        {
            continue;
        }
        if (A->ActorHasTag(TEXT("SkillPreviewCaster")))
        {
            A->SetActorLocation(FVector(0.f, 0.f, 50.f));
        }
        else if (A->ActorHasTag(TEXT("SkillPreviewTarget")))
        {
            A->SetActorLocation(FVector(1000.f, 0.f, 50.f));
        }
        else if (A->ActorHasTag(TEXT("SkillPreviewProp")))
        {
            Props.Add(A);
        }
    }

    // 데모 큐브를 타격 기준 링 위치로 복원(개수 무관, 큐브는 동일하므로 순서 무관)
    const FVector RingCenter(1000.f, 0.f, 50.f);
    const float RingRadius = 300.f;
    const int32 Num = Props.Num();
    for (int32 i = 0; i < Num; ++i)
    {
        const float Ang = (2.f * PI) * i / FMath::Max(1, Num);
        const FVector P = RingCenter + FVector(FMath::Cos(Ang) * RingRadius, FMath::Sin(Ang) * RingRadius, 0.f);
        if (Props[i] != nullptr)
        {
            Props[i]->SetActorLocation(P);
        }
    }
}

void UEUW_SkillForge::OnOpenPreviewLevelClicked()
{
    USkillForgeLibrary::OpenPreviewLevel();
}

void UEUW_SkillForge::OnAddModuleClicked()
{
    if (SelectedSkill == nullptr || ModuleTypeCombo == nullptr)
    {
        return;
    }

    const int32 TypeIndex = ModuleTypeCombo->GetSelectedIndex();
    if (TypeIndex < 0)
    {
        return;
    }

    USkillForgeLibrary::AddModule(SelectedSkill, static_cast<ESkillModuleType>(TypeIndex));
    UpdateSummary();
}

void UEUW_SkillForge::RefreshModuleList()
{
    // SetObject는 같은 오브젝트면 무시 → null로 비웠다 다시 세팅해 강제 재구성(배열 변경 반영)
    if (SkillDetails != nullptr)
    {
        SkillDetails->SetObject(nullptr);
        SkillDetails->SetObject(SelectedSkill);
    }
    if (GADetails != nullptr)
    {
        GADetails->SetObject(nullptr);
        GADetails->SetObject(SelectedSkill);
    }
}

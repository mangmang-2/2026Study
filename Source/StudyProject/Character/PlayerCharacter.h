#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterBase.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UTradeComponent;
class UHUDWidget;
class UInteractionDetectorComponent;
class UDialogueWidget;
class ULockOnComponent;

UCLASS()
class STUDYPROJECT_API APlayerCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    APlayerCharacter();

    UFUNCTION(BlueprintCallable, Category = "Components")
    UTradeComponent* GetTradeComponent() const { return TradeComp; }

    UFUNCTION(BlueprintCallable, Category = "Components")
    ULockOnComponent* GetLockOnComponent() const { return LockOnComp; }

    // 이동 입력 원본값(X=좌우, Y=전후). 이동이 잠긴 콤보 중에도 갱신됨(키 눌림 감지용).
    UFUNCTION(BlueprintCallable, Category = "Input")
    FVector2D GetMoveInput() const { return LastMoveInput; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UHUDWidget* GetHUDWidget() const { return HUDWidget; }

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void StartDialogue(int32 InDialogueID, AActor* Interactor);

    // NPC 상호작용(F)에서 호출 — 서비스 화면 열기
    void OpenShopScreen(int32 ShopID);
    void OpenEnhanceScreen();

    // 콘솔 커맨드: DebugDropItem <ItemID> <Quantity>
    UFUNCTION(Exec)
    void DebugDropItem(int32 ItemID, int32 Quantity = 1);

    // 콘솔 커맨드: 가장 가까운 다른 플레이어에게 거래 요청(2인 테스트용)
    UFUNCTION(Exec)
    void DebugTradeNearest();

protected:
    // ── 카메라 ────────────────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    // ── 입력 컨텍스트 ─────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> UIMappingContext;

    // ── 입력 액션 (구현됨) ───────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> InventoryAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> PauseAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> UsePotionAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> EnhanceAction;   // 임시 테스트용 단축키 (K)

    // ── 입력 액션 (2단계용 — 빈 슬롯) ──────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "Input|Phase2")
    TObjectPtr<UInputAction> AttackAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Phase2")
    TObjectPtr<UInputAction> HeavyAttackAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Phase2")
    TObjectPtr<UInputAction> DodgeAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Phase2")
    TObjectPtr<UInputAction> LockOnAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Phase2")
    TObjectPtr<UInputAction> LockOnSwitchAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input|Phase2")
    TObjectPtr<UInputAction> FinisherAction;

    // ── UI 위젯 클래스 ────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UHUDWidget> HUDWidgetClass;

    // SP/HP 바 위젯(WBP_SPBar). BeginPlay에서 뷰포트에 추가
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> SPBarWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> InventoryScreenWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PauseMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UDialogueWidget> DialogueWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> EnhanceScreenWidgetClass;   // 임시 테스트용

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> ShopScreenWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> TradeScreenWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> InteractionPromptClass;   // 머리 위 F 프롬프트 (HUD-follow)

    // ── 컴포넌트 ─────────────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTradeComponent> TradeComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UInteractionDetectorComponent> InteractionDetector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<ULockOnComponent> LockOnComp;

    // ── 상태 ─────────────────────────────────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsSprinting = false;

    // 마지막 이동 입력값(콤보 스텝인 등에서 전진키 눌림 판정용)
    FVector2D LastMoveInput = FVector2D::ZeroVector;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void NotifyControllerChanged() override;

private:
    // 입력 핸들러
    void HandleMove(const FInputActionValue& Value);
    void HandleMoveCompleted(const FInputActionValue& Value);
    void HandleLook(const FInputActionValue& Value);
    void HandleJump();
    void HandleStopJump();
    void HandleInteract();
    void HandleInventory();
    void HandlePause();
    void HandleSprintStart();
    void HandleSprintEnd();
    void HandleUsePotion();
    void HandleEnhance();

    // GAS 어빌리티 입력 — 입력 태그(Input.*)로 매칭되는 GA 활성화
    void HandleAttack();   // 지상/공중 자동 분기
    void HandleLauncher(); // 공중 띄우기
    void HandleFinisher(); // 처형
    void HandleDodge();
    void HandleLockOn();   // 록온 토글
    void HandleLockOnSwitch(const FInputActionValue& Value); // 타겟 좌우 전환

    // UI 열기/닫기
    void OpenInventory();
    UFUNCTION() void CloseInventory();
    void OpenPauseMenu();
    void ClosePauseMenu();
    void OpenEnhance();
    UFUNCTION() void CloseEnhance();
    void OpenShop(int32 ShopID);
    UFUNCTION() void CloseShop();
    void OpenTradeScreen();
    void CloseTradeScreen();
    UFUNCTION() void HandleTradeUpdated();
    UFUNCTION() void HandleTradeResult(bool bSuccess);
    void SwitchToUIInput();
    void SwitchToGameInput();

    UPROPERTY()
    TObjectPtr<UHUDWidget> HUDWidget             = nullptr;
    UPROPERTY()
    TObjectPtr<UUserWidget> SPBarWidget          = nullptr;
    UPROPERTY() 
    TObjectPtr<UUserWidget> InventoryScreenWidget = nullptr;

    UPROPERTY() 
    TObjectPtr<UUserWidget> PauseMenuWidget       = nullptr;

    UPROPERTY()
    TObjectPtr<UDialogueWidget> DialogueWidget        = nullptr;

    UPROPERTY()
    TObjectPtr<UUserWidget> EnhanceScreenWidget   = nullptr;

    UPROPERTY()
    TObjectPtr<UUserWidget> ShopScreenWidget      = nullptr;

    UPROPERTY()
    TObjectPtr<UUserWidget> TradeScreenWidget     = nullptr;

    bool bInventoryOpen  = false;
    bool bPauseMenuOpen  = false;
    bool bEnhanceOpen    = false;
    bool bShopOpen       = false;
    bool bTradeOpen      = false;

    UFUNCTION(Server, Reliable)
    void Server_DebugDropItem(int32 ItemID, int32 Quantity);

    UPROPERTY() 
    TObjectPtr<AActor> LastFocusedActor = nullptr;

    UFUNCTION()
    void OnFocusChanged(AActor* NewFocus);

    // 머리 위 프롬프트 위젯 (뷰포트에 띄우고 NPC 위치를 매 프레임 추적)
    UPROPERTY()
    TObjectPtr<UUserWidget> InteractionPromptW = nullptr;

    void UpdateInteractionPrompt();
};

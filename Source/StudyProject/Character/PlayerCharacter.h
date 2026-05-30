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

UCLASS()
class STUDYPROJECT_API APlayerCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    APlayerCharacter();

    UFUNCTION(BlueprintCallable, Category = "Components")
    UTradeComponent* GetTradeComponent() const { return TradeComp; }

    UFUNCTION(BlueprintCallable, Category = "UI")
    UHUDWidget* GetHUDWidget() const { return HUDWidget; }

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void StartDialogue(int32 InDialogueID, AActor* Interactor);

    // 콘솔 커맨드: DebugDropItem <ItemID> <Quantity>
    UFUNCTION(Exec)
    void DebugDropItem(int32 ItemID, int32 Quantity = 1);

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

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> InventoryScreenWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PauseMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UDialogueWidget> DialogueWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> EnhanceScreenWidgetClass;   // 임시 테스트용

    // ── 컴포넌트 ─────────────────────────────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UTradeComponent> TradeComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UInteractionDetectorComponent> InteractionDetector;

    // ── 상태 ─────────────────────────────────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsSprinting = false;

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void NotifyControllerChanged() override;

private:
    // 입력 핸들러
    void HandleMove(const FInputActionValue& Value);
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

    // UI 열기/닫기
    void OpenInventory();
    void CloseInventory();
    void OpenPauseMenu();
    void ClosePauseMenu();
    void OpenEnhance();
    void CloseEnhance();
    void SwitchToUIInput();
    void SwitchToGameInput();

    UPROPERTY() 
    TObjectPtr<UHUDWidget> HUDWidget             = nullptr;
    UPROPERTY() 
    TObjectPtr<UUserWidget> InventoryScreenWidget = nullptr;

    UPROPERTY() 
    TObjectPtr<UUserWidget> PauseMenuWidget       = nullptr;

    UPROPERTY()
    TObjectPtr<UDialogueWidget> DialogueWidget        = nullptr;

    UPROPERTY()
    TObjectPtr<UUserWidget> EnhanceScreenWidget   = nullptr;

    bool bInventoryOpen  = false;
    bool bPauseMenuOpen  = false;
    bool bEnhanceOpen    = false;

    UFUNCTION(Server, Reliable)
    void Server_DebugDropItem(int32 ItemID, int32 Quantity);

    UPROPERTY() 
    TObjectPtr<AActor> LastFocusedActor = nullptr;

    UFUNCTION()
    void OnFocusChanged(AActor* NewFocus);
};

// 第一人稱玩家角色：Enhanced Input 綁定（移動/視角/疾跑/手電筒/互動）、SpotLight 手電筒微閃爍、每幀互動射線。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HVPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpotLightComponent;
struct FInputActionValue;

/**
 * 林秀蘭事件的調查者（第一人稱）。
 * - Enhanced Input：Move / Look / Sprint / Flashlight / Interact 五個 InputAction。
 * - 手電筒：掛在相機下的 SpotLight，Tick 以 Perlin 噪聲做微閃爍，偶發短暫壓暗。
 * - 互動：每幀自相機發出 LineTrace，焦點變化時透過 GameMode 廣播 UI 提示。
 */
UCLASS()
class HOTELVICTORIA_API AHVPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHVPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "HV|Flashlight")
	bool IsFlashlightOn() const { return bFlashlightOn; }

	UFUNCTION(BlueprintCallable, Category = "HV|Flashlight")
	void SetFlashlightOn(bool bOn);

	UFUNCTION(BlueprintPure, Category = "HV|Camera")
	UCameraComponent* GetFirstPersonCamera() const { return Camera; }

protected:
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ---- Enhanced Input 處理函式 ----
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleSprintStart(const FInputActionValue& Value);
	void HandleSprintStop(const FInputActionValue& Value);
	void HandleToggleFlashlight(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);

	// ---- 每幀子流程 ----
	void UpdateFlashlightFlicker(float DeltaSeconds);
	void UpdateInteractionTrace();

	// ---- 元件 ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Flashlight")
	TObjectPtr<USpotLightComponent> Flashlight;

	// ---- 輸入資產（在 BP_HVPlayer 指定 IMC / IA 資產）----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Input")
	TObjectPtr<UInputAction> FlashlightAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Input")
	TObjectPtr<UInputAction> InteractAction;

	// ---- 移動參數 ----
	UPROPERTY(EditDefaultsOnly, Category = "HV|Movement", meta = (ClampMin = "50.0"))
	float WalkSpeed = 240.f;

	UPROPERTY(EditDefaultsOnly, Category = "HV|Movement", meta = (ClampMin = "50.0"))
	float SprintSpeed = 430.f;

	// ---- 手電筒參數 ----
	UPROPERTY(EditDefaultsOnly, Category = "HV|Flashlight", meta = (ClampMin = "0.0"))
	float FlashlightBaseIntensity = 5000.f;

	/** 微閃爍幅度（0~1，基礎強度的比例）。 */
	UPROPERTY(EditDefaultsOnly, Category = "HV|Flashlight", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerAmount = 0.08f;

	/** 微閃爍頻率（Perlin 噪聲取樣速度）。 */
	UPROPERTY(EditDefaultsOnly, Category = "HV|Flashlight", meta = (ClampMin = "0.1"))
	float FlickerFrequency = 9.f;

	// ---- 互動參數 ----
	UPROPERTY(EditDefaultsOnly, Category = "HV|Interact", meta = (ClampMin = "50.0"))
	float InteractDistance = 250.f;

private:
	bool bFlashlightOn = true;
	float FlickerSeed = 0.f;

	/** 目前射線焦點中的可互動 Actor（弱參照，避免懸空）。 */
	TWeakObjectPtr<AActor> FocusedActor;
};

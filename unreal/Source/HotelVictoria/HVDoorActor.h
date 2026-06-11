// 門 Actor：可鎖定，開關以 Timeline + 浮點曲線驅動門板 Yaw 旋轉；支援 1013 房事件解鎖。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HVInteractable.h"
#include "HVDoorActor.generated.h"

class UCurveFloat;
class UStaticMeshComponent;
class UTimelineComponent;

/**
 * 飯店房門 / 樓層門。
 * - bLocked = true 時互動只顯示鎖住提示。
 * - 開關動畫：OpenCurve（0→1）經 Timeline 取樣，插值門板 Yaw 至 OpenAngle。
 * - bIsRoom1013Door = true 的門在 GameMode 廣播 OnRoom1013Unlocked 後自動解鎖並顯形。
 */
UCLASS()
class HOTELVICTORIA_API AHVDoorActor : public AActor, public IHVInteractable
{
	GENERATED_BODY()

public:
	AHVDoorActor();

	// ---- IHVInteractable ----
	virtual void Interact_Implementation(AHVPlayerCharacter* Interactor) override;
	virtual FText GetInteractHint_Implementation() override;
	virtual bool CanInteract_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "HV|Door")
	void SetLocked(bool bInLocked) { bLocked = bInLocked; }

	UFUNCTION(BlueprintPure, Category = "HV|Door")
	bool IsLocked() const { return bLocked; }

	UFUNCTION(BlueprintPure, Category = "HV|Door")
	bool IsOpen() const { return bIsOpen; }

	/** 直接開/關（含動畫）；給關卡藍圖或女鬼事件用。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Door")
	void SetDoorOpen(bool bOpen);

protected:
	virtual void BeginPlay() override;

	/** Timeline 每幀回呼：Alpha 0~1 插值門板 Yaw。 */
	UFUNCTION()
	void HandleOpenProgress(float Alpha);

	/** Timeline 播畢回呼：解除運轉中旗標。 */
	UFUNCTION()
	void HandleOpenFinished();

	/** GameMode 1013 解鎖廣播回呼。 */
	UFUNCTION()
	void HandleRoom1013Unlocked();

	/** 上鎖時互動的藍圖掛點（搖門把音效、輕微鏡頭抖動）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HV|Door")
	void OnLockedInteract();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Door")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Door")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	/** 門板。樞紐技巧：把 Mesh 的 Relative Location 沿 Y 偏移半個門寬，旋轉自然繞門軸。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Door")
	TObjectPtr<UTimelineComponent> OpenTimeline;

	/** 開門曲線（0→1，建議 0.8 秒 EaseOut）；未指定時直接瞬間開關。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Door")
	TObjectPtr<UCurveFloat> OpenCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Door")
	float OpenAngle = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Door")
	bool bLocked = false;

	/** true = 此門是 1013 房門：初始隱藏＋上鎖，集滿證據後浮現。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Door")
	bool bIsRoom1013Door = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Door")
	FText LockedHint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Door")
	FText OpenHint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Door")
	FText CloseHint;

private:
	bool bIsOpen = false;
	bool bAnimating = false;
	float ClosedYaw = 0.f;
};

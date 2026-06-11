// 女鬼角色：SkeletalMesh 佔位、姿態切換、溶解材質淡入淡出、曲線參數化位移；BP 可覆寫演出。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HVTypes.h"
#include "HVGhostCharacter.generated.h"

class UCurveFloat;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHVOnGhostFadeOutFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHVOnGhostMoveFinished);

/**
 * 林秀蘭（女鬼）。永不傷害玩家——膠囊體不擋人、不觸發傷害。
 * - GhostMode：供 UHVGhostAnimInstance 讀取以切換動畫狀態機。
 * - 溶解：BeginPlay 為所有材質建立 MID，Tick 將 "Dissolve" 參數朝目標值推進（1=全消散、0=全顯形）。
 * - 位移：StartParametricMove 以 0→1 曲線（可選 Ease 曲線）插值世界座標，供導演驅動爬行/經過等運動。
 */
UCLASS()
class HOTELVICTORIA_API AHVGhostCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHVGhostCharacter();

	virtual void Tick(float DeltaSeconds) override;

	/** 切換姿態：更新 AnimInstance 讀取值，倒吊/天花板姿態自動翻轉 Mesh。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void SetGhostMode(EHVGhostMode NewMode);

	UFUNCTION(BlueprintPure, Category = "HV|Ghost")
	EHVGhostMode GetGhostMode() const { return GhostMode; }

	/** 自全溶解狀態淡入顯形。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void FadeIn(float Duration = 0.8f);

	/** 淡出消散；完成時廣播 OnFadeOutFinished（導演據此回收）。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void FadeOut(float Duration = 0.8f);

	/** 曲線參數化直線位移：Duration 秒內由 From 移到 To，EaseCurve 為 null 時線性。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void StartParametricMove(const FVector& From, const FVector& To, float Duration, UCurveFloat* EaseCurve);

	/** 以 Yaw 朝向玩家相機（凝視類事件用）。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void FacePlayer();

	UFUNCTION(BlueprintPure, Category = "HV|Ghost")
	float GetCurrentMoveSpeed() const { return CurrentMoveSpeed; }

	UPROPERTY(BlueprintAssignable, Category = "HV|Ghost")
	FHVOnGhostFadeOutFinished OnFadeOutFinished;

	UPROPERTY(BlueprintAssignable, Category = "HV|Ghost")
	FHVOnGhostMoveFinished OnMoveFinished;

protected:
	virtual void BeginPlay() override;

	/** 姿態切換的藍圖掛點：BP 子類可在此調整 Mesh 偏移、播放一次性 Montage、開關音效。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HV|Ghost")
	void OnGhostModeChanged(EHVGhostMode NewMode);

	/** 溶解材質純量參數名（Mixamo 佔位材質與正式溶解材質都需含此參數）。 */
	UPROPERTY(EditDefaultsOnly, Category = "HV|Ghost")
	FName DissolveParamName = TEXT("Dissolve");

private:
	void UpdateDissolve(float DeltaSeconds);
	void UpdateParametricMove(float DeltaSeconds);
	void ApplyDissolveValue();

	/** AnimInstance 透過 GetGhostMode() 讀取。 */
	EHVGhostMode GhostMode = EHVGhostMode::Hidden;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DissolveMIDs;

	// 溶解狀態（1 = 完全消散不可見、0 = 完全顯形）
	float DissolveValue = 1.f;
	float DissolveTarget = 1.f;
	float DissolveSpeed = 1.f;
	bool bFadingOut = false;

	// 參數化位移狀態
	bool bMoving = false;
	FVector MoveFrom = FVector::ZeroVector;
	FVector MoveTo = FVector::ZeroVector;
	float MoveDuration = 1.f;
	float MoveElapsed = 0.f;
	float CurrentMoveSpeed = 0.f;

	UPROPERTY()
	TObjectPtr<UCurveFloat> MoveEaseCurve;

	// Mesh 預設相對位置/旋轉（倒吊姿態翻轉後復原用）
	FVector DefaultMeshRelLoc = FVector::ZeroVector;
	FRotator DefaultMeshRelRot = FRotator::ZeroRotator;
};

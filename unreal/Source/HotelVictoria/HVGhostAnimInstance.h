// 女鬼 AnimInstance：每幀同步 GhostMode 與移動速度，AnimBP 狀態機據此切換爬行/倒吊/行走等分支。
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HVTypes.h"
#include "HVGhostAnimInstance.generated.h"

/**
 * 女鬼動畫實例 C++ 基類。
 * AnimBP 以此為父類後可直接在 AnimGraph 讀取 GhostMode / MoveSpeed，
 * 對應 Mixamo 匯入的爬行、倒吊（Control Rig 調整）、反折行走等動畫分支。
 */
UCLASS()
class HOTELVICTORIA_API UHVGhostAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 當前姿態（驅動狀態機分支）。 */
	UPROPERTY(BlueprintReadOnly, Category = "HV|Ghost")
	EHVGhostMode GhostMode = EHVGhostMode::Hidden;

	/** 參數化位移的瞬時速度（cm/s），供爬行/行走混合空間取用。 */
	UPROPERTY(BlueprintReadOnly, Category = "HV|Ghost")
	float MoveSpeed = 0.f;

	/** 是否在移動中。 */
	UPROPERTY(BlueprintReadOnly, Category = "HV|Ghost")
	bool bIsMoving = false;
};

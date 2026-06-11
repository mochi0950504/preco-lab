// 證據 Actor：可閱讀文件（登記簿/存根/懺悔信/剪報/日記），讀畢收取並通知 GameMode。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HVInteractable.h"
#include "HVEvidenceActor.generated.h"

class UStaticMeshComponent;

/**
 * 五份證據的共用基類。
 * 互動 → GameMode 開啟閱讀 Widget → 關閉時 MarkCollected()：
 * 通知 GameMode::RegisterEvidence、隱藏自身並停用碰撞。
 */
UCLASS()
class HOTELVICTORIA_API AHVEvidenceActor : public AActor, public IHVInteractable
{
	GENERATED_BODY()

public:
	AHVEvidenceActor();

	// ---- IHVInteractable ----
	virtual void Interact_Implementation(AHVPlayerCharacter* Interactor) override;
	virtual FText GetInteractHint_Implementation() override;
	virtual bool CanInteract_Implementation() override;

	/** 閱讀結束後由 GameMode 呼叫：登記證據、隱藏自身。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Evidence")
	void MarkCollected();

	UFUNCTION(BlueprintPure, Category = "HV|Evidence")
	FName GetEvidenceId() const { return EvidenceId; }

	UFUNCTION(BlueprintPure, Category = "HV|Evidence")
	FText GetEvidenceTitle() const { return EvidenceTitle; }

	UFUNCTION(BlueprintPure, Category = "HV|Evidence")
	const TArray<FText>& GetPages() const { return Pages; }

	UFUNCTION(BlueprintPure, Category = "HV|Evidence")
	bool IsCollected() const { return bCollected; }

protected:
	/** 收取瞬間的藍圖掛點（音效、粒子、HUD 動畫）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HV|Evidence")
	void OnCollected();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Evidence")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	/** 唯一識別（如 Evidence.Register1F / Evidence.Pawnshop3F …）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Evidence")
	FName EvidenceId;

	/** 閱讀視窗標題（如「1F 辦公室登記簿」）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Evidence")
	FText EvidenceTitle;

	/** 閱讀文本，逐頁顯示。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Evidence", meta = (MultiLine = "true"))
	TArray<FText> Pages;

	/** 準心提示文字。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Evidence")
	FText InteractHint;

private:
	bool bCollected = false;
};

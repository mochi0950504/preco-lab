// 遊戲主流程 GameMode：流程狀態機（標題/運鏡/探索/閱讀/終章）、證據計數、1013 房解鎖、UI 廣播。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HVTypes.h"
#include "HVGameMode.generated.h"

class AHVEvidenceActor;
class UHVReadingWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHVOnGameStateChanged, EHVGameState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHVOnEvidenceChanged, int32, Collected, int32, Required);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHVOnRoom1013Unlocked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHVOnInteractHintChanged, bool, bVisible, const FText&, HintText);

/**
 * 《十樓的她》主 GameMode。
 * - 持有遊戲流程狀態並以委派廣播給 HUD / 關卡藍圖。
 * - 收集 5 份證據（1F 登記簿、3F 當舖存根、5F 懺悔信、7F 剪報、9F 日記）後解鎖 1013 房。
 * - 進入 Reading 狀態時建立閱讀 Widget 並切換輸入模式。
 */
UCLASS()
class HOTELVICTORIA_API AHVGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHVGameMode();

	// ---- 流程狀態 ----

	/** 切換流程狀態並廣播；同時依狀態啟停女鬼導演。 */
	UFUNCTION(BlueprintCallable, Category = "HV|State")
	void SetGameState(EHVGameState NewState);

	UFUNCTION(BlueprintPure, Category = "HV|State")
	EHVGameState GetGameState() const { return CurrentState; }

	/** 開場 Level Sequence 播放完畢後由關卡藍圖呼叫，進入自由探索。 */
	UFUNCTION(BlueprintCallable, Category = "HV|State")
	void StartPlay1972() { SetGameState(EHVGameState::Play); }

	/** 走進 1013 房交還真相時由觸發器呼叫，進入終章。 */
	UFUNCTION(BlueprintCallable, Category = "HV|State")
	void StartFinale();

	// ---- 證據 ----

	/** 登記一份證據；重複 Id 會被忽略。集滿 RequiredEvidenceCount 即解鎖 1013。回傳是否為新證據。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Evidence")
	bool RegisterEvidence(FName EvidenceId);

	UFUNCTION(BlueprintPure, Category = "HV|Evidence")
	bool HasEvidence(FName EvidenceId) const { return CollectedEvidenceIds.Contains(EvidenceId); }

	UFUNCTION(BlueprintPure, Category = "HV|Evidence")
	int32 GetEvidenceCount() const { return CollectedEvidenceIds.Num(); }

	UFUNCTION(BlueprintPure, Category = "HV|Evidence")
	bool IsRoom1013Unlocked() const { return bRoom1013Unlocked; }

	// ---- 閱讀流程 ----

	/** 玩家對證據互動時呼叫：開啟閱讀 Widget、切到 Reading 狀態；關閉時自動收取證據。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Reading")
	void BeginEvidenceReading(AHVEvidenceActor* Evidence);

	// ---- UI 廣播 ----

	/** 玩家互動射線焦點變化時呼叫，HUD 監聽以顯示/隱藏提示。 */
	UFUNCTION(BlueprintCallable, Category = "HV|UI")
	void BroadcastInteractHint(bool bVisible, const FText& HintText);

	UPROPERTY(BlueprintAssignable, Category = "HV|State")
	FHVOnGameStateChanged OnGameStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "HV|Evidence")
	FHVOnEvidenceChanged OnEvidenceChanged;

	UPROPERTY(BlueprintAssignable, Category = "HV|Evidence")
	FHVOnRoom1013Unlocked OnRoom1013Unlocked;

	UPROPERTY(BlueprintAssignable, Category = "HV|UI")
	FHVOnInteractHintChanged OnInteractHintChanged;

protected:
	virtual void BeginPlay() override;

	/** 集滿證據時呼叫：標記解鎖、廣播給關卡藍圖讓 1013 房門「浮現」。 */
	void UnlockRoom1013();

	/** 閱讀 Widget 關閉回呼：收取證據、還原輸入、回到 Play 狀態。 */
	UFUNCTION()
	void HandleReadingClosed();

	/** 閱讀 UI 的 C++ 基類藍圖子類（在 BP_HVGameMode 指定 WBP_Reading）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Reading")
	TSubclassOf<UHVReadingWidget> ReadingWidgetClass;

	/** 通關所需證據總數。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|Evidence", meta = (ClampMin = "1"))
	int32 RequiredEvidenceCount = 5;

	/** 遊戲啟動時的初始狀態（測試關卡可直接設 Play 跳過開場）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HV|State")
	EHVGameState InitialState = EHVGameState::Title;

private:
	EHVGameState CurrentState = EHVGameState::Title;

	UPROPERTY()
	TSet<FName> CollectedEvidenceIds;

	bool bRoom1013Unlocked = false;

	UPROPERTY()
	TObjectPtr<UHVReadingWidget> ReadingWidget;

	TWeakObjectPtr<AHVEvidenceActor> PendingEvidence;
};

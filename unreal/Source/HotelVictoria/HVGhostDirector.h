// 女鬼導演（World Subsystem）：冷卻排程、12 種事件資料表驅動（依玩家樓層/證據數篩選）、
// 各事件的 Spawn/姿態/移動/消散 C++ 入口；強度隨證據數提升。
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HVTypes.h"
#include "HVGhostDirector.generated.h"

class AHVGhostCharacter;
class UCurveFloat;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHVOnGhostEventTriggered, EHVGhostEventType, EventType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHVOnLightsOutRequested, bool, bLightsOut);

/**
 * 出沒排程中樞。
 * - Tick 倒數冷卻，到點後依玩家樓層 / 證據數自 EventTable 加權抽選事件。
 * - 每種事件有獨立的 C++ 觸發入口（計算 Spawn 位置、姿態、參數化位移、消散排程）。
 * - 同時間僅一隻女鬼活動；事件結束後依資料表冷卻（除以強度係數）。
 * - 證據越多 → IntensityScale 越高 → 冷卻越短、事件越貼近玩家。
 * - 場景定位點：放 TargetPoint Actor 並加上資料表列的 SpawnTag（如 TVPoint / StairHangPoint）。
 * - LightsOutFace 事件以 OnLightsOutRequested 廣播熄燈/復燈，關卡藍圖監聽執行。
 */
UCLASS(Config = Game)
class HOTELVICTORIA_API UHVGhostDirector : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- USubsystem / FTickableGameObject ----
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// ---- 外部控制 ----

	/** GameMode 依流程狀態啟停排程（僅 Play 狀態開啟）。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void SetEnabled(bool bInEnabled);

	/** GameMode 證據數變化時呼叫：拉高強度係數。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void NotifyEvidenceCount(int32 NewCount);

	/** 觸發器點名事件（如電梯倒掛）：在指定 Transform 強制觸發，必要時先回收現役女鬼。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void RequestEventAtTransform(EHVGhostEventType EventType, const FTransform& SpawnTransform);

	/** 由 BP_HVGameMode 注入資料表（亦可走 DefaultGame.ini 的 Config 軟路徑）。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void SetEventTable(UDataTable* InTable) { EventTable = InTable; }

	/** 由 BP_HVGameMode 注入女鬼藍圖類別。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void SetGhostClass(TSubclassOf<AHVGhostCharacter> InClass) { GhostClass = InClass; }

	/** 注入位移 Ease 曲線（0→1，建議 EaseInOut）。 */
	UFUNCTION(BlueprintCallable, Category = "HV|Ghost")
	void SetMoveEaseCurve(UCurveFloat* InCurve) { MoveEaseCurve = InCurve; }

	/** 玩家目前樓層（1~10），由 Z 高度換算。 */
	UFUNCTION(BlueprintPure, Category = "HV|Ghost")
	int32 GetPlayerFloor() const;

	/** 1 + 證據數 × IntensityPerEvidence。 */
	UFUNCTION(BlueprintPure, Category = "HV|Ghost")
	float GetIntensityScale() const;

	/** 任一事件觸發瞬間廣播（音效 sting、控制器震動掛這裡）。 */
	UPROPERTY(BlueprintAssignable, Category = "HV|Ghost")
	FHVOnGhostEventTriggered OnGhostEventTriggered;

	/** 熄燈貼臉事件廣播：true=熄燈、false=復燈，由關卡藍圖執行實際燈光開關。 */
	UPROPERTY(BlueprintAssignable, Category = "HV|Ghost")
	FHVOnLightsOutRequested OnLightsOutRequested;

protected:
	// ---- 排程內部 ----
	bool TryTriggerScheduledEvent();
	bool TriggerEvent(const FHVGhostEventRow& Row, const FTransform* OverrideTransform);
	void FinishEventSetup(const FHVGhostEventRow& Row);

	/** 生成女鬼並淡入。失敗回傳 nullptr。 */
	AHVGhostCharacter* SpawnGhost(const FTransform& Transform, EHVGhostMode Mode, float FadeInTime);

	/** 找同樓層、距玩家最近、帶指定 Tag 的 TargetPoint。 */
	bool FindTaggedSpawn(FName Tag, FTransform& OutTransform) const;

	/** 取玩家視點（相機位置與朝向）。 */
	bool GetPlayerView(FVector& OutLocation, FRotator& OutRotation) const;

	/** 排程 Duration 秒後淡出消散。 */
	void ScheduleDespawn(float Duration);

	UFUNCTION()
	void HandleDespawnTimer();

	UFUNCTION()
	void HandleGhostFadedOut();

	/** 凝視檢查：bDespawnWhenSeen 事件被玩家直視時提前消散。 */
	void UpdateSeenCheck(float DeltaTime);

	// ---- 12 種事件的 C++ 觸發入口（計算 Spawn/姿態/位移；具體運動以曲線參數化）----
	bool Trigger_WallCrawl(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_CeilingCrawl(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_ContortWalk(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_TVCrawlOut(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_ElevatorHang(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_PassBehind(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_StandBehind(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_DistantStare(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_DoorPeek(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_LightsOutFace(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_StairwellHang(const FHVGhostEventRow& Row, const FTransform* Override);
	bool Trigger_LobbyCrawl(const FHVGhostEventRow& Row, const FTransform* Override);

	/** 取得事件的 Spawn Transform：優先 Override → SpawnTag 定位點 → 玩家相對位置保底。 */
	bool ResolveSpawnTransform(const FHVGhostEventRow& Row, const FTransform* Override,
		const FVector& RelativeFallbackOffset, FTransform& OutTransform) const;

	// ---- 設定（可由 DefaultGame.ini [/Script/HotelVictoria.HVGhostDirector] 區段覆寫）----

	/** 事件資料表軟路徑（RowStruct = HVGhostEventRow）。 */
	UPROPERTY(Config)
	FSoftObjectPath GhostEventTablePath;

	/** 女鬼藍圖類別軟路徑（…/BP_Ghost.BP_Ghost_C）。 */
	UPROPERTY(Config)
	FSoftObjectPath GhostClassPath;

	/** 開局首次出沒前的冷卻秒數。 */
	UPROPERTY(Config)
	float InitialCooldown = 25.f;

	/** 抽選失敗（無候選列 / 定位點缺失）後的重試間隔秒數。 */
	UPROPERTY(Config)
	float RetryInterval = 8.f;

	/** 每份證據增加的強度（冷卻 = CooldownAfter / (1 + N × 此值)）。 */
	UPROPERTY(Config)
	float IntensityPerEvidence = 0.25f;

	/** 1 樓地板的世界 Z（cm）。 */
	UPROPERTY(Config)
	float GroundFloorZ = 0.f;

	/** 標準層層高（cm）。 */
	UPROPERTY(Config)
	float FloorHeight = 500.f;

private:
	UPROPERTY()
	TObjectPtr<UDataTable> EventTable;

	UPROPERTY()
	TSubclassOf<AHVGhostCharacter> GhostClass;

	UPROPERTY()
	TObjectPtr<UCurveFloat> MoveEaseCurve;

	UPROPERTY()
	TObjectPtr<AHVGhostCharacter> ActiveGhost;

	bool bEnabled = false;
	int32 EvidenceCount = 0;
	float CooldownRemaining = 25.f;
	float PendingCooldown = 30.f;
	float ActiveEventElapsed = 0.f;
	bool bActiveDespawnWhenSeen = false;
	bool bDespawnInProgress = false;
	EHVGhostEventType ActiveEventType = EHVGhostEventType::None;
	EHVGhostEventType LastEventType = EHVGhostEventType::None;

	FTimerHandle DespawnTimerHandle;
};

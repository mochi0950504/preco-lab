// 共用型別定義：遊戲流程狀態、女鬼事件/姿態列舉、女鬼事件資料表列結構。
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "HVTypes.generated.h"

/** 遊戲整體流程狀態（由 AHVGameMode 持有與廣播）。 */
UENUM(BlueprintType)
enum class EHVGameState : uint8
{
	Title      UMETA(DisplayName = "標題畫面"),
	Cinematic  UMETA(DisplayName = "開場運鏡"),
	Play       UMETA(DisplayName = "自由探索"),
	Reading    UMETA(DisplayName = "閱讀文件"),
	Finale     UMETA(DisplayName = "終章")
};

/** 女鬼 12 種出沒事件（永不傷害玩家，純氛圍驚嚇）。 */
UENUM(BlueprintType)
enum class EHVGhostEventType : uint8
{
	None           UMETA(DisplayName = "無"),
	WallCrawl      UMETA(DisplayName = "爬牆"),
	CeilingCrawl   UMETA(DisplayName = "天花板爬行"),
	ContortWalk    UMETA(DisplayName = "反折行走"),
	TVCrawlOut     UMETA(DisplayName = "電視爬出"),
	ElevatorHang   UMETA(DisplayName = "電梯倒掛"),
	PassBehind     UMETA(DisplayName = "背後經過"),
	StandBehind    UMETA(DisplayName = "背後佇立"),
	DistantStare   UMETA(DisplayName = "遠端凝視"),
	DoorPeek       UMETA(DisplayName = "門縫探頭"),
	LightsOutFace  UMETA(DisplayName = "熄燈貼臉"),
	StairwellHang  UMETA(DisplayName = "樓梯間倒吊"),
	LobbyCrawl     UMETA(DisplayName = "大廳爬行")
};

/** 女鬼當前姿態（AnimInstance 讀取此值切換動畫狀態機分支）。 */
UENUM(BlueprintType)
enum class EHVGhostMode : uint8
{
	Hidden        UMETA(DisplayName = "隱藏"),
	Stand         UMETA(DisplayName = "佇立"),
	Walk          UMETA(DisplayName = "行走"),
	ContortWalk   UMETA(DisplayName = "反折行走"),
	CrawlFloor    UMETA(DisplayName = "地面爬行"),
	CrawlWall     UMETA(DisplayName = "爬牆"),
	CrawlCeiling  UMETA(DisplayName = "天花板爬行"),
	HangInverted  UMETA(DisplayName = "倒吊"),
	DoorPeek      UMETA(DisplayName = "門縫探頭"),
	FaceClose     UMETA(DisplayName = "貼臉")
};

/**
 * 女鬼事件資料表列（DataTable 列結構，RowStruct 選 HVGhostEventRow）。
 * UHVGhostDirector 依玩家樓層 / 已收集證據數篩選候選列，再按 Weight 加權隨機抽選。
 */
USTRUCT(BlueprintType)
struct FHVGhostEventRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 事件種類。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost")
	EHVGhostEventType EventType = EHVGhostEventType::None;

	/** 允許觸發的最低樓層（1 = 大廳）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MinFloor = 1;

	/** 允許觸發的最高樓層。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxFloor = 10;

	/** 至少需收集幾份證據後才會出現（強度遞進）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost", meta = (ClampMin = "0", ClampMax = "5"))
	int32 MinEvidence = 0;

	/** 加權隨機權重；0 = 不參與隨機排程（僅能由觸發器點名，例如電梯倒掛）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	/** 事件持續秒數（到時自動淡出消散）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost", meta = (ClampMin = "0.5"))
	float Duration = 4.f;

	/** 事件結束後的基礎冷卻秒數（會被強度係數除短）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost", meta = (ClampMin = "1.0"))
	float CooldownAfter = 45.f;

	/** 出沒定位點 Actor Tag（場景中放 TargetPoint 並加同名 Tag）；None = 以玩家相對位置計算。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost")
	FName SpawnTag = NAME_None;

	/** true = 找不到同樓層 SpawnTag 定位點時放棄本次觸發（如電視、門縫等必須對位的事件）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost")
	bool bRequiresSpawnTag = false;

	/** true = 玩家直視女鬼時提前淡出（背後經過 / 背後佇立用）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost")
	bool bDespawnWhenSeen = false;
};

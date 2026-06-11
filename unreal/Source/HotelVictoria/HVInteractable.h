// 可互動物件介面：證據、門、電梯等皆實作此介面，玩家射線命中後呼叫。
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HVInteractable.generated.h"

class AHVPlayerCharacter;

UINTERFACE(MinimalAPI, BlueprintType)
class UHVInteractable : public UInterface
{
	GENERATED_BODY()
};

class HOTELVICTORIA_API IHVInteractable
{
	GENERATED_BODY()

public:
	/** 玩家按下互動鍵時呼叫。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interact")
	void Interact(AHVPlayerCharacter* Interactor);

	/** 準心對準時顯示的提示文字（例如「E 撿起登記簿」）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interact")
	FText GetInteractHint();

	/** 目前是否可互動（已收取的證據、運轉中的門回傳 false）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interact")
	bool CanInteract();
	virtual bool CanInteract_Implementation() { return true; }
};

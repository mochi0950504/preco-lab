// 《十樓的她》主遊戲模組入口：註冊 Primary Game Module 與日誌分類。
#include "HotelVictoria.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogHotelVictoria);

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, HotelVictoria, "HotelVictoria");

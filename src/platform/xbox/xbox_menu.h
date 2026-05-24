#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void Xbox_Menu_Update(void);
void Xbox_Menu_Draw(void);
bool Xbox_Menu_IsOpen(void);
bool Xbox_Menu_IsTurbo(void);
bool Xbox_Menu_IsPaused(void);
bool Xbox_Menu_JustClosed(void);
bool Xbox_Menu_IsGodHealth(void);
bool Xbox_Menu_IsGodMagic(void);
bool Xbox_Menu_IsGodBombs(void);
bool Xbox_Menu_IsGodArrows(void);
bool Xbox_Menu_IsGodRupees(void);
#ifdef __cplusplus
}
#endif
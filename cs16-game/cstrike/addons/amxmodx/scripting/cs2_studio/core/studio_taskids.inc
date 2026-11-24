#if defined _studio_taskids_included
    #endinput
#endif
#define _studio_taskids_included

// Task ID Partition: 40000 - 49999 (cs2_client_studio)
#define TASK_STU_PROBE   40100 // + player id (unified xhair + viewmodel probe)
#define TASK_HUD_SYNC    44100 // + player id (batched crosshair/HUD cvar sync)
#define TASK_HUD_SAVE    44500 // + player id (deferred console crosshair import save)
#define TASK_VM_DEFER    46200 // + player id (deferred Knife/M4 viewmodel sync)
#define TASK_VM_SPAWN    46300 // + player id (post-spawn viewmodel sync)

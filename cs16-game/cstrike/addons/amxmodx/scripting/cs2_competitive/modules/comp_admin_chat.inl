#if defined _comp_admin_chat_included
    #endinput
#endif
#define _comp_admin_chat_included

// ========================================================================
// ADMIN COMMANDS
// ========================================================================
public AdminCmd_StartMatch(id, level, cid)
{
    if (!cmd_access(id, level, cid, 1))
        return PLUGIN_HANDLED;

    client_print(0, print_chat, "^x04[CS:GO Match] Admin force-started the match!");
    remove_task(TASK_COMP_WARMUP);
    remove_task(TASK_COMP_COUNTDOWN);
    StartLiveMatch();
    return PLUGIN_HANDLED;
}

public AdminCmd_RestartWarmup(id, level, cid)
{
    if (!cmd_access(id, level, cid, 1))
        return PLUGIN_HANDLED;

    client_print(0, print_chat, "^x04[CS:GO Match] Admin reset the match to Warmup!");
    remove_task(TASK_COMP_WARMUP);
    remove_task(TASK_COMP_SWEEP1);
    remove_task(TASK_COMP_SWEEP2);
    remove_task(TASK_COMP_COUNTDOWN);
    remove_task(TASK_COMP_SIDEPICK);
    remove_task(TASK_COMP_TIMEOUT);
    remove_task(TASK_COMP_MATCH_END);
    remove_task(TASK_COMP_HALF2);
    remove_task(TASK_COMP_OT2);
    remove_task(TASK_COMP_SYNC_TELEMETRY);
    ResetToWarmup();
    return PLUGIN_HANDLED;
}

// ========================================================================
// CHAT COMMAND HANDLER
// ========================================================================
public HandleSay(id)
{
    if (get_pcvar_num(pCvar_Mode) == 0)
        return PLUGIN_CONTINUE;

    new text[64];
    read_args(text, charsmax(text));
    remove_quotes(text);
    trim(text);

    if (equali(text, ".ready") || equali(text, "!ready") || equali(text, "ready"))
    {
        Cmd_Ready(id);
        return PLUGIN_HANDLED;
    }
    else if (equali(text, ".unready") || equali(text, "!unready") || equali(text, ".notready") || equali(text, "unready"))
    {
        Cmd_Unready(id);
        return PLUGIN_HANDLED;
    }
    else if (equali(text, ".score") || equali(text, "!score"))
    {
        Cmd_Score(id);
        return PLUGIN_HANDLED;
    }
    else if (equali(text, ".dmg") || equali(text, ".damage") || equali(text, "!dmg") || equali(text, ".hp"))
    {
        Cmd_Damage(id);
        return PLUGIN_HANDLED;
    }
    else if (equali(text, ".timeout") || equali(text, ".pause") || equali(text, "!timeout") || equali(text, "!pause"))
    {
        Cmd_Timeout(id);
        return PLUGIN_HANDLED;
    }
    else if (equali(text, ".stay"))
    {
        Cmd_SideChoice(id, 1);
        return PLUGIN_HANDLED;
    }
    else if (equali(text, ".switch"))
    {
        Cmd_SideChoice(id, 2);
        return PLUGIN_HANDLED;
    }

    return PLUGIN_CONTINUE;
}

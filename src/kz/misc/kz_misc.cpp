#include "common.h"
#include "utils/utils.h"
#include "utils/ctimer.h"
#include "kz/kz.h"
#include "utils/simplecmds.h"

#include "kz/checkpoint/kz_checkpoint.h"
#include "kz/jumpstats/kz_jumpstats.h"
#include "kz/quiet/kz_quiet.h"
#include "kz/mode/kz_mode.h"
#include "kz/language/kz_language.h"
#include "kz/style/kz_style.h"
#include "kz/noclip/kz_noclip.h"
#include "kz/hud/kz_hud.h"
#include "kz/option/kz_option.h"
#include "kz/spec/kz_spec.h"
#include "kz/goto/kz_goto.h"
#include "kz/telemetry/kz_telemetry.h"
#include "kz/timer/kz_timer.h"
#include "kz/tip/kz_tip.h"
#include "kz/global/kz_global.h"

#include "sdk/gamerules.h"

#define RESTART_CHECK_INTERVAL 1800.0f
static_global CTimer<> *mapRestartTimer;

static_global class KZOptionServiceEventListener_Misc : public KZOptionServiceEventListener
{
	virtual void OnPlayerPreferencesLoaded(KZPlayer *player)
	{
		bool hideLegs = player->optionService->GetPreferenceBool("hideLegs", false);
		if (player->HidingLegs() != hideLegs)
		{
			player->ToggleHideLegs();
		}
	}
} optionEventListener;

SCMD(kz_hidelegs, SCFL_PLAYER | SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->ToggleHideLegs();
	if (player->HidingLegs())
	{
		player->languageService->PrintChat(true, false, "Quiet Option - Show Player Legs - Disable");
	}
	else
	{
		player->languageService->PrintChat(true, false, "Quiet Option - Show Player Legs - Enable");
	}
	return MRES_SUPERCEDE;
}

SCMD(kz_hide, SCFL_PLAYER | SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->quietService->ToggleHide();
	if (player->quietService->hideOtherPlayers)
	{
		player->languageService->PrintChat(true, false, "Quiet Option - Show Players - Disable");
	}
	else
	{
		player->languageService->PrintChat(true, false, "Quiet Option - Show Players - Enable");
	}
	return MRES_SUPERCEDE;
}

SCMD(kz_end, SCFL_MAP)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);

	// If the player specify a course name, we first check if it's valid or not.
	if (V_strlen(args->ArgS()) > 0)
	{
		const KZCourseDescriptor *course = KZ::course::GetCourse(args->ArgS(), false);

		if (!course || !course || !course->hasEndPosition)
		{
			player->languageService->PrintChat(true, false, "No End Position For Course", args->ArgS());
			return MRES_SUPERCEDE;
		}
	}

	bool shouldTeleport = false;
	Vector tpOrigin;
	QAngle tpAngles;
	if (player->timerService->GetCourse())
	{
		if (player->timerService->GetCourse()->hasEndPosition)
		{
			tpOrigin = player->timerService->GetCourse()->endPosition;
			tpAngles = player->timerService->GetCourse()->endAngles;
			shouldTeleport = true;
		}
		else
		{
			CUtlString courseName = player->timerService->GetCourse()->GetName();
			player->languageService->PrintChat(true, false, "No End Position For Course", courseName.Get());
			return MRES_SUPERCEDE;
		}
	}

	// If we have no active course the map only has one course, !r should send the player to the end of that course.
	else if (KZ::course::GetCourseCount() == 1)
	{
		const KZCourseDescriptor *descriptor = KZ::course::GetFirstCourse();
		if (descriptor->hasEndPosition)
		{
			tpOrigin = descriptor->endPosition;
			tpAngles = descriptor->endAngles;
			shouldTeleport = true;
		}
		else
		{
			CUtlString courseName = KZ::course::GetFirstCourse()->GetName();
			player->languageService->PrintChat(true, false, "No End Position For Course", courseName.Get());
		}
	}

	if (shouldTeleport)
	{
		player->timerService->TimerStop();
		if (player->GetPlayerPawn()->IsAlive())
		{
			if (player->noclipService->IsNoclipping())
			{
				player->noclipService->DisableNoclip();
				player->noclipService->HandleNoclip();
			}
		}
		else
		{
			KZ::misc::JoinTeam(player, CS_TEAM_CT, false);
		}
		player->Teleport(&tpOrigin, &tpAngles, &vec3_origin);
	}
	return MRES_SUPERCEDE;
}

SCMD(kz_restart, SCFL_TIMER | SCFL_MAP)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	const KZCourseDescriptor *startPosCourse = nullptr;
	// If the player specify a course name, we first check if it's valid or not.
	if (V_strlen(args->ArgS()) > 0)
	{
		startPosCourse = KZ::course::GetCourse(args->ArgS(), false);

		if (!startPosCourse || !startPosCourse || !startPosCourse->hasStartPosition)
		{
			player->languageService->PrintChat(true, false, "No Start Position For Course", args->ArgS());
			return MRES_SUPERCEDE;
		}
	}

	player->timerService->OnTeleportToStart();
	if (player->GetPlayerPawn()->IsAlive())
	{
		// Fix players spawning 500u under spawn positions.
		if (player->noclipService->IsNoclipping())
		{
			player->noclipService->DisableNoclip();
			player->noclipService->HandleNoclip();
		}
		player->GetPlayerPawn()->Respawn();
		player->quietService->ResetHideWeapon();
	}
	else
	{
		KZ::misc::JoinTeam(player, CS_TEAM_CT, false);
	}

	if (startPosCourse)
	{
		player->Teleport(&startPosCourse->startPosition, &startPosCourse->startAngles, &vec3_origin);
		return MRES_SUPERCEDE;
	}

	// Then prioritize custom start position.
	if (player->checkpointService->HasCustomStartPosition())
	{
		player->checkpointService->TpToStartPosition();
		return MRES_SUPERCEDE;
	}

	// If we have no custom start position, we try to get the start position of the current course.
	if (player->timerService->GetCourse())
	{
		if (player->timerService->GetCourse()->hasStartPosition)
		{
			player->Teleport(&player->timerService->GetCourse()->startPosition, &player->timerService->GetCourse()->startAngles, &vec3_origin);
		}
		return MRES_SUPERCEDE;
	}

	// If we have no active course the map only has one course, !r should send the player to that course.
	if (KZ::course::GetCourseCount() == 1)
	{
		const KZCourseDescriptor *descriptor = KZ::course::GetFirstCourse();
		if (descriptor && descriptor->hasStartPosition)
		{
			player->Teleport(&descriptor->startPosition, &descriptor->startAngles, &vec3_origin);
			return MRES_SUPERCEDE;
		}
	}

	return MRES_SUPERCEDE;
}

SCMD_LINK(kz_r, kz_restart);

SCMD(kz_lj, SCFL_JUMPSTATS | SCFL_MAP)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);

	Vector destPos;
	QAngle destAngles;
	if (g_pMappingApi->GetJumpstatArea(destPos, destAngles))
	{
		player->timerService->TimerStop();
		player->Teleport(&destPos, &destAngles, &vec3_origin);
	}
	else
	{
		player->languageService->PrintChat(true, false, "No Jumpstat Area Found", args->ArgS());
	}

	return MRES_SUPERCEDE;
}

SCMD_LINK(kz_ljarea, kz_lj);
SCMD_LINK(kz_jsarea, kz_lj);

SCMD(kz_hideweapon, SCFL_PLAYER | SCFL_PREFERENCE)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->quietService->ToggleHideWeapon();
	if (player->quietService->ShouldHideWeapon())
	{
		player->languageService->PrintChat(true, false, "Quiet Option - Show Weapon - Disable");
	}
	else
	{
		player->languageService->PrintChat(true, false, "Quiet Option - Show Weapon - Enable");
	}
	return MRES_SUPERCEDE;
}

SCMD(kz_playercheck, SCFL_PLAYER)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	KZPlayer *targetPlayer = nullptr;
	if (args->ArgC() < 2)
	{
		targetPlayer = player;
	}
	else
	{
		for (i32 i = 0; i <= MAXPLAYERS; i++)
		{
			CBasePlayerController *controller = g_pKZPlayerManager->players[i]->GetController();

			if (!controller)
			{
				continue;
			}

			if (V_strstr(V_strlower((char *)controller->GetPlayerName()), V_strlower((char *)args->ArgS())))
			{
				targetPlayer = g_pKZPlayerManager->ToPlayer(i);
				break;
			}
		}
	}
	if (!targetPlayer)
	{
		player->languageService->PrintChat(true, false, "Error Message (Player Not Found)", args->ArgS());
		return MRES_SUPERCEDE;
	}
	player->languageService->PrintChat(
		true, false, targetPlayer->IsAuthenticated() ? "Player Authenticated (Steam)" : "Player Not Authenticated (Steam)", targetPlayer->GetName());
	return MRES_SUPERCEDE;
}

SCMD_LINK(kz_pc, kz_playercheck);

SCMD(jointeam, SCFL_HIDDEN)
{
	KZ::misc::JoinTeam(g_pKZPlayerManager->ToPlayer(controller), atoi(args->Arg(1)), true);
	return MRES_SUPERCEDE;
}

SCMD(switchhands, SCFL_HIDDEN)
{
	KZPlayer *player = g_pKZPlayerManager->ToPlayer(controller);
	player->quietService->ResetHideWeapon();
	return MRES_IGNORED;
}

SCMD_LINK(switchhandsleft, switchhands);
SCMD_LINK(switchhandsright, switchhands);

static_function f64 CheckRestart()
{
	utils::ResetMapIfEmpty();
	return RESTART_CHECK_INTERVAL;
}

void KZ::misc::Init()
{
	KZOptionService::RegisterEventListener(&optionEventListener);
	KZ::misc::EnforceTimeLimit();
	mapRestartTimer = StartTimer(CheckRestart, RESTART_CHECK_INTERVAL, true, true);
	CConVarRef<int32> sv_infinite_ammo("sv_infinite_ammo");
	if (sv_infinite_ammo.IsValidRef() && sv_infinite_ammo.IsConVarDataAvailable())
	{
		sv_infinite_ammo.RemoveFlags(FCVAR_CHEAT);
	}
	CConVarRef<CUtlString> bot_stop("bot_stop");
	if (bot_stop.IsValidRef() && bot_stop.IsConVarDataAvailable())
	{
		bot_stop.RemoveFlags(FCVAR_CHEAT);
	}
}

void KZ::misc::OnServerActivate()
{
	KZ::misc::EnforceTimeLimit();
	g_pKZUtils->UpdateCurrentMapMD5();

	interfaces::pEngine->ServerCommand("exec cs2kz.cfg");
	KZ::misc::InitTimeLimit();

	// Restart round to ensure settings (e.g. mp_weapons_allow_map_placed) are applied
	interfaces::pEngine->ServerCommand("mp_restartgame 1");
}

// TODO: Fullupdate spectators on spec_mode/spec_next/spec_player/spec_prev

void KZ::misc::JoinTeam(KZPlayer *player, int newTeam, bool restorePos)
{
	int currentTeam = player->GetController()->GetTeam();

	// Don't use CS_TEAM_NONE
	if (newTeam == CS_TEAM_NONE)
	{
		newTeam = CS_TEAM_SPECTATOR;
	}

	if (newTeam == CS_TEAM_SPECTATOR && currentTeam != CS_TEAM_SPECTATOR)
	{
		if (currentTeam != CS_TEAM_NONE)
		{
			player->specService->SavePosition();
		}

		if (!player->timerService->GetPaused() && !player->timerService->CanPause())
		{
			player->timerService->TimerStop();
		}
		player->GetController()->ChangeTeam(CS_TEAM_SPECTATOR);
		player->quietService->SendFullUpdate();
		// TODO: put spectators of this player to freecam, and send them full updates
	}
	else if (newTeam == CS_TEAM_CT && currentTeam != CS_TEAM_CT || newTeam == CS_TEAM_T && currentTeam != CS_TEAM_T)
	{
		player->GetPlayerPawn()->CommitSuicide(false, true);
		player->GetController()->SwitchTeam(newTeam);
		player->GetController()->Respawn();
		player->quietService->ResetHideWeapon();
		if (restorePos && player->specService->HasSavedPosition())
		{
			player->specService->LoadPosition();
		}
		else
		{
			player->timerService->TimerStop();
			// Just joining a team alone can put you into weird invalid spawns.
			// Need to teleport the player to a valid one.
			Vector spawnOrigin {};
			QAngle spawnAngles {};
			if (utils::FindValidSpawn(spawnOrigin, spawnAngles))
			{
				auto pawn = player->GetPlayerPawn();
				pawn->Teleport(&spawnOrigin, &spawnAngles, &vec3_origin);
				// CS2 bug: m_flWaterJumpTime is not properly initialized upon player spawn.
				// If the player is teleported on the very first tick of movement and lose the ground flag,
				// the player might get teleported to a random place.
				pawn->m_pWaterServices()->m_flWaterJumpTime(0.0f);
			}
		}
		player->specService->ResetSavedPosition();
	}
	player->tipService->OnPlayerJoinTeam(newTeam);
}

static_function void SanitizeMsg(const char *input, char *output, u32 size)
{
	int x = 0;
	for (int i = 0; input[i] != '\0'; i++)
	{
		if (x + 1 == size)
		{
			break;
		}

		int character = input[i];

		if (character > 0x10)
		{
			if (character == '\\')
			{
				output[x++] = character;
			}
			output[x++] = character;
		}
	}

	output[x] = '\0';
}

void KZ::misc::ProcessConCommand(ConCommandRef cmd, const CCommandContext &ctx, const CCommand &args)
{
	if (!GameEntitySystem())
	{
		return;
	}
	CPlayerSlot slot = ctx.GetPlayerSlot();

	CCSPlayerController *controller = (CCSPlayerController *)utils::GetController(slot);

	KZPlayer *player = NULL;

	if (!cmd.IsValidRef() || !controller || !(player = g_pKZPlayerManager->ToPlayer(controller)))
	{
		return;
	}
	const char *p {};
	const char *commandName = cmd.GetName();

	// Is it a chat message?
	if (!V_stricmp(commandName, "say") || !V_stricmp(commandName, "say_team"))
	{
		if (args.ArgC() < 2)
		{
			// no argument, happens when the player just types "say" or "say_team" in console
			return;
		}
		// 3 cases:
		// say ""
		// say_team ""
		// say /<insert text>
		i32 argLen = strlen(args.ArgS());
		p = args.ArgS();
		bool wrappedInQuotes = false;
		if (args.ArgS()[0] == '"' && args.ArgS()[argLen - 1] == '"')
		{
			argLen -= 2;
			wrappedInQuotes = true;
			p += 1;
		}
		if (argLen < 1 || args[1][0] == SCMD_CHAT_SILENT_TRIGGER)
		{
			// arg is too short!
			return;
		}

		CUtlString message;
		message.SetDirect(p, strlen(p) - wrappedInQuotes);

		if (player->IsAlive())
		{
			utils::SayChat(player->GetController(), "{lime}%s{default}: %s", player->GetName(), message.Get());
			utils::PrintConsoleAll("%s: %s", player->GetName(), message.Get());
			META_CONPRINTF("%s: %s\n", player->GetName(), message.Get());
		}
		else
		{
			utils::SayChat(player->GetController(), "{grey}* {lime}%s{default}: %s", player->GetName(), message.Get());
			utils::PrintConsoleAll("* %s: %s", player->GetName(), message.Get());
			META_CONPRINTF("* %s: %s\n", player->GetName(), message.Get());
		}
	}

	return;
}

void KZ::misc::OnRoundStart()
{
	CCSGameRules *gameRules = g_pKZUtils->GetGameRules();
	if (gameRules)
	{
		gameRules->m_bGameRestart(true);
		gameRules->m_iRoundWinStatus(1);
		// Make sure that the round time is synchronized with the global time.
		gameRules->m_fRoundStartTime().SetTime(0.0f);
		gameRules->m_flGameStartTime().SetTime(0.0f);
	}
}

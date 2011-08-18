
#include "script/ScriptedNPC.h"

#include "game/NPC.h"
#include "graphics/Math.h"
#include "graphics/data/Mesh.h"
#include "gui/Speech.h"
#include "gui/Interface.h"
#include "io/FilePath.h"
#include "platform/String.h"
#include "scene/Interactive.h"
#include "script/ScriptEvent.h"
#include "script/ScriptUtils.h"

using std::string;

namespace script {

namespace {

class BehaviourCommand : public Command {
	
public:
	
	BehaviourCommand() : Command("behavior", IO_NPC) { }
	
	Result execute(Context & context) {
		
		INTERACTIVE_OBJ * io = context.getIO();
		
		Behaviour behavior = 0;
		HandleFlags("lsdmfa012") {
			behavior |= (flg & flag('l')) ? BEHAVIOUR_LOOK_AROUND : Behaviour(0);
			behavior |= (flg & flag('s')) ? BEHAVIOUR_SNEAK : Behaviour(0);
			behavior |= (flg & flag('d')) ? BEHAVIOUR_DISTANT : Behaviour(0);
			behavior |= (flg & flag('m')) ? BEHAVIOUR_MAGIC : Behaviour(0);
			behavior |= (flg & flag('f')) ? BEHAVIOUR_FIGHT : Behaviour(0);
			behavior |= (flg & flag('a')) ? BEHAVIOUR_STARE_AT : Behaviour(0);
			if(flg & flags("012")) {
				io->_npcdata->tactics = 0;
			}
		}
		
		string command = context.getWord();
		
		if(options.empty()) {
			if(command == "stack") {
				DebugScript(' ' << options << ' ' << command);
				ARX_NPC_Behaviour_Stack(io);
				return Success;
			} else if(command == "unstack") {
				DebugScript(' ' << options << ' ' << command);
				ARX_NPC_Behaviour_UnStack(io);
				return Success;
			} else if(command == "unstackall") {
				DebugScript(' ' << options << ' ' << command);
				ARX_NPC_Behaviour_Reset(io);
				return Success;
			}
		}
		
		float behavior_param = 0.f;
		if(command == "go_home") {
			behavior |= BEHAVIOUR_GO_HOME;
		} else if(command == "friendly") {
			io->_npcdata->movemode = NOMOVEMODE;
			behavior |= BEHAVIOUR_FRIENDLY;
		} else if(command == "move_to") {
			io->_npcdata->movemode = WALKMODE;
			behavior |= BEHAVIOUR_MOVE_TO;
		} else if(command == "flee") {
			behavior_param = context.getFloat();
			io->_npcdata->movemode = RUNMODE;
			behavior |= BEHAVIOUR_FLEE;
		} else if(command == "look_for") {
			behavior_param = context.getFloat();
			io->_npcdata->movemode = WALKMODE;
			behavior |= BEHAVIOUR_LOOK_FOR;
		} else if(command == "hide") {
			behavior_param = context.getFloat();
			io->_npcdata->movemode = WALKMODE;
			behavior |= BEHAVIOUR_HIDE;
		} else if(command == "wander_around") {
			behavior_param = context.getFloat();
			io->_npcdata->movemode = WALKMODE;
			behavior |= BEHAVIOUR_WANDER_AROUND;
		} else if(command == "guard") {
			behavior |= BEHAVIOUR_GUARD;
			io->targetinfo = -2;
			io->_npcdata->movemode = NOMOVEMODE;
		} else if(command != "none") {
			ScriptWarning << "unexpected command: " << options << " \"" << command << '"';
		}
		
		DebugScript(' ' << options << " \"" << command << "\" " << behavior_param);
		
		ARX_NPC_Behaviour_Change(io, behavior, checked_range_cast<long>(behavior_param));
		
		return Success;
	}
	
};

class ReviveCommand : public Command {
	
public:
	
	ReviveCommand() : Command("revive", ANY_IO) { }
	
	Result execute(Context & context) {
		
		bool init = false;
		HandleFlags("i") {
			init = test_flag(flg, 'i');
		}
		
		DebugScript(' ' << options);
		
		ARX_NPC_Revive(context.getIO(), init ? 1 : 0);
		
		return Success;
	}
	
};

class SpellcastCommand : public Command {
	
public:
	
	SpellcastCommand() : Command("spellcast", ANY_IO) { }
	
	Result execute(Context & context) {
		
		SpellcastFlags spflags = 0;
		long duration = -1;
		bool haveDuration = 0;
		
		HandleFlags("kdxmsfz") {
			
			if(flg & flag('k')) {
				
				string spellname = context.getWord();
				Spell spellid = GetSpellId(spellname);
				
				DebugScript(' ' << options << ' ' << spellname);
				
				long from = GetInterNum(context.getIO());
				if(ValidIONum(from)) {
					long sp = ARX_SPELLS_GetInstanceForThisCaster(spellid, from);
					if(sp >= 0) {
						spells[sp].tolive = 0;
					}
				}
				
				return Success;
			}
			
			if(flg & flag('d')) {
				spflags |= SPELLCAST_FLAG_NOCHECKCANCAST;
				duration = context.getFloat();
				if(duration <= 0) {
					duration = 99999999; // TODO should this be FLT_MAX?
				}
				haveDuration = 1;
			}
			if(flg & flag('x')) {
				spflags |= SPELLCAST_FLAG_NOSOUND;
			}
			if(flg & flag('m')) {
				spflags |= SPELLCAST_FLAG_NOCHECKCANCAST | SPELLCAST_FLAG_NODRAW;
			}
			if(flg & flag('s')) {
				spflags |= SPELLCAST_FLAG_NOCHECKCANCAST | SPELLCAST_FLAG_NOANIM;
			}
			if(flg & flag('f')) {
				spflags |= SPELLCAST_FLAG_NOCHECKCANCAST | SPELLCAST_FLAG_NOMANA;
			}
			if(flg & flag('z')) {
				spflags |= SPELLCAST_FLAG_RESTORE;
			}
		}
		
		long level = clamp(static_cast<long>(context.getFloat()), 1l, 10l);
		if(!haveDuration) {
			duration = 1000 + level * 2000;
		}
		
		string spellname = context.getWord();
		Spell spellid = GetSpellId(spellname);
		
		string target = context.getWord();
		INTERACTIVE_OBJ * t = inter.getById(target, context.getIO());
		
		if(!t || spellid == SPELL_NONE) {
			return Failed;
		}
		
		if(context.getIO() != inter.iobj[0]) {
			spflags |= SPELLCAST_FLAG_NOCHECKCANCAST;
		}
		
		DebugScript(' ' << spellname << ' ' << level << ' ' << target << ' ' << spflags << ' ' << duration);
		
		TryToCastSpell(context.getIO(), spellid, level, GetInterNum(t), spflags, duration);
		
		return Success;
	}
	
};

class SetDetectCommand : public Command {
	
public:
	
	SetDetectCommand() : Command("setdetect", IO_NPC) { }
	
	Result execute(Context & context) {
		
		string detectvalue = context.getWord();
		
		DebugScript(' ' << detectvalue);
		
		if(detectvalue == "off") {
			context.getIO()->_npcdata->fDetect = -1;
		} else {
			context.getIO()->_npcdata->fDetect = clamp((int)context.getFloatVar(detectvalue), -1, 100);
		}
		
		return Success;
	}
	
};

class SetBloodCommand : public Command {
	
public:
	
	SetBloodCommand() : Command("setblood", IO_NPC) { }
	
	Result execute(Context & context) {
		
		float r = context.getFloat();
		float g = context.getFloat();
		float b = context.getFloat();
		
		DebugScript(' ' << r << ' ' << g << ' ' << b);
		
		context.getIO()->_npcdata->blood_color = Color3f(r, g, b).to<u8>();
		
		return Success;
	}
	
};

class SetSpeedCommand : public Command {
	
public:
	
	SetSpeedCommand() : Command("setspeed", ANY_IO) { }
	
	Result execute(Context & context) {
		
		float speed = clamp(context.getFloat(), 0.f, 10.f);
		
		DebugScript(' ' << speed);
		
		context.getIO()->basespeed = speed;
		
		return Success;
	}
	
};

class SetStareFactorCommand : public Command {
	
public:
	
	SetStareFactorCommand() : Command("setstarefactor", IO_NPC) { }
	
	Result execute(Context & context) {
		
		float stare_factor = context.getFloat();
		
		DebugScript(' ' << stare_factor);
		
		context.getIO()->_npcdata->stare_factor = stare_factor;
		
		return Success;
	}
	
};

class SetNPCStatCommand : public Command {
	
public:
	
	SetNPCStatCommand() : Command("setnpcstat", IO_NPC) { }
	
	Result execute(Context & context) {
		
		string stat = context.getWord();
		float value = context.getFloat();
		
		DebugScript(' ' << stat << ' ' << value);
		
		if(!ARX_NPC_SetStat(*context.getIO(), stat, value)) {
			ScriptWarning << "unknown stat name: " << stat << ' ' << value;
			return Failed;
		}
		
		return Success;
	}
	
};

class SetXPValueCommand : public Command {
	
public:
	
	SetXPValueCommand() : Command("setxpvalue", IO_NPC) { }
	
	Result execute(Context & context) {
		
		float xpvalue = context.getFloat();
		if(xpvalue < 0) {
			xpvalue = 0;
		}
		
		DebugScript(' ' << xpvalue);
		
		context.getIO()->_npcdata->xpvalue = (long)xpvalue;
		
		return Success;
	}
	
};

class SetMoveModeCommand : public Command {
	
public:
	
	SetMoveModeCommand() : Command("setmovemode", IO_NPC) { }
	
	Result execute(Context & context) {
		
		string mode = context.getWord();
		
		DebugScript(' ' << mode);
		
		INTERACTIVE_OBJ * io = context.getIO();
		if(mode == "walk") {
			ARX_NPC_ChangeMoveMode(io, WALKMODE);
		} else if(mode == "run") {
			ARX_NPC_ChangeMoveMode(io, RUNMODE);
		} else if(mode == "none") {
			ARX_NPC_ChangeMoveMode(io, NOMOVEMODE);
		} else if(mode == "sneak") {
			ARX_NPC_ChangeMoveMode(io, SNEAKMODE);
		} else {
			ScriptWarning << "unexpected mode: " << mode;
			return Failed;
		}
		
		return Success;
	}
	
};

class SetLifeCommand : public Command {
	
public:
	
	SetLifeCommand() : Command("setlife", IO_NPC) { }
	
	Result execute(Context & context) {
		
		float life = context.getFloat();
		
		DebugScript(' ' << life);
		
		context.getIO()->_npcdata->maxlife = context.getIO()->_npcdata->life = life;
		
		return Success;
	}
	
};

class SetTargetCommand : public Command {
	
public:
	
	SetTargetCommand() : Command("settarget", ANY_IO) { }
	
	Result execute(Context & context) {
		
		INTERACTIVE_OBJ * io = context.getIO();
		if(io->ioflags & IO_NPC) {
			io->_npcdata->pathfind.flags &= ~(PATHFIND_ALWAYS|PATHFIND_ONCE|PATHFIND_NO_UPDATE);
		}
		
		HandleFlags("san") {
			if(io->ioflags & IO_NPC) {
				if(flg & flag('s')) {
					io->_npcdata->pathfind.flags |= PATHFIND_ONCE;
				}
				if(flg & flag('a')) {
					io->_npcdata->pathfind.flags |= PATHFIND_ALWAYS;
				}
				if(flg & flag('n')) {
					io->_npcdata->pathfind.flags |= PATHFIND_NO_UPDATE;
				}
			}
		}
		
		long old_target = -12;
		if(io->ioflags & IO_NPC) {
			if(io->_npcdata->reachedtarget) {
				old_target = io->targetinfo;
			}
			if(io->_npcdata->behavior & (BEHAVIOUR_FLEE|BEHAVIOUR_WANDER_AROUND)) {
				old_target = -12;
			}
		}
		
		string target = context.getWord();
		if(target == "object") {
			target = context.getWord();
		}
		target = context.getStringVar(target);
		INTERACTIVE_OBJ * t = inter.getById(target, io);
		
		DebugScript(' ' << options << ' ' << target);
		
		if(io->ioflags & IO_CAMERA) {
			io->_camdata->cam.translatetarget = Vec3f::ZERO;
		}
		
		if(t != NULL) {
			io->targetinfo = GetInterNum(t);
			GetTargetPos(io);
		}
		
		if(target == "path") {
			io->targetinfo = TARGET_PATH;
			GetTargetPos(io);
		} else if(target == "none") {
			io->targetinfo = TARGET_NONE;
		}
		
		if(old_target != GetInterNum(t)) {
			if(io->ioflags & IO_NPC) {
				io->_npcdata->reachedtarget = 0;
			}
			ARX_NPC_LaunchPathfind(io, GetInterNum(t));
		}
		
		return Success;
	}
	
};

class ForceDeathCommand : public Command {
	
public:
	
	ForceDeathCommand() : Command("forcedeath", ANY_IO) { }
	
	Result execute(Context & context) {
		
		string target = context.getWord();
		
		DebugScript(' ' << target);
		
		INTERACTIVE_OBJ * t = inter.getById(target, context.getIO());
		if(!t) {
			ScriptWarning << "unknown target: " << target;
			return Failed;
		}
		
		ARX_DAMAGES_ForceDeath(t, context.getIO());
		
		return Success;
	}
	
};

class PathfindCommand : public Command {
	
public:
	
	PathfindCommand() : Command("pathfind", IO_NPC) { }
	
	Result execute(Context & context) {
		
		string target = context.getWord();
		
		DebugScript(' ' << target);
		
		long t = inter.getById(target);
		ARX_NPC_LaunchPathfind(context.getIO(), t);
		
		return Success;
	}
	
};

}

void setupScriptedNPC() {
	
	ScriptEvent::registerCommand(new BehaviourCommand);
	ScriptEvent::registerCommand(new ReviveCommand);
	ScriptEvent::registerCommand(new SpellcastCommand);
	ScriptEvent::registerCommand(new SetDetectCommand);
	ScriptEvent::registerCommand(new SetBloodCommand);
	ScriptEvent::registerCommand(new SetSpeedCommand);
	ScriptEvent::registerCommand(new SetStareFactorCommand);
	ScriptEvent::registerCommand(new SetNPCStatCommand);
	ScriptEvent::registerCommand(new SetXPValueCommand);
	ScriptEvent::registerCommand(new SetMoveModeCommand);
	ScriptEvent::registerCommand(new SetLifeCommand);
	ScriptEvent::registerCommand(new SetTargetCommand);
	ScriptEvent::registerCommand(new ForceDeathCommand);
	ScriptEvent::registerCommand(new PathfindCommand);
	
}

} // namespace script
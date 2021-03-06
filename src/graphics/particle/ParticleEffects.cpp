/*
 * Copyright 2011-2012 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Based on:
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved

#include "graphics/particle/ParticleEffects.h"

#include <algorithm>

#include <glm/gtx/norm.hpp>

#include "core/Application.h"
#include "core/Config.h"
#include "core/Core.h"
#include "core/GameTime.h"

#include "game/Damage.h"
#include "game/EntityManager.h"
#include "game/NPC.h"
#include "game/Player.h"

#include "gui/Interface.h"

#include "graphics/Draw.h"
#include "graphics/GraphicsModes.h"
#include "graphics/Math.h"
#include "graphics/data/TextureContainer.h"
#include "graphics/effects/SpellEffects.h"
#include "graphics/particle/MagicFlare.h"

#include "input/Input.h"

#include "math/Random.h"

#include "physics/Collisions.h"

#include "scene/GameSound.h"
#include "scene/Interactive.h"
#include "scene/Light.h"

using std::max;

//TODO(lubosz): extern globals :(
extern Color ulBKGColor;

struct OBJFX {
	Vec3f pos;
	Vec3f move;
	Vec3f scale;
	Color3f fade;
	EERIE_3DOBJ * obj;
	long special;
	Anglef spe[8];
	Anglef speinc[8];
	unsigned long tim_start;
	unsigned long duration;
	bool exist;
	long dynlight;
};

static const size_t MAX_PARTICLES = 2200;
static long ParticleCount = 0;
static PARTICLE_DEF particle[MAX_PARTICLES];

static TextureContainer * bloodsplat[6];
TextureContainer * water_splat[3];
static TextureContainer * water_drop[3];
static TextureContainer * smokeparticle = NULL;
static TextureContainer * bloodsplatter = NULL;
static TextureContainer * healing = NULL;
static TextureContainer * tzupouf = NULL;
TextureContainer * fire2=NULL;

short			OPIPOrgb=0;
short			PIPOrgb=0;

long			NewSpell=0;

long getParticleCount() {
	return ParticleCount;
}

void LaunchDummyParticle() {
		
	PARTICLE_DEF * pd = createParticle();
	if(!pd) {
		return;
	}
	
	float f = radians(player.angle.getPitch());
	pd->ov = player.pos + Vec3f(EEsin(f) * 100.f, 0.f, -EEcos(f) * 100.f);
	pd->tolive = 600;
	pd->tc = smokeparticle;
	pd->siz = 15.f;
	pd->scale = randomVec(-15.f, -10.f);
	pd->special = FIRE_TO_SMOKE;
}

void ARX_PARTICLES_Spawn_Lava_Burn(Vec3f * poss, Entity * io) {
	
	Vec3f pos = *poss;
	
	if(io && io->obj && !io->obj->facelist.empty()) {
		size_t num = 0;
		long notok = 10;
		while(notok--) {
			num = Random::get(0, io->obj->facelist.size() - 1);
			if(io->obj->facelist[num].facetype & POLY_HIDE) {
				continue;
			}
			if(EEfabs(pos.y-io->obj->vertexlist3[io->obj->facelist[num].vid[0]].v.y) > 50.f) {
				continue;
			}
			notok = 0;
		}
		// TODO notok is always -1 here!
		if(notok >= 0) {
			pos = io->obj->vertexlist3[io->obj->facelist[num].vid[0]].v;
		}
	}
	
	PARTICLE_DEF * pd = createParticle();
	if(!pd) {
		return;
	}
	
	pd->ov = pos;
	pd->move = Vec3f(rnd() * 2.f - 4.f, rnd() * -12.f - 15.f, rnd() * 2.f - 4.f);
	pd->tolive = 800;
	pd->tc = smokeparticle;
	pd->siz = 15.f;
	pd->scale = randomVec(15.f, 20.f);
	pd->special = FIRE_TO_SMOKE;
	if(rnd() > 0.5f) {
		pd->special |= SUBSTRACT;
	}
}

static void ARX_PARTICLES_Spawn_Rogue_Blood(const Vec3f & pos, float dmgs, Color col) {
	
	PARTICLE_DEF * pd = createParticle();
	if(!pd) {
		return;
	}
	
	pd->ov = pos;
	pd->siz = 3.1f * (dmgs * (1.f / 60) + .9f);
	pd->scale = Vec3f(-pd->siz * 0.25f);
	pd->special = PARTICLE_SUB2 | SUBSTRACT | GRAVITY | ROTATING | MODULATE_ROTATION
	              | SPLAT_GROUND;
	pd->tolive = 1600;
	pd->move = Vec3f(rnd() * 60.f - 30.f, rnd() * -10.f - 15.f, rnd() * 60.f - 30.f);
	pd->rgb = col.to<float>();
	long num = Random::get(0, 5);
	pd->tc = bloodsplat[num];
	pd->fparam = rnd() * (1.f/10) - .05f;
	
}

static void ARX_PARTICLES_Spawn_Blood3(const Vec3f & pos, float dmgs, Color col,
                                       long flags = 0) {
	
	PARTICLE_DEF * pd = createParticle();
	if(pd) {
		
		float power = (dmgs * (1.f/60)) + .9f;
		pd->ov = pos + Vec3f(-sin(float(arxtime) * 0.001f), sin(float(arxtime) * 0.001f),
		               cos(float(arxtime) * 0.001f)) * 30.f;
		pd->siz = 3.5f * power + sin(float(arxtime) * (1.f/1000));
		pd->scale = Vec3f(-pd->siz * 0.5f);
		pd->special = PARTICLE_SUB2 | SUBSTRACT | GRAVITY | ROTATING | MODULATE_ROTATION
		              | flags;
		pd->tolive = 1100;
		pd->rgb = col.to<float>();
		pd->tc = bloodsplatter;
		pd->fparam = rnd() * 0.1f - .05f;
	}
	
	if(rnd() > .90f) {
		ARX_PARTICLES_Spawn_Rogue_Blood(pos, dmgs, col);
	}
	
}

void ARX_POLYSPLAT_Add(Vec3f * poss, Color3f * col, float size, long flags) {
	
	if(polyboom.size() > (MAX_POLYBOOM >> 2) - 30)
		return;

	if(polyboom.size() > 250 && size < 10)
		return;

	float splatsize=90;

	if(size > 40.f)
		size = 40.f;

	size *= 0.75f;

	switch(config.video.levelOfDetail) {
		case 2:
			if(polyboom.size() > 160)
				return;

			splatsize = 90;
			size *= 1.f;
		break;
		case 1:
			if(polyboom.size() > 60)
				return;

			splatsize = 60;
			size *= 0.5f;
		break;
		default:
			if(polyboom.size() > 10)
				return;

			splatsize = 30;
			size *= 0.25f;
		break;
	}


	float py;
	EERIEPOLY *ep = CheckInPoly(poss->x, poss->y-40, poss->z, &py);

	if(!ep)
		return;
	
	if(flags & 1)
		py=poss->y;

	EERIEPOLY TheoricalSplat; // clockwise
	TheoricalSplat.v[0].p.x=-splatsize;
	TheoricalSplat.v[0].p.y = py; 
	TheoricalSplat.v[0].p.z=-splatsize;

	TheoricalSplat.v[1].p.x=-splatsize;
	TheoricalSplat.v[1].p.y = py; 
	TheoricalSplat.v[1].p.z=+splatsize;

	TheoricalSplat.v[2].p.x=+splatsize;
	TheoricalSplat.v[2].p.y = py; 
	TheoricalSplat.v[2].p.z=+splatsize;

	TheoricalSplat.v[3].p.x=+splatsize;
	TheoricalSplat.v[3].p.y = py; 
	TheoricalSplat.v[3].p.z=-splatsize;
	TheoricalSplat.type=POLY_QUAD;

	Vec3f RealSplatStart(-size, py, -size);

	TheoricalSplat.v[0].p.x+=poss->x;
	TheoricalSplat.v[0].p.z+=poss->z;

	TheoricalSplat.v[1].p.x+=poss->x;
	TheoricalSplat.v[1].p.z+=poss->z;

	TheoricalSplat.v[2].p.x+=poss->x;
	TheoricalSplat.v[2].p.z+=poss->z;

	TheoricalSplat.v[3].p.x+=poss->x;
	TheoricalSplat.v[3].p.z+=poss->z;
	
	RealSplatStart.x+=poss->x;
	RealSplatStart.z+=poss->z;

	float hdiv,vdiv;
	hdiv=vdiv=1.f/(size*2);

	unsigned long tim = (unsigned long)(arxtime);

	std::vector<POLYBOOM>::iterator pb = polyboom.begin();
	while(pb != polyboom.end()) {

		//TODO what does this do ?
		pb->type |= 128;
		++ pb;
	}

	long px = poss->x * ACTIVEBKG->Xmul;
	long pz = poss->z * ACTIVEBKG->Zmul;

	long x0 = clamp(px - 3, 0, ACTIVEBKG->Xsize - 1);
	long x1 = clamp(px + 3, 0, ACTIVEBKG->Xsize - 1);
	long z0 = clamp(pz - 3, 0, ACTIVEBKG->Zsize - 1);
	long z1 = clamp(pz + 3, 0, ACTIVEBKG->Zsize - 1);

	for(long j = z0; j <= z1; j++)
	for(long i = x0; i <= x1; i++) {
		EERIE_BKG_INFO *eg = &ACTIVEBKG->Backg[i + j * ACTIVEBKG->Xsize];

		for(long l = 0; l < eg->nbpolyin; l++) {
			EERIEPOLY *ep = eg->polyin[l];

			if((flags & 2) && !(ep->type & POLY_WATER))
				continue;

			if((ep->type & POLY_TRANS) && !(ep->type & POLY_WATER))
				continue;

			long nbvert = (ep->type & POLY_QUAD) ? 4 : 3;

			bool oki = false;

			for(long k = 0; k < nbvert; k++) {
				if(PointIn2DPolyXZ(&TheoricalSplat, ep->v[k].p.x, ep->v[k].p.z)
					&& fabs(ep->v[k].p.y-py) < 100.f)
				{
					oki = true;
					break;
				}

				if(PointIn2DPolyXZ(&TheoricalSplat, (ep->v[k].p.x+ep->center.x) * 0.5f, (ep->v[k].p.z+ep->center.z) * 0.5f)
					&& fabs(ep->v[k].p.y-py) < 100.f)
				{
					oki = true;
					break;
				}
			}

			if(!oki && PointIn2DPolyXZ(&TheoricalSplat, ep->center.x, ep->center.z) && EEfabs(ep->center.y-py) < 100.f)
				oki = true;

			if(oki) {

				if(polyboom.capacity() > 0) {
					POLYBOOM pb;

					if(flags & 2) {
						pb.type = 2;

						long num = Random::get(0, 2);
						pb.tc = water_splat[num];

						pb.tolive=1500;
					} else {
						pb.type = 1;

						long num = Random::get(0, 5);
						pb.tc = bloodsplat[num];

						pb.tolive=(long)(float)(16000 * size * (1.0f/40));
					}

					pb.ep=ep;
					
					pb.timecreation=tim;

					pb.tx = static_cast<short>(i);
					pb.tz = static_cast<short>(j);

					pb.rgb = *col;

					for(int k = 0; k < nbvert; k++) {
						float vdiff=EEfabs(ep->v[k].p.y-RealSplatStart.y);
						pb.u[k]=(ep->v[k].p.x-RealSplatStart.x)*hdiv;

						if(pb.u[k]<0.5f)
							pb.u[k]-=vdiff*hdiv;
						else
							pb.u[k]+=vdiff*hdiv;

						pb.v[k]=(ep->v[k].p.z-RealSplatStart.z)*vdiv;

						if(pb.v[k]<0.5f)
							pb.v[k]-=vdiff*vdiv;
						else
							pb.v[k]+=vdiff*vdiv;
					}

					pb.nbvert=(short)nbvert;

					polyboom.push_back(pb);
				}
			}
		}			
	}	
}

void SpawnGroundSplat(EERIE_SPHERE * sp, Color3f * rgb, float size, long flags) {
	ARX_POLYSPLAT_Add(&sp->origin, rgb, size, flags);
}

void ARX_PARTICLES_Spawn_Blood2(const Vec3f & pos, float dmgs, Color col, Entity * io) {
	
	bool isNpc = io && (io->ioflags & IO_NPC);
	
	if(isNpc && io->_npcdata->SPLAT_TOT_NB) {
		
		if(io->_npcdata->SPLAT_DAMAGES < 3) {
			return;
		}
		
		float power = (io->_npcdata->SPLAT_DAMAGES * (1.f/60)) + .9f;
		
		Vec3f vect = pos - io->_npcdata->last_splat_pos;
		float dist = glm::length(vect);
		vect = glm::normalize(vect);
		long nb = long(dist / 4.f * power);
		if(nb == 0) {
			nb = 1;
		}
		
		long MAX_GROUND_SPLATS;
		switch(config.video.levelOfDetail) {
			case 2:  MAX_GROUND_SPLATS = 10; break;
			case 1:  MAX_GROUND_SPLATS = 5; break;
			default: MAX_GROUND_SPLATS = 1; break;
		}
		
		for(long k = 0; k < nb; k++) {
			Vec3f posi = io->_npcdata->last_splat_pos + vect * Vec3f(k * 4.f * power);
			io->_npcdata->SPLAT_TOT_NB++;
			if(io->_npcdata->SPLAT_TOT_NB > MAX_GROUND_SPLATS) {
				ARX_PARTICLES_Spawn_Blood3(posi, io->_npcdata->SPLAT_DAMAGES, col, SPLAT_GROUND);
				io->_npcdata->SPLAT_TOT_NB=1;
			} else {
				ARX_PARTICLES_Spawn_Blood3(posi, io->_npcdata->SPLAT_DAMAGES, col);
			}
		}
		
	} else {
		if(isNpc) {
			io->_npcdata->SPLAT_DAMAGES = (short)dmgs;
		}
		ARX_PARTICLES_Spawn_Blood3(pos, dmgs, col, SPLAT_GROUND);
		if(isNpc) {
			io->_npcdata->SPLAT_TOT_NB = 1;
		}
	}
	
	if(isNpc) {
		io->_npcdata->last_splat_pos = pos;
	}
}

void ARX_PARTICLES_Spawn_Blood(Vec3f * pos, float dmgs, long source) {
	
	if(source < 0) {
		return;
	}
	
	float nearest_dist = std::numeric_limits<float>::max();
	long nearest = -1;
	long count = entities[source]->obj->nbgroups;
	for(long i = 0; i < count; i += 2) {
		long vertex = entities[source]->obj->grouplist[i].origin;
		float dist = glm::distance2(*pos, entities[source]->obj->vertexlist3[vertex].v);
		if(dist < nearest_dist) {
			nearest_dist = dist;
			nearest = i;
		}
	}
	if(nearest < 0) {
		return;
	}
	
	// Decides number of blood particles...
	long spawn_nb = clamp(long(dmgs * 2.f), 5l, 26l);
	
	long totdelay = 0;
	
	for(long k = 0; k < spawn_nb; k++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			return;
		}
		
		pd->siz = 0.f;
		pd->scale = Vec3f(float(spawn_nb));
		pd->special = GRAVITY | ROTATING | MODULATE_ROTATION | DELAY_FOLLOW_SOURCE;
		pd->source = &entities[source]->obj->vertexlist3[nearest].v;
		pd->sourceionum = source;
		pd->tolive = 1200 + spawn_nb * 5;
		totdelay += 45 + Random::get(0, 150 - spawn_nb);
		pd->delay = totdelay;
		pd->rgb = Color3f(.9f, 0.f, 0.f);
		pd->tc = bloodsplatter;
		pd->fparam = rnd() * 0.1f - 0.05f;
	}
}

long SPARK_COUNT = 0;

// flag & 1 punch failed
// flag & 2 punch success
void ARX_PARTICLES_Spawn_Spark(Vec3f * pos, float dmgs, long flags) {
	
	if(!pos) {
		return;
	}

	long spawn_nb = dmgs;
	
	if(SPARK_COUNT < 1000) {
		SPARK_COUNT += spawn_nb * 25;
	} else {
		SPARK_COUNT -= static_cast<long>(framedelay);
		return;
	}
	
	for(long k = 0; k < spawn_nb; k++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			return;
		}
		
		pd->oldpos = pd->ov = *pos + randomVec(-5.f, 5.f);
		pd->siz = 2.f;
		pd->move = randomVec(-6.f, 6.f);
		
		pd->special = PARTICLE_SPARK;
		float len = clamp(spawn_nb * (1.f / 3), 3.f, 8.f);
		pd->tolive = (unsigned long)(len * 90.f + float(spawn_nb));
		
		if(flags == 0) {
			pd->rgb = Color3f(.3f, .3f, 0.f);
		} else if(flags & 1) {
			pd->rgb = Color3f(.2f, .2f, .1f);
		} else if(flags & 2) {
			pd->rgb = Color3f(.45f, .1f, 0.f);
		}
		
		pd->fparam = len + rnd() * len; // Spark tail length
	}
}

void MakeCoolFx(Vec3f * pos) {
	ARX_BOOMS_Add(pos,1);
}

void MakePlayerAppearsFX(Entity * io) {
	MakeCoolFx(&io->pos);
	MakeCoolFx(&io->pos);
	AddRandomSmoke(io, 30);
	ARX_PARTICLES_Add_Smoke(&io->pos, 1 | 2, 20); // flag 1 = randomize pos
}

void AddRandomSmoke(Entity * io, long amount) {
	
	if(!io) {
		return;
	}
	
	for(long i = 0; i < amount; i++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			return;
		}
		
		long vertex = Random::get(0, io->obj->vertexlist.size());
		pd->ov = io->obj->vertexlist3[vertex].v + randomVec(-5.f, 5.f);
		pd->siz = rnd() * 8.f;
		if(pd->siz < 4.f) {
			pd->siz = 4.f;
		}
		pd->scale = Vec3f(10.f);
		pd->special = ROTATING | MODULATE_ROTATION | FADE_IN_AND_OUT;
		pd->tolive = Random::get(900, 1300);
		pd->move = Vec3f(0.25f - 0.5f * rnd(), -1.f * rnd() + 0.3f, 0.25f - 0.5f * rnd());
		pd->rgb = Color3f(0.3f, 0.3f, 0.34f);
		pd->tc = smokeparticle;
		pd->fparam = 0.001f;
	}
}

// flag 1 = randomize pos
void ARX_PARTICLES_Add_Smoke(Vec3f * pos, long flags, long amount, Color3f * rgb) {
	
	Vec3f mod = (flags & 1) ? randomVec(-50.f, 50.f) : Vec3f_ZERO;
	
	while(amount--) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			return;
		}
		
		pd->ov = *pos + mod;
		if(flags & 2) {
			pd->siz = rnd() * 20.f + 15.f;
			pd->scale = randomVec(40.f, 55.f);
		} else {
			pd->siz = std::max(4.f, rnd() * 8.f + 5.f);
			pd->scale = randomVec(10.f, 15.f);
		}
		pd->special = ROTATING | MODULATE_ROTATION | FADE_IN_AND_OUT;
		pd->tolive = Random::get(1100, 1500);
		pd->delay = amount * 120 + Random::get(0, 100);
		pd->move = Vec3f(0.25f - 0.5f * rnd(), -1.f * rnd() + 0.3f, 0.25f - 0.5f * rnd());
		pd->rgb = (rgb) ? *rgb : Color3f(0.3f, 0.3f, 0.34f);
		pd->tc = smokeparticle;
		pd->fparam = 0.01f;
	}
}

extern long cur_mr;

void ManageTorch() {
	
	EERIE_LIGHT * el = &DynLight[0];
	
	if(player.torch) {
		
		float rr = rnd();
		el->pos = player.pos;
		el->intensity = 1.6f;
		el->fallstart = 280.f + rr * 20.f;
		el->fallend = el->fallstart + 280.f;
		el->exist = 1;
		el->rgb = Color3f(Project.torch.r - rr * 0.1f, Project.torch.g - rr * 0.1f,
		                  Project.torch.b - rr * 0.1f);
		el->duration = 0;
		el->extras = 0;
		
	} else if(cur_mr == 3) {
		
		el->pos = player.pos;
		el->intensity = 1.8f;
		el->fallstart = 480.f;
		el->fallend = el->fallstart + 480.f; 
		el->exist = 1;
		el->rgb = Color3f(1.f, .5f, .8f);
		el->duration = 0;
		el->extras = 0;
		
	} else {
		
		if(flarenum == 0) {
			el->exist = 0;
		} else {
			
			long count = MagicFlareCountNonFlagged();
			
			if(count) {
				float rr = rnd();
				el->pos = player.pos;
				el->fallstart = 140.f + float(count) * 0.333333f + rr * 5.f;
				el->fallend = 220.f + float(count) * 0.5f + rr * 5.f;
				el->intensity = 1.6f;
				el->exist = 1;
				el->rgb = Color3f(0.01f * count, 0.009f * count, 0.008f * count);
			}
		}
	}
	
	if(entities.size() > 0 && entities.player() && entities.player()->obj
	   && entities.player()->obj->fastaccess.head_group_origin > -1) {
		short vertex = entities.player()->obj->fastaccess.head_group_origin;
		el->pos.y = entities.player()->obj->vertexlist3[vertex].v.y;
	}
}

void ARX_BOOMS_ClearAllPolyBooms() {
	polyboom.clear();
}

void ARX_BOOMS_Add(Vec3f * poss,long type) {
	
	PARTICLE_DEF * pd = createParticle(true);
	if(pd) {
		
		static TextureContainer * tc1 = TextureContainer::Load("graph/particles/fire_hit");
		
		pd->ov = *poss;
		pd->move = Vec3f(3.f - 6.f * rnd(), 4.f - 12.f * rnd(), 3.f - 6.f * rnd());
		pd->tolive = Random::get(600, 700);
		pd->tc = tc1;
		pd->siz = (100.f + 10.f * rnd()) * ((type == 1) ? 2.f : 1.f);
		pd->zdec = true;
		if(type == 1) {
			pd->rgb = Color3f(.4f, .4f, 1.f);
		}
		
		pd = createParticle(true);
		if(pd) {
			pd->ov = *poss;
			pd->move = Vec3f(3.f - 6.f * rnd(), 4.f - 12.f * rnd(), 3.f - 6.f * rnd());
			pd->tolive = Random::get(600, 700);
			pd->tc = tc1;
			pd->siz = (40.f + 30.f * rnd()) * ((type == 1) ? 2.f : 1.f);
			pd->zdec = true;
			if(type == 1) {
				pd->rgb = Color3f(.4f, .4f, 1.f);
			}
		}
		
	}
	
	static TextureContainer * tc2 = TextureContainer::Load("graph/particles/boom");
	
	// TODO was F2L at some point - should this be rounded?
	long x0 = long(poss->x * ACTIVEBKG->Xmul) - 3;
	long z0 = long(poss->z * ACTIVEBKG->Zmul) - 3;
	long x1 = x0 + 6;
	long z1 = z0 + 6;
	x0 = clamp(x0, 0l, ACTIVEBKG->Xsize - 1l);
	x1 = clamp(x1, 0l, ACTIVEBKG->Xsize - 1l);
	z0 = clamp(z0, 0l, ACTIVEBKG->Zsize - 1l);
	z1 = clamp(z1, 0l, ACTIVEBKG->Zsize - 1l);
	
	for(long j = z0; j <= z1; j++) for(long i = x0; i <= x1;i++) {
		EERIE_BKG_INFO & eg = ACTIVEBKG->Backg[i + j * ACTIVEBKG->Xsize];
		for(long l = 0; l < eg.nbpoly; l++) {
			EERIEPOLY * ep = &eg.polydata[l];
			
			if((ep->type & POLY_TRANS) && !(ep->type & POLY_WATER)) {
				continue;
			}
			
			long nbvert = (ep->type & POLY_QUAD) ? 4 : 3;
			
			float temp_uv1[4];
			
			bool dod = true;
			for(long k = 0; k < nbvert; k++) {
				float ddd = fdist(ep->v[k].p, *poss);
				if(ddd > BOOM_RADIUS) {
					dod = false;
					break;
				} else {
					temp_uv1[k] = 0.5f - ddd * (0.5f / BOOM_RADIUS);
				}
			}
			if(!dod) {
				continue;
			}
			
			if(polyboom.capacity() == 0) {
				continue;
			}
			
			POLYBOOM pb;
			
			pb.type = 0;
			pb.ep = ep;
			pb.tc = tc2;
			pb.tolive = 10000;
			pb.timecreation = long(arxtime);
			pb.tx = short(i);
			pb.tz = short(j);
			for(int k = 0; k < nbvert; k++) {
				pb.v[k] = pb.u[k] = temp_uv1[k];
			}
			pb.nbvert = short(nbvert);

			polyboom.push_back(pb);
		}
	}
}

void Add3DBoom(Vec3f * position) {
	
	Vec3f poss = *position;
	ARX_SOUND_PlaySFX(SND_SPELL_FIRE_HIT, &poss);
	
	float dist = fdist(player.pos - Vec3f(0, 160.f, 0.f), *position);
	if(dist < 300) {
		Vec3f vect = (player.pos - *position - Vec3f(0.f, 160.f, 0.f)) / dist;
		player.physics.forces += vect * ((300.f - dist) * 0.0125f);
	}
	
	for(size_t i = 0; i < entities.size(); i++) {
		
		Entity * entity = entities[i];
		if(!entity || entity->show != 1 || !(entity->ioflags & IO_ITEM)) {
			continue;
		}
		
		if(!entity->obj || !entity->obj->pbox) {
			continue;
		}
		
		for(long k = 0; k < entity->obj->pbox->nb_physvert; k++) {
			float dist = fdist(entity->obj->pbox->vert[k].pos, *position);
			if(dist < 300.f) {
				entity->obj->pbox->active = 1;
				entity->obj->pbox->stopcount = 0;
				Vec3f vect = (entity->obj->pbox->vert[k].pos - *position) / dist;
				entity->obj->pbox->vert[k].velocity += vect * ((300.f - dist) * 10.f);
			}
		}
	}
}

void ARX_PARTICLES_FirstInit() {
	
	smokeparticle = TextureContainer::Load("graph/particles/smoke");
	
	// TODO bloodsplat and water_splat cannot use mipmapping because they need a constant color border pixel
	// this may also apply to other textures
	
	bloodsplat[0] = TextureContainer::Load("graph/particles/new_blood", TextureContainer::NoMipmap);
	bloodsplat[1] = TextureContainer::Load("graph/particles/new_blood_splat1", TextureContainer::NoMipmap);
	bloodsplat[2] = TextureContainer::Load("graph/particles/new_blood_splat2", TextureContainer::NoMipmap);
	bloodsplat[3] = TextureContainer::Load("graph/particles/new_blood_splat3", TextureContainer::NoMipmap);
	bloodsplat[4] = TextureContainer::Load("graph/particles/new_blood_splat4", TextureContainer::NoMipmap);
	bloodsplat[5] = TextureContainer::Load("graph/particles/new_blood_splat5", TextureContainer::NoMipmap);
	bloodsplatter = bloodsplat[0];
	
	water_splat[0] = TextureContainer::Load("graph/particles/[fx]_water01", TextureContainer::NoMipmap);
	water_splat[1] = TextureContainer::Load("graph/particles/[fx]_water02", TextureContainer::NoMipmap);
	water_splat[2] = TextureContainer::Load("graph/particles/[fx]_water03", TextureContainer::NoMipmap);
	
	water_drop[0]=TextureContainer::Load("graph/particles/[fx]_water_drop01");
	water_drop[1]=TextureContainer::Load("graph/particles/[fx]_water_drop02");
	water_drop[2]=TextureContainer::Load("graph/particles/[fx]_water_drop03");
	healing=TextureContainer::Load("graph/particles/heal_0005");
	tzupouf=TextureContainer::Load("graph/obj3d/textures/(fx)_tsu_greypouf");
	fire2=TextureContainer::Load("graph/particles/fire2");
}

void ARX_PARTICLES_ClearAll() {
	memset(particle, 0, sizeof(PARTICLE_DEF) * MAX_PARTICLES);
	ParticleCount = 0;
}

PARTICLE_DEF * createParticle(bool allocateWhilePaused) {
	
	if(!allocateWhilePaused && arxtime.is_paused()) {
		return NULL;
	}
	
	for(size_t i = 0; i < MAX_PARTICLES; i++) {
		
		PARTICLE_DEF * pd = &particle[i];
		
		if(pd->exist) {
			continue;
		}
		
		ParticleCount++;
		pd->exist = true;
		pd->timcreation = long(arxtime);
		
		pd->type = 0;
		pd->rgb = Color3f::white;
		pd->tc = NULL;
		pd->special = 0;
		pd->source = NULL;
		pd->delay = 0;
		pd->zdec = false;
		pd->move = Vec3f_ZERO;
		pd->scale = Vec3f_ONE;
		
		return pd;
	}
	
	return NULL;
}

void MagFX(const Vec3f & pos) {
	
	PARTICLE_DEF * pd	=	createParticle();
	if(!pd) {
		return;
	}
	
	pd->ov = pos + Vec3f(rnd() * 6.f - rnd() * 12.f, rnd() * 6.f-rnd() * 12.f, 0.f);
	pd->move = Vec3f(6.f - rnd() * 12.f, -8.f + rnd() * 16.f, 0.f);
	pd->scale = Vec3f(4.4f, 4.4f, 1.f);
	pd->tolive = Random::get(1500, 2400);
	pd->tc = healing;
	pd->rgb = Color3f::magenta;
	pd->siz = 56.f;
	pd->type = PARTICLE_2D;
}

void MakeBookFX(const Vec3f & pos) {
	
	for(long i = 0; i < 12; i++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			break;
		}
		
		pd->ov = pos + Vec3f(rnd() * 6.f - rnd() * 12.f, rnd() * 6.f - rnd() * 12.f, 0.f);
		pd->move = Vec3f(6.f - rnd() * 12.f, -8.f + rnd() * 16.f, 0.f);
		pd->scale = Vec3f(4.4f, 4.4f, 1.f);
		pd->tolive = Random::get(1500, 2400);
		pd->tc = healing;
		pd->rgb = Color3f::magenta;
		pd->siz = 56.f;
		pd->type = PARTICLE_2D;
	}
	
	for(int i = 0; i < 5; i++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			break;
		}
		
		pd->ov = pos - Vec3f(float(i * 2), float(i * 2), 0.f);
		pd->move = Vec3f(-float(i) * 0.5f, -float(i) * 0.5f, 0.f);
		pd->scale = Vec3f(float(i * 10), float(i * 10), 0.f);
		pd->tolive = Random::get(1200, 1600);
		pd->tc = ITC.Get("book");
		pd->rgb = Color3f(1.f - float(i) * 0.1f, float(i) * 0.1f, 0.5f - float(i) * 0.1f);
		pd->siz = 32.f + float(i * 4);
		pd->type = PARTICLE_2D;
	}
	
	NewSpell = 1;
}

void createSphericalSparks(const Vec3f & pos, float r, TextureContainer * tc,
                           const Color3f & color, int mask) {
	
	int nb = Random::get(0, 31);
	for(int i = 0; i < nb; i++) {
		
		PARTICLE_DEF * pd = createParticle(true);
		if(!pd) {
			return;
		}
		
		float a = radians(rnd() * 360.f);
		float b = radians(rnd() * 360.f);
		pd->type = PARTICLE_SPARK2;
		pd->special = GRAVITY;
		pd->ov = pd->oldpos = pos;
		pd->move = Vec3f(EEsin(a) * EEcos(b), EEsin(a) * EEsin(b), EEcos(a)) * r;
		pd->tolive = Random::get(1000, 1500);
		pd->rgb = color;
		pd->tc = tc;
		pd->mask = mask;
	}
}

void ARX_PARTICLES_Spawn_Splat(const Vec3f & pos, float dmgs, Color col) {
	
	float power = (dmgs * (1.f / 60)) + .9f;
	
	for(long kk = 0; kk < 20; kk++) {
		
		PARTICLE_DEF * pd = createParticle(true);
		if(!pd) {
			return;
		}
		
		pd->special = PARTICLE_SUB2 | SUBSTRACT | GRAVITY;
		pd->ov = pos;
		pd->move = randomVec(-11.5f, 11.5f);
		pd->tolive = (unsigned long)(1000 + dmgs*3);
		pd->tc = blood_splat;
		pd->siz = 0.3f + 0.01f * power;
		pd->scale = Vec3f(0.2f + 0.3f * power);
		pd->zdec = true;
		pd->rgb = col.to<float>();
	}
}

void ARX_PARTICLES_SpawnWaterSplash(const Vec3f * _ePos) {
	
	long nbParticles = Random::get(15, 35);
	for(long kk=0; kk < nbParticles; kk++) {
		
		PARTICLE_DEF * pd = createParticle(true);
		if(!pd) {
			return;
		}
		
		pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING
		              | GRAVITY | SPLAT_WATER;
		pd->ov = *_ePos + Vec3f(30.f * rnd(), -20.f * rnd(), 30.f * rnd());
		pd->move = Vec3f(6.5f * frand2(), -11.5f * rnd(), 6.5f * frand2());
		pd->tolive = Random::get(1000, 1300);
		
		int t = Random::get(0, 2);
		pd->tc = water_drop[t];
		pd->siz = 0.4f;
		float s = rnd();
		pd->zdec = true;
		pd->rgb = Color3f::gray(s);
	}
}

void SpawnFireballTail(Vec3f * poss, Vec3f * vecto, float level, long flags) {
	
	if(!explo[0]) {
		return;
	}
	
	for(long nn = 0; nn < 2; nn++) {
		
		PARTICLE_DEF * pd = createParticle(true);
		if(!pd) {
			return;
		}
		
		pd->special = FIRE_TO_SMOKE | FADE_IN_AND_OUT | PARTICLE_ANIMATED | ROTATING
		              | MODULATE_ROTATION;
		pd->fparam = 0.02f - rnd() * 0.02f;
		pd->move = Vec3f(0.f, -rnd() * 3.f, 0.f);
		pd->tc = explo[0];
		pd->rgb = Color3f::gray(.7f);
		pd->siz = (level + rnd()) * 2.f;
		
		if(flags & 1) {
			pd->tolive = Random::get(400, 500);
			pd->siz *= 0.7f;
			pd->scale = Vec3f(level * 1.4f);
		} else {
			pd->scale = Vec3f(level * 2.f);
			pd->tolive=Random::get(800, 900);
		}
		
		pd->cval1 = 0;
		pd->cval2 = MAX_EXPLO - 1;
		
		if(nn == 1) {
			pd->delay = Random::get(150, 250);
			pd->ov = *poss + *vecto * Vec3f(pd->delay);
		} else {
			pd->ov = *poss;
		}
	}
}

void LaunchFireballBoom(Vec3f * poss, float level, Vec3f * direction, Color3f * rgb) {
	
	level *= 1.6f;
	
	if(explo[0] == NULL) {
		return;
	}
	
	PARTICLE_DEF * pd = createParticle(true);
	if(!pd) {
		return;
	}
	
	pd->special = FIRE_TO_SMOKE | FADE_IN_AND_OUT | PARTICLE_ANIMATED;
	pd->ov = *poss;
	pd->move = (direction) ? *direction : Vec3f(0.f, -rnd() * 5.f, 0.f);
	pd->tolive = Random::get(1600, 2200);
	pd->tc = explo[0];
	pd->siz = level * 3.f + 2.f * rnd();
	pd->scale = Vec3f(level * 3.f);
	pd->zdec = true;
	pd->cval1 = 0;
	pd->cval2 = MAX_EXPLO - 1;
	if(rgb) {
		pd->rgb = *rgb;
	}
	
}

void ARX_PARTICLES_Render(EERIE_CAMERA * cam)  {
	
	if(!ACTIVEBKG) {
		return;
	}
	
	if(ParticleCount == 0) {
		return;
	}
	
	TexturedVertex in, inn, out;
	
	unsigned long tim = (unsigned long)arxtime;
	
	GRenderer->SetCulling(Renderer::CullNone);
	GRenderer->SetFogColor(Color::none);
	
	long pcc = ParticleCount;
	
	for(size_t i = 0; i < MAX_PARTICLES && pcc > 0; i++) {
		
		PARTICLE_DEF * part = &particle[i];
		if(!part->exist) {
			continue;
		}
		
		long framediff = part->timcreation + part->tolive - tim;
		long framediff2 = tim - part->timcreation;
		
		if(framediff2 < long(part->delay)) {
			continue;
		}
		
		if(part->delay > 0) {
			part->timcreation += part->delay;
			part->delay=0;
			if((part->special & DELAY_FOLLOW_SOURCE) && part->sourceionum >= 0
					&& entities[part->sourceionum]) {
				part->ov = *part->source;
				Entity * target = entities[part->sourceionum];
				Vec3f vector = (part->ov - target->pos) * Vec3f(1.f, 0.5f, 1.f);
				vector = glm::normalize(vector);
				part->move = vector * Vec3f(18.f, 5.f, 18.f) + randomVec(-0.5f, 0.5f);
				
			}
			continue;
		}
		
		if(!(part->type & PARTICLE_2D)) {

			FAST_BKG_DATA * bkgData = getFastBackgroundData(part->ov.x, part->ov.z);

			if(!bkgData || !bkgData->treat) {
				part->exist = false;
				ParticleCount--;
				continue;
			}
		}
		
		if(framediff <= 0) {
			if((part->special & FIRE_TO_SMOKE) && rnd() > 0.7f) {
				
				part->ov += part->move;
				part->tolive += (part->tolive / 4) + (part->tolive / 8);
				part->special &= ~FIRE_TO_SMOKE;
				part->tc = smokeparticle;
				part->scale *= 2.4f;
				if(part->scale.x < 0.f) {
					part->scale.x *= -1.f;
				}
				if(part->scale.y < 0.f) {
					part->scale.y *= -1.f;
				}
				if(part->scale.z < 0.f) {
					part->scale.z *= -1.f;
				}
				part->rgb = Color3f::gray(.45f);
				part->move *= 0.5f;
				part->siz *= 1.f / 3;
				part->special &= ~FIRE_TO_SMOKE;
				part->timcreation = tim;
				part->tc = smokeparticle;
				
				framediff = part->tolive;
				
			} else {
				part->exist = false;
				ParticleCount--;
				continue;
			}
		}
		
		if((part->special & FIRE_TO_SMOKE2)
				&& framediff2 > long(part->tolive - (part->tolive / 4))) {
			
			part->special &= ~FIRE_TO_SMOKE2;
		
			PARTICLE_DEF * pd = createParticle(true);
			if(pd) {
				*pd = *part;
				pd->timcreation = tim;
				pd->zdec = false;
				pd->special |= SUBSTRACT;
				pd->ov = part->oldpos;
				pd->tc = tzupouf;
				pd->scale *= 4.f;
				if(pd->scale.x < 0.f) {
					pd->scale.x *= -1.f;
				}
				if(pd->scale.y < 0.f) {
					pd->scale.y *= -1.f;
				}
				if(pd->scale.z < 0.f) {
					pd->scale.z *= -1.f;
				}
				pd->rgb = Color3f::white;
				pd->move *= 0.5f;
				pd->siz *= 1.f / 3;
			}
		}
		
		float val = (part->tolive - framediff) * 0.01f;
		
		if((part->special & FOLLOW_SOURCE) && part->sourceionum >= 0
				&& entities[part->sourceionum]) {
			inn.p = in.p = *part->source;
		} else if((part->special & FOLLOW_SOURCE2) && part->sourceionum >= 0
							&& entities[part->sourceionum]) {
			inn.p = in.p = *part->source + part->move * val;
		} else {
			inn.p = in.p = part->ov + part->move * val;
		}
		
		if(part->special & GRAVITY) {
			in.p.y = inn.p.y = inn.p.y + 1.47f * val * val;
		}
		
		if(part->special & PARTICLE_NOZBUFFER) {
			GRenderer->SetRenderState(Renderer::DepthTest, false);
		} else {
			GRenderer->SetRenderState(Renderer::DepthTest, true);
		}
		
		float fd = float(framediff2) / float(part->tolive);
		float r = 1.f - fd;
		if(part->special & FADE_IN_AND_OUT) {
			long t = part->tolive / 2;
			if(framediff2 <= t) {
				r = float(framediff2) / float(t);
			} else {
				r = 1.f - float(framediff2 - t) / float(t);
			}
		}
		
		if(!(part->type & PARTICLE_2D)) {
			
			EERIE_SPHERE sp;
			sp.origin = in.p;
			EE_RTP(&inn, &out);
			if(out.rhw < 0 || out.p.z > cam->cdepth * fZFogEnd) {
				continue;
			}
			
			if(part->special & PARTICLE_SPARK) {
				
				if(part->special & NO_TRANS) {
					GRenderer->SetRenderState(Renderer::AlphaBlending, false);
				} else {
					GRenderer->SetRenderState(Renderer::AlphaBlending, true);
					if(part->special & SUBSTRACT) {
						GRenderer->SetBlendFunc(Renderer::BlendZero, Renderer::BlendInvSrcColor);
					} else {
						GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
					}
				}
				
				GRenderer->SetCulling(Renderer::CullNone);
				Vec3f vect = part->oldpos - in.p;
				fnormalize(vect);
				TexturedVertex tv[3];
				tv[0].color = part->rgb.toBGR();
				tv[1].color = 0xFF666666;
				tv[2].color = 0xFF000000;
				tv[0].p = out.p;
				tv[0].rhw = out.rhw;
				TexturedVertex temp;
				temp.p = in.p + Vec3f(rnd() * 0.5f, 0.8f, rnd() * 0.5f);
				EE_RTP(&temp, &tv[1]);
				temp.p = in.p + vect * part->fparam;
				
				EE_RTP(&temp, &tv[2]);
				GRenderer->ResetTexture(0);
				
				EERIEDRAWPRIM(Renderer::TriangleStrip, tv);
				if(!arxtime.is_paused()) {
					part->oldpos = in.p;
				}
				
				continue;
			}
			
			if(part->special & SPLAT_GROUND) {
				float siz = part->siz + part->scale.x * fd;
				sp.radius = siz * 10.f;
				if(CheckAnythingInSphere(&sp, 0, CAS_NO_NPC_COL)) {
					if(rnd() < 0.9f) {
						Color3f rgb = part->rgb;
						SpawnGroundSplat(&sp, &rgb, sp.radius, 0);
					}
					part->exist = false;
					ParticleCount--;
					continue;
				}
			}
			
			if(part->special & SPLAT_WATER) {
				float siz = part->siz + part->scale.x * fd;
				sp.radius = siz * (10.f + rnd() * 20.f);
				if(CheckAnythingInSphere(&sp, 0, CAS_NO_NPC_COL)) {
					if(rnd() < 0.9f) {
						Color3f rgb = part->rgb * 0.5f;
						SpawnGroundSplat(&sp, &rgb, sp.radius, 2);
					}
					part->exist = false;
					ParticleCount--;
					continue;
				}
			}
			
		}
		
		if((part->special & DISSIPATING) && out.p.z < 0.05f) {
			out.p.z *= 20.f;
			r *= out.p.z;
		}
		
		if(r <= 0.f) {
			pcc--;
			continue;
		}
		
		if(part->special & NO_TRANS) {
			GRenderer->SetRenderState(Renderer::AlphaBlending, false);
		} else {
			GRenderer->SetRenderState(Renderer::AlphaBlending, true);
			if(part->special & SUBSTRACT) {
				GRenderer->SetBlendFunc(Renderer::BlendZero, Renderer::BlendInvSrcColor);
			} else {
				GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
			}
		}
		
		Vec3f op = part->oldpos;
		if(!arxtime.is_paused()) {
			part->oldpos = in.p;
		}
		
		if(part->special & PARTICLE_GOLDRAIN) {
			float v = (rnd() - 0.5f) * 0.2f;
			if(part->rgb.r + v <= 1.f && part->rgb.r + v > 0.f
				&& part->rgb.g + v <= 1.f && part->rgb.g + v > 0.f
				&& part->rgb.b + v <= 1.f && part->rgb.b + v > 0.f) {
				part->rgb = Color3f(part->rgb.r + v, part->rgb.g + v, part->rgb.b + v);
			}
		}
		
		Color color = (part->rgb * r).to<u8>();
		if(Project.improve) {
			color.g = 0;
		}
		
		TextureContainer * tc = part->tc;
		if(tc == explo[0] && (part->special & PARTICLE_ANIMATED)) {
			long animrange = part->cval2 - part->cval1;
			long num = long(float(framediff2) / float(part->tolive) * animrange);
			num = clamp(num, part->cval1, part->cval2);
			tc = explo[num];
		}
		
		float siz = part->siz + part->scale.x * fd;
		
		if(part->special & ROTATING) {
			if(!(part->type & PARTICLE_2D)) {
				
				float rott;
				if(part->special & MODULATE_ROTATION) {
					rott = MAKEANGLE(float(tim + framediff2) * part->fparam);
				} else {
					rott = MAKEANGLE(float(tim + framediff2 * 2) * 0.25f);
				}
				
				float temp = (part->zdec) ? 0.0001f : 2.f;
				if(part->special & PARTICLE_SUB2) {
					TexturedVertex in2 = in;
					GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
					EERIEDrawRotatedSprite(&in, siz, tc, color, temp, rott);
					GRenderer->SetBlendFunc(Renderer::BlendZero, Renderer::BlendInvSrcColor);
					EERIEDrawRotatedSprite(&in2, siz, tc, Color::white, temp, rott);
				} else {
					EERIEDrawRotatedSprite(&in, siz, tc, color, temp, rott);
				}
				
			}
		} else if(part->type & PARTICLE_2D) {
			
			float siz2 = part->siz + part->scale.y * fd;
			if(part->special & PARTICLE_SUB2) {
				TexturedVertex in2 = in;
				GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
				EERIEDrawBitmap(in.p.x, in.p.y, siz, siz2, in.p.z, tc, color);
				GRenderer->SetBlendFunc(Renderer::BlendZero, Renderer::BlendInvSrcColor);
				EERIEDrawBitmap(in2.p.x, in.p.y, siz, siz2, in.p.z, tc, Color::white);
			} else {
				EERIEDrawBitmap(in.p.x, in.p.y, siz, siz2, in.p.z, tc, color);
			}
			
		} else if(part->type & PARTICLE_SPARK2) {
			
			Vec3f pos = in.p;
			Color col = (part->rgb * r).to<u8>();
			Vec3f end = pos - (pos - op) * 2.5f;
			Color masked = Color::fromBGRA(col.toBGRA() & part->mask);
			Draw3DLineTex2(end, pos, 2.f, masked, col);
			EERIEDrawSprite(&in, 0.7f, tc, col, 2.f);
			
		} else {
			
			float temp = (part->zdec) ? 0.0001f : 2.f;
			if(part->special & PARTICLE_SUB2) {
				TexturedVertex in2 = in;
				GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
				EERIEDrawSprite(&in, siz, tc, color, temp);
				GRenderer->SetBlendFunc(Renderer::BlendZero, Renderer::BlendInvSrcColor);
				EERIEDrawSprite(&in2, siz, tc, Color::white, temp);
			} else {
				EERIEDrawSprite(&in, siz, tc, color, temp);
			}
		}
		
		pcc--;
	}
	
	GRenderer->SetFogColor(ulBKGColor);
	GRenderer->SetRenderState(Renderer::DepthTest, true);
}

void RestoreAllLightsInitialStatus() {
	for(size_t i = 0; i < MAX_LIGHTS; i++) {
		if(GLight[i]) {
			GLight[i]->status = (GLight[i]->extras & EXTRAS_STARTEXTINGUISHED) ? 0 : 1;
			if(GLight[i]->status == 0) {
				if(ValidDynLight(GLight[i]->tl)) {
					DynLight[GLight[i]->tl].exist = 0;
				}
				GLight[i]->tl = -1;
			}
		}
	}
}

// Draws Flame Particles
void TreatBackgroundActions() {
	
	float fZFar = square(ACTIVECAM->cdepth * fZFogEnd * 1.3f);
	
	for(size_t i = 0; i < MAX_LIGHTS; i++) {
		
		EERIE_LIGHT * gl = GLight[i];
		if(!gl) {
			continue;
		}
		
		float dist = glm::distance2(gl->pos,	ACTIVECAM->orgTrans.pos);
		if(dist > fZFar) {
			// Out of treat range
			ARX_SOUND_Stop(gl->sample);
			gl->sample = audio::INVALID_ID;
			continue;
		}
		
		if((gl->extras & EXTRAS_SPAWNFIRE) && gl->status) {
			long id = ARX_DAMAGES_GetFree();
			if(id !=-1) {
				damages[id].radius = gl->ex_radius; 
				damages[id].damages = gl->ex_radius * (1.0f / 7);
				damages[id].area = DAMAGE_FULL;
				damages[id].duration = 1;
				damages[id].source = -5;
				damages[id].flags = 0; 
				damages[id].type = DAMAGE_TYPE_MAGICAL | DAMAGE_TYPE_FIRE | DAMAGE_TYPE_NO_FIX;
				damages[id].exist = true;
				damages[id].pos = gl->pos;
			}
		}
		
		if(!(gl->extras & (EXTRAS_SPAWNFIRE | EXTRAS_SPAWNSMOKE)) || !gl->status) {
			if(!gl->status && gl->sample != audio::INVALID_ID) {
				ARX_SOUND_Stop(gl->sample);
				gl->sample = audio::INVALID_ID;
			}
			continue;
		}
		
		if(gl->sample == audio::INVALID_ID) {
			gl->sample = SND_FIREPLACE;
			float pitch = 0.95f + 0.1f * rnd();
			ARX_SOUND_PlaySFX(gl->sample, &gl->pos, pitch, ARX_SOUND_PLAY_LOOPED);
		} else {
			ARX_SOUND_RefreshPosition(gl->sample, &gl->pos);
		}
		
		long count = 2;
		if(dist < square(ACTIVECAM->cdepth * (1.f / 6))) {
			count = 4;
		} else if(dist > square(ACTIVECAM->cdepth * (1.f / 3))) {
			count = 3;
		}
		
		for(long n = 0; n < count; n++) {
			
			if(rnd() < gl->ex_frequency) {
				PARTICLE_DEF * pd = createParticle();
				if(pd) {
					float t = rnd() * PI;
					Vec3f s = Vec3f(EEsin(t), EEsin(t), EEcos(t)) * randomVec();
					pd->ov = gl->pos + s * gl->ex_radius;
					pd->move = Vec3f(2.f - 4.f * rnd(), 2.f - 22.f * rnd(), 2.f - 4.f * rnd());
					pd->move *= gl->ex_speed;
					pd->siz = 7.f * gl->ex_size;
					pd->tolive = 500 + Random::get(0, 1000 * gl->ex_speed);
					if((gl->extras & EXTRAS_SPAWNFIRE) && (gl->extras & EXTRAS_SPAWNSMOKE)) {
						pd->special = FIRE_TO_SMOKE;
					}
					pd->tc = (gl->extras & EXTRAS_SPAWNFIRE) ? fire2 : smokeparticle;
					pd->special |= ROTATING | MODULATE_ROTATION;
					pd->fparam = 0.1f - rnd() * 0.2f * gl->ex_speed;
					pd->scale = Vec3f(-8.f);
					pd->rgb = (gl->extras & EXTRAS_COLORLEGACY) ? gl->rgb : Color3f::white;
				}
			}
			
			if(!(gl->extras & EXTRAS_SPAWNFIRE) || rnd() <= 0.95f) {
				continue;
			}
			
			if(rnd() < gl->ex_frequency) {
				PARTICLE_DEF * pd = createParticle();
				if(pd) {
					float t = rnd() * (PI * 2.f) - PI;
					Vec3f s = Vec3f(EEsin(t), EEsin(t), EEcos(t)) * randomVec();
					pd->ov = gl->pos + s * gl->ex_radius;
					Vec3f vect = glm::normalize(pd->ov - gl->pos);
					float d = (gl->extras & EXTRAS_FIREPLACE) ? 6.f : 4.f;
					pd->move = Vec3f(vect.x * d, -10.f - 8.f * rnd(), vect.z * d) * gl->ex_speed;
					pd->siz = 4.f * gl->ex_size * 0.3f;
					pd->tolive = 1200 + Random::get(0, 500 * gl->ex_speed);
					pd->tc = fire2;
					pd->special |= ROTATING | MODULATE_ROTATION | GRAVITY;
					pd->fparam = 0.1f - rnd() * 0.2f * gl->ex_speed;
					pd->scale = Vec3f(-3.f);
					pd->rgb = (gl->extras & EXTRAS_COLORLEGACY) ? gl->rgb : Color3f::white;
				}
			}
			
		}
	}
}


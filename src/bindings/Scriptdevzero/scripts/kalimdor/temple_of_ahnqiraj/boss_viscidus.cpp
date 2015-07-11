/* Copyright (C) 2006 - 2011 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Viscidus
SD%Complete: 50
SDComment: place holder
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "precompiled.h"

enum
{
    // Timer spells
    SPELL_POISON_SHOCK          = 25993,
    SPELL_POISONBOLT_VOLLEY     = 25991,					// doesn't ignore los like it should?
    SPELL_TOXIN                 = 26575,                    // Triggers toxin cloud every 10~~ sec
    SPELL_TOXIN_CLOUD           = 25989,					

	// Root
	SPELL_ROOT					= 20548,

    // Debuffs gained by the boss on frost damage
    SPELL_VISCIDUS_SLOWED       = 26034,
    SPELL_VISCIDUS_SLOWED_MORE  = 26036,
    SPELL_VISCIDUS_FREEZE       = 25937,

    // When frost damage exceeds a certain limit, then boss explodes
    SPELL_REJOIN_VISCIDUS       = 25896,
    SPELL_VISCIDUS_EXPLODE      = 25938,                    // Casts a lot of spells in the same time: 25865 to 25884; All spells have target coords
    SPELL_VISCIDUS_SUICIDE      = 26003,					// same as above?

	SPELL_MEMBRANE_VISCIDUS     = 25994,                    // damage reduction spell

    NPC_GLOB_OF_VISCIDUS        = 15667
};

struct MANGOS_DLL_DECL boss_viscidusAI : public ScriptedAI
{
    boss_viscidusAI(Creature* pCreature) : ScriptedAI(pCreature) 
	{
		m_creature->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_NATURE, true);
		Reset();
	}

	bool m_bSlowed1;
	bool m_bSlowed2;
	bool m_bFrozen;

	bool m_bCracking1;
	bool m_bCracking2;
	bool m_bExploded;

	bool m_bCanDoDamage;

	uint32 m_uiThawTimer;			// remove shatter

	uint8 m_uiFrostSpellCounter;
	uint8 m_uiMeleeCounter;
	uint8 m_uiGlobCount;

	uint32 m_uiSetInvisTimer;
	uint32 m_uiGlobSpawnTimer;
	uint32 m_uiPoisonShockTimer;
	uint32 m_uiPoisonVolleyTimer;
	uint32 m_uiToxicCloudTimer;

    void Reset()
    {
		m_uiGlobCount = 0;
		m_uiGlobSpawnTimer = 0;
		m_uiSetInvisTimer = 5000;
		m_uiPoisonVolleyTimer = 10000;									// timer confirmed
		m_uiPoisonShockTimer = 11000;
		m_uiToxicCloudTimer = urand(30000,40000);
		m_uiThawTimer = 20000;
		
		RemoveAuras();
		ResetBool(0);
		ResetBool(1);
		SetVisible(1);
    }

	void RemoveAuras()
	{
		if (m_creature->HasAura(SPELL_VISCIDUS_SLOWED))
			m_creature->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED);
		else if (m_creature->HasAura(SPELL_VISCIDUS_SLOWED_MORE))
			m_creature->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED_MORE);
		else if (m_creature->HasAura(SPELL_VISCIDUS_FREEZE))
			m_creature->RemoveAurasDueToSpell(SPELL_VISCIDUS_FREEZE);
	}

	void ResetBool(int Action = 0)
	{
		if(Action == 0)		// reset frost phase
		{
			m_bCanDoDamage = true;
			m_bSlowed1 = false;
			m_bSlowed2 = false;
			m_bFrozen = false;
			m_uiFrostSpellCounter = 0;
		}
		else if(Action == 1)	// reset melee phase
		{
			m_bCanDoDamage = true;
			m_bCracking1 = false;
			m_bCracking2 = false;
			m_bExploded = false;
			m_uiMeleeCounter = 0;
		}
	}
	
	void Aggro(Unit* /*pWho*/)
    {
    }

	void SpellHit(Unit* /*pCaster*/, const SpellEntry* pSpell)			// Count every frost spell
    {
        if (pSpell->School == SPELL_SCHOOL_FROST && !m_bFrozen && !m_bExploded)
		{
			++m_uiFrostSpellCounter;
			SpellCount();			// count incoming spells
		}
	}

	void DamageTaken(Unit* /*pDoneBy*/, uint32& uiDamage)			// Find a real way to distinguish the melee / phys from spells
	{
		if(uiDamage == SPELL_SCHOOL_MASK_NORMAL && m_bFrozen)
		{
			m_creature->GenericTextEmote("lol.", NULL, false);
			++m_uiMeleeCounter;
			MeleeHitCount();
		}
	}

	void SpellCount()
	{		
		if(m_uiFrostSpellCounter >= 1 && !m_bSlowed1)
		{
			m_creature->CastSpell(m_creature, SPELL_VISCIDUS_SLOWED_MORE, true);
			m_creature->GenericTextEmote("Viscidus begins to slow.", NULL, false);
			//m_creature->SetSpeedRate(MOVE_RUN, m_creature->GetSpeedRate(MOVE_RUN)*.85f);
			m_creature->SetAttackTime(BASE_ATTACK, m_creature->GetAttackTime(BASE_ATTACK)*.85f);
			m_bSlowed1 = true;
		}

		else if(m_uiFrostSpellCounter >= 5 && !m_bSlowed2)
		{
			m_creature->CastSpell(m_creature, SPELL_VISCIDUS_SLOWED_MORE, true);
			m_creature->GenericTextEmote("Viscidus is freezing up.", NULL, false);
			//m_creature->SetSpeedRate(MOVE_RUN, m_creature->GetSpeedRate(MOVE_RUN)*.7f);
			m_creature->SetAttackTime(BASE_ATTACK, m_creature->GetAttackTime(BASE_ATTACK)*.7f);
			m_bSlowed2 = true;
		}

		else if(m_uiFrostSpellCounter >= 10 && !m_bFrozen)					// does this emote multiple times?
		{
			m_bCanDoDamage = false;
			m_bFrozen = true;
			m_creature->CastSpell(m_creature, SPELL_VISCIDUS_FREEZE, true);
			m_creature->GenericTextEmote("Viscidus is frozen solid.", NULL, false);
			//m_creature->SetSpeedRate(MOVE_RUN, m_creature->GetSpeedRate(MOVE_RUN));
			m_creature->SetAttackTime(BASE_ATTACK, m_creature->GetAttackTime(BASE_ATTACK));					// FIX DIS: update to normal attack speed
			m_uiThawTimer = 15000;			
		}
	}

	void MeleeHitCount()
	{
		if(m_uiMeleeCounter >= 1 && !m_bCracking1)
		{
			m_creature->GenericTextEmote("Viscidus begins to crack.", NULL, false);
			m_bCracking1 = true;
		}

		else if(m_uiMeleeCounter >= 5 && !m_bCracking2)
		{
			m_creature->GenericTextEmote("Viscidus looks ready to shatter.", NULL, false);
			m_bCracking2 = true;
		}

		else if(m_uiMeleeCounter >= 10 && !m_bExploded)
		{
			if (HealthBelowPct(5))			// if Viscidus has less than 5% hp he should die since every glob is 5% hp
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NONE, NULL, false);

			m_bCanDoDamage = false;
			m_bExploded = true;
			RemoveAuras();					// remove auras if we're gonna explode
			m_creature->CastSpell(m_creature, SPELL_VISCIDUS_EXPLODE,true);
			m_creature->CastSpell(m_creature, SPELL_ROOT, true);
			m_creature->GenericTextEmote("Viscidus explodes.", NULL, false);
			m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);			

			m_uiSetInvisTimer = 4000;
		}
	}

	void SetVisible(int Visible = 0)
	{
		if (Visible == 0)
		{
			m_creature->RemoveAllAuras();
			m_creature->AttackStop();
			m_creature->setFaction(35);
			DoResetThreat();
			m_creature->SetVisibility(VISIBILITY_OFF);	
			if (m_creature->HasAura(SPELL_ROOT))
				m_creature->RemoveAurasDueToSpell(SPELL_ROOT);
		}
		else if (Visible == 1)
		{
			m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_creature->setFaction(m_creature->GetCreatureInfo()->faction_A);
            m_creature->SetVisibility(VISIBILITY_ON);
			ResetBool(1);			// reset all counters for melee here
			ResetBool(0);			// reset all counters for spells here
		}		
	}

	void SpawnGlobs()
	{
		//spawn all adds here
	}

	void JustSummoned(Creature* pSummoned)
    {
		pSummoned->GetMotionMaster()->MovePoint(1,-7992.36f,908.19,-52.62);		// a point in the middle of the room
		pSummoned->SetRespawnDelay(-10);			// make sure they won't respawn
    }

	void SummonedMovementInform(Creature* pSummoned, uint32 uiMotionType, uint32 uiPointId)				// something like this?
    {
		if(uiPointId == 1)
			if(pSummoned->IsWithinDistInMap(m_creature, 5.0f))
				pSummoned->CastSpell(m_creature, SPELL_REJOIN_VISCIDUS, true);
	}
	
    void SummonedCreatureJustDied(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_GLOB_OF_VISCIDUS)
            m_creature->SetHealth(m_creature->GetHealth()-m_creature->GetMaxHealth()*0.05f);			// remove 5% hp for each glob that dies
    }

    void UpdateAI(const uint32 uiDiff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;
		
		if (m_bFrozen && !m_bExploded)
		{
			if (m_uiThawTimer <= uiDiff)			// remove all slows and reset counters
			{
				RemoveAuras();
				ResetBool(0);	
			}
			else
				m_uiThawTimer -= uiDiff;
		}

		if(m_bExploded)
		{
			if (m_uiSetInvisTimer <= uiDiff)
			{
				m_uiGlobSpawnTimer = 4000;			// slight delay before we spawn the adds
				SetVisible(0);
			}
			else
				m_uiSetInvisTimer -= uiDiff;

			if (m_uiGlobSpawnTimer <= uiDiff)
			{
				SpawnGlobs();
			}
			else
				m_uiGlobSpawnTimer -= uiDiff;
		}

		if (m_bCanDoDamage)
		{
			if (m_uiPoisonShockTimer <= uiDiff)
			{
				m_creature->CastSpell(m_creature, SPELL_POISON_SHOCK, false);
				m_uiPoisonShockTimer = 10000;
			}
			else
				m_uiPoisonShockTimer -= uiDiff;

			if (m_uiPoisonVolleyTimer <= uiDiff)
			{
				m_creature->CastSpell(m_creature, SPELL_POISONBOLT_VOLLEY, false);
				m_uiPoisonVolleyTimer = 10000;
			}
			else
				m_uiPoisonVolleyTimer -= uiDiff;

			if (m_uiToxicCloudTimer <= uiDiff)				// redo this, spawn the npcs instead, or don't summon the cloud under the boss
			{
				m_creature->CastSpell(m_creature, SPELL_TOXIN_CLOUD, false);   
				m_uiToxicCloudTimer = urand(10000,15000);
			}
			else
				m_uiToxicCloudTimer -= uiDiff;

			DoMeleeAttackIfReady();
		}
    }
};

CreatureAI* GetAI_boss_viscidus(Creature* pCreature)
{
    return new boss_viscidusAI(pCreature);
}

struct MANGOS_DLL_DECL boss_glob_of_viscidusAI : public ScriptedAI
{
    boss_glob_of_viscidusAI(Creature* pCreature) : ScriptedAI(pCreature) 
	{
		m_creature->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_NATURE, true);
		Reset();
	}

	void Reset()
	{
	}

	void MoveInLineOfSight(Unit* /*pWho*/)
    {
        // Must to be empty to ignore aggro
    }

	void UpdateAI(const uint32 uiDiff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;
		//m_creature->SetTargetGuid(ObjectGuid());				// target self even when someone does dmg, - needs testing if this is needed
	}
};

CreatureAI* GetAI_boss_glob_of_viscidus(Creature* pCreature)
{
    return new boss_glob_of_viscidusAI(pCreature);
}

void AddSC_boss_viscidus()
{
	Script* pNewscript;

    pNewscript = new Script;
    pNewscript->Name = "boss_viscidus";
    pNewscript->GetAI = &GetAI_boss_viscidus;
    pNewscript->RegisterSelf();

	pNewscript = new Script;
    pNewscript->Name = "boss_glob_of_viscidus";
    pNewscript->GetAI = &GetAI_boss_glob_of_viscidus;
    pNewscript->RegisterSelf();
}

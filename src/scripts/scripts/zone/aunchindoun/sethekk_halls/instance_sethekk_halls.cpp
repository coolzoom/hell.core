/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ScriptData
SDName: Instance - Sethekk Halls
SD%Complete: 50
SDComment: Instance Data for Sethekk Halls instance
SDCategory: Auchindoun, Sethekk Halls
EndScriptData */

#include "precompiled.h"
#include "def_sethekk_halls.h"
#include "escort_ai.h"

#define ENCOUNTERS          3

#define IKISS_DOOR          177203
#define NPC_LAKKA           18956
#define QUEST_BROTHER       10097

struct instance_sethekk_halls : public ScriptedInstance
{
    instance_sethekk_halls(Map *map) : ScriptedInstance(map) {Initialize();};

    uint32 Encounter[ENCOUNTERS];

    bool SummonLakka;

    uint64 IkissDoorGUID;

    void Initialize()
    {
        IkissDoorGUID = 0;
        SummonLakka = false;

        for(uint8 i = 0; i < ENCOUNTERS; i++)
            Encounter[i] = NOT_STARTED;
    }

    bool IsEncounterInProgress() const
    {
        for(uint8 i = 0; i < ENCOUNTERS; i++)
            if(Encounter[i] == IN_PROGRESS) return true;

        return false;
    }

    void OnPlayerEnter(Player* player)
    {
        if (player->GetQuestStatus(QUEST_BROTHER) == QUEST_STATUS_INCOMPLETE && !SummonLakka)
        {
            player->SummonCreature(NPC_LAKKA, -158.226f, 158.690f, 0.0f, 1.21f, TEMPSUMMON_DEAD_DESPAWN, 10000);
            SummonLakka = true;
        }
    }

    void OnCreatureCreate(Creature *creature, uint32 entry)
    {
        switch(entry)
        {
            case 23035:
                if(GetData(DATA_ANZUEVENT) == DONE)
                {
                    creature->Kill(creature);
                    creature->RemoveCorpse();
                }
                break;
        }
    }

    void OnObjectCreate(GameObject *go)
    {
        switch(go->GetEntry())
        {
            case IKISS_DOOR:
                IkissDoorGUID = go->GetGUID();
                if(GetData(DATA_IKISSEVENT) == DONE)
                    HandleGameObject(IkissDoorGUID, true);
                break;
        }
    }

    void SetData(uint32 type, uint32 data)
    {
        switch(type)
        {
            case DATA_IKISSEVENT:
                if(Encounter[0] != DONE)
                    Encounter[0] = data;
                if(data == DONE)
                    HandleGameObject(IkissDoorGUID, true);
                break;
            case DATA_ANZUEVENT:
                if(Encounter[1] != DONE)
                    Encounter[1] = data;
                break;
            case DATA_DARKWEAVEREVENT:
                if(Encounter[2] != DONE)
                    Encounter[2] = data;
                break;
        }

        if (data == DONE)
            SaveToDB();
    }

    uint32 GetData(uint32 type)
    {
        switch( type )
        {
            case DATA_IKISSEVENT:           return Encounter[0];
            case DATA_ANZUEVENT:            return Encounter[1];
            case DATA_DARKWEAVEREVENT:      return Encounter[2];
                        
        }
        return false;
    }

   std::string GetSaveData()
    {
        OUT_SAVE_INST_DATA;

        std::ostringstream stream;
        stream << Encounter[0] << " "
                << Encounter[1] << " "
                << Encounter[2];

        OUT_SAVE_INST_DATA_COMPLETE;

        return stream.str();
    }

    void Load(const char* in)
    {
        if (!in)
        {
            OUT_LOAD_INST_DATA_FAIL;
            return;
        }

        OUT_LOAD_INST_DATA(in);

        std::istringstream loadStream(in);
        loadStream >> Encounter[0] >> Encounter[1] >> Encounter[2];

        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            if (Encounter[i] == IN_PROGRESS)
                Encounter[i] = NOT_STARTED;

        OUT_LOAD_INST_DATA_COMPLETE;
    }

};

InstanceData* GetInstanceData_instance_sethekk_halls(Map* map)
{
    return new instance_sethekk_halls(map);
}

/*#####
## npc_lakka
#####*/

struct npc_lakkaAI : public npc_escortAI
{
    npc_lakkaAI(Creature *creature) : npc_escortAI(creature) {}

    void Reset() {}

    void WaypointReached(uint32 i) {}
};

CreatureAI* GetAI_npc_lakka(Creature* pCreature)
{
    return new npc_lakkaAI(pCreature);
}

#define GOSSIP_FREE "I'll have you out of here in just a moment."
#define SAY_FREE            -1900254
#define GO_SETHEKK_CAGE     183051

bool GossipHello_npc_lakka(Player *player, Creature *creature)
{
    if(player->GetQuestStatus(QUEST_BROTHER) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_FREE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(9636, creature->GetGUID());
    return true;
}

bool GossipSelect_npc_lakka(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        player->CLOSE_GOSSIP_MENU();
        if (GameObject* cage = FindGameObject(GO_SETHEKK_CAGE, INTERACTION_DISTANCE, creature))
            cage->UseDoorOrButton(5);
        player->KilledMonster(NPC_LAKKA, creature->GetGUID());
        DoScriptText(SAY_FREE, creature, player);
        ((npc_lakkaAI*)creature->AI())->Start(false, false, player->GetGUID());
    }

    return true;
}

void AddSC_instance_sethekk_halls()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_sethekk_halls";
    newscript->GetInstanceData = &GetInstanceData_instance_sethekk_halls;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_lakka";
    newscript->GetAI = &GetAI_npc_lakka;
    newscript->pGossipHello =  &GossipHello_npc_lakka;
    newscript->pGossipSelect = &GossipSelect_npc_lakka;
    newscript->RegisterSelf();
}


#include "../../../ps2/veronica/prog/eneset.h"
#include "../../../ps2/veronica/prog/en01.h"
#include "../../../ps2/veronica/prog/en02.h"
#include "../../../ps2/veronica/prog/en03.h"
#include "../../../ps2/veronica/prog/en04.h"
#include "../../../ps2/veronica/prog/en05.h"
#include "../../../ps2/veronica/prog/en06.h"
#include "../../../ps2/veronica/prog/en07.h"
#include "../../../ps2/veronica/prog/en08.h"
#include "../../../ps2/veronica/prog/en09.h"
#include "../../../ps2/veronica/prog/en10.h"
#include "../../../ps2/veronica/prog/en11.h"
#include "../../../ps2/veronica/prog/en12.h"
#include "../../../ps2/veronica/prog/en13.h"
#include "../../../ps2/veronica/prog/en14.h"
#include "../../../ps2/veronica/prog/en15.h"
#include "../../../ps2/veronica/prog/en16.h"
#include "../../../ps2/veronica/prog/en17.h"
#include "../../../ps2/veronica/prog/en18.h"
#include "../../../ps2/veronica/prog/en19.h"
#include "../../../ps2/veronica/prog/en20.h"
#include "../../../ps2/veronica/prog/en21.h"
#include "../../../ps2/veronica/prog/en22.h"
#include "../../../ps2/veronica/prog/en23.h"
#include "../../../ps2/veronica/prog/en24.h"
#include "../../../ps2/veronica/prog/en25.h"
#include "../../../ps2/veronica/prog/en26.h"
#include "../../../ps2/veronica/prog/en27.h"
#include "../../../ps2/veronica/prog/en29.h"
#include "../../../ps2/veronica/prog/en30.h"
#include "../../../ps2/veronica/prog/en54.h"
#include "../../../ps2/veronica/prog/en55.h"
#include "../../../ps2/veronica/prog/en71.h"
#include "../../../ps2/veronica/prog/MdlPut.h"
#include "../../../ps2/veronica/prog/Motion.h"
#include "../../../ps2/veronica/prog/effect.h"
#include "../../../ps2/veronica/prog/face.h"
#include "../../../ps2/veronica/prog/flag.h"
#include "../../../ps2/veronica/prog/hitchk.h"
#include "../../../ps2/veronica/prog/light.h"
#include "../../../ps2/veronica/prog/njplus.h"
#include "../../../ps2/veronica/prog/pwksub.h"
#include "../../../ps2/veronica/prog/sdfunc.h"
#include "../../../ps2/veronica/prog/subpl.h"
#include "../../../ps2/veronica/prog/weapon.h"
#include "../../../ps2/veronica/prog/zonzon1.h"
#include "../../../ps2/veronica/prog/main.h"

typedef void (*JumpEnemy_proc)(BH_PWORK*);

JumpEnemy_proc bhJumpEnemy[100] = 
{
	bhEne00,
	bhEne01,
	bhEne02,
	bhEne03,
	bhEne04,
	bhEne05,
	bhEne06,
	bhEne07,
	bhEne08,
	bhEne09,
	bhEne10,
	bhEne11,
	bhEne12,
	bhEne13,
	bhEne14,
	bhEne15,
	bhEne16,
	bhEne17,
	bhEne18,
	bhEne19,
	bhEne20,
	bhEne21,
	bhEne22,
	bhEne23,
	bhEne24,
	bhEne25,
	bhEne26,
	bhEne27,
	bhEne28,
	bhEne29,
	bhEne30,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhEne53,
	bhEne54,
	bhEne55,
	bhEne56,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhEne71,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl,
	bhSubpl
};
#ifdef TARGET_PC
/* Helper code (docs/ENEMIES.md "Budgets on the PC"): the parts and children an enemy spawns get ids 31..38 and
   run whatever bhEne_SetCallFunc put in bhJumpEnemy[31..38] last - on the PS2 one function per
   id, so two types using them in one room ran each other's code. Here each child runs the
   function its own creator's type registered: the creator is the enemy whose code runs when
   bhSetEnemy takes the slot. Only in rooms the PC added enemies to (pc_enemy_added: the F2
   spawner, the enemy options), so retail rooms run as on the PS2 (RM_6030 mixes a zombie with
   moths); a room with one such type calls the same function either way. */
int pc_enemy_added;
static JumpEnemy_proc pc_helper[100][8];   /* [creator type][id - 31] */
static unsigned char pc_owner[128];        /* slot -> creator type + 1; 0: made outside an enemy's code (room load, spawner) */
static BH_PWORK* pc_running;               /* the enemy whose code runs (bhControlEnemy) */

/* One enemy at a time takes a player. An enemy's code that sets the player's busy flag (flg & 4:
   grabbed, thrown, hit) owns that player until the flag clears; the others check it before they
   attack - within a family. Types the PS2 never put in one room do not always: a Black Widow's
   grab wrote its throw level into a Hunter's hit, Monster Steve's throw read a zombie's grab
   motions (crashes). So in rooms with added enemies a held player is put back after another
   enemy's code changed it (its parts and children count as the holder). Per player: co-op runs
   an enemy against the partner's work (coop_glue.c swaps plp). */
static struct { BH_PWORK* pl; BH_PWORK* by; } pc_held[2];

static BH_PWORK* pc_root(BH_PWORK* ep)   /* a part's or child's parent enemy */
{
    int n;
    for (n = 0; n < 3 && ep->lkwkp && (BH_PWORK*)ep->lkwkp >= ene && (BH_PWORK*)ep->lkwkp < ene + 128; n++) ep = (BH_PWORK*)ep->lkwkp;
    return ep;
}

void pc_enemy_watch(BH_PWORK* ep, int slot, int ran);   /* coop_glue.c */
void pc_enemy_unhold(BH_PWORK* ep);
void host_log(const char* fmt, ...);

/* co-op (coop_glue.c grab_in): the host's zombie hands its grab to our game, which puts our player in
   the grab outside any enemy's code - that zombie is the holder from now on */
void pc_enemy_holds(BH_PWORK* pl, BH_PWORK* by)
{
    int h;
    for (h = 0; h < 2 && pc_held[h].pl != pl; h++);
    if (h == 2) h = pc_held[0].by ? 1 : 0;
    pc_held[h].pl = pl;
    pc_held[h].by = by;
}

/* the enemy type whose sounds a request of enemy `slot` plays (sdfunc.c via spawn_glue.c): the enemy
   whose code runs, else `slot`; 127 outside enemy code is an effect's request (zonzon1.c
   bhEne_CallEffectSE from the effect's step): the type that made the effect (effect.c). A part or
   child counts as its creator's type. -1: none */
int pc_enemy_sound_type(int slot)
{
    extern short pc_eff_owner[512][2];
    BH_PWORK* ep = pc_running ? pc_running : slot >= 0 && slot < 127 ? &ene[slot] : NULL;
    int o;
    if (!pc_running && slot == 127)
        return sys->efnow < 512 && pc_eff_owner[sys->efnow][1] == eff[sys->efnow].id + 1 ? pc_eff_owner[sys->efnow][0] : -1;
    if (!ep || !(ep->flg & 0x1)) return -1;
    ep = pc_root(ep);
    o = pc_owner[ep - ene];
    return o ? o - 1 : ep->id;
}

static void pc_call_enemy(BH_PWORK* ep, int slot)
{
    JumpEnemy_proc f = bhJumpEnemy[ep->id];
    int o = slot < 128 ? pc_owner[slot] : 0, h, busy;
    BH_PWORK* pl = plp;
    if (pc_enemy_added && ep->id >= 31 && ep->id <= 38 && o && pc_helper[o - 1][ep->id - 31]) f = pc_helper[o - 1][ep->id - 31];
    pc_running = ep;
    if (!pc_enemy_added || !pl) {
        f(ep);
        pc_running = NULL;
        return;
    }
    for (h = 0; h < 2 && pc_held[h].pl != pl; h++);
    if (h == 2) {
        h = pc_held[0].by ? 1 : 0;
        pc_held[h].pl = pl;
        pc_held[h].by = NULL;
    }
    if (pc_held[h].by && (!(pl->flg & 0x4) || !(pc_held[h].by->flg & 0x1))) pc_held[h].by = NULL;
    if (pc_held[h].by && pc_root(ep) != pc_root(pc_held[h].by)) {
        BH_PWORK keep = *pl;
        f(ep);
        busy = pl->mode0 != keep.mode0 && (pl->mode0 == 4 || pl->mode0 == 6);   /* it started a grab */
        *pl = keep;
        /* refused: a zombie would play its grab on nobody (and its bites would drive the other holder's
           grab); it lets go as at the end of a grab instead (coop_glue.c) */
        if (busy && ep->id == 1) {
            pc_enemy_unhold(ep);
            host_log("enemies: enemy %d's grab refused, another enemy holds the player: released\n", slot);
        }
    } else {
        busy = pl->flg & 0x4;
        f(ep);
        if (!busy && (pl->flg & 0x4)) pc_held[h].by = ep;
    }
    pc_running = NULL;
}
#endif

CPCL En00CapColTab[5] =
{
    { 0, 0,   35 },
    { 0, 50,  0  },
    { 0, 0,   35 },
    { 0, 100, 0  },
    { 0, 0,   0  },
};

// 100% matching! 
void bhInitEnemy()
{
    npSetMemory((unsigned char*)ene, 180224, 0);
}

// 100% matching!
BH_PWORK* bhSetEnemy(EGG_WORK* etp, int unused) // second parameter is not present on the debugging symbols
{
    BH_PWORK* epp;
    int i;
    
    epp = ene;
    
    for (i = 0; ; )
    {
        if (!(epp->flg & 0x1)) 
        {
            npSetMemory((unsigned char*)epp, sizeof(BH_PWORK), 0);
            
            epp->flg = etp->flg;
            
            epp->id = etp->id;
            
            if (epp->id > 40) 
            {
                epp->mdflg |= 0x20;
                
                switch (epp->id) 
                {                      
                case 41:
                case 42:
                case 43:
                case 44:
                case 45:
                case 46:
                case 47:
                case 48:
                    epp->clp_jno[0] = 0;
                    epp->clp_jno[1] = 3;
                    epp->clp_jno[2] = 5;
                    epp->clp_jno[3] = 8;
                    epp->clp_jno[4] = 12;
                    epp->clp_jno[5] = 15;
                    epp->clp_jno[6] = 19;
                    epp->clp_jno[7] = -1;
                    break;
                case 91:
                case 92:
                case 93:
                case 94:
                case 95:
                case 96:
                case 97:
                case 98:
                    epp->clp_jno[0] = 0;
                    epp->clp_jno[1] = 3;
                    epp->clp_jno[2] = 5;
                    epp->clp_jno[3] = 17;
                    epp->clp_jno[4] = 21;
                    epp->clp_jno[5] = 24;
                    epp->clp_jno[6] = 28;
                    epp->clp_jno[7] = -1;
                    break;
                }
                
                epp->flg |= 0x8000;
            }
            
            if (epp->id > 90) 
            {
                epp->mdflg |= 0x100;
            }
            
            epp->type = etp->type;
            
            epp->flr_no = etp->flr_no;
            
            epp->mdlver = etp->mdlver;
            
            epp->px = etp->px;
            epp->py = etp->py;
            epp->pz = etp->pz;
            
            epp->ax = 0;
            epp->ay = etp->ay;
            epp->az = 0;
            
            epp->aoz = 0;
            epp->aoy = 0;
            epp->aox = 0;
            
            *(int*)&epp->mode0 = 0;
            
            epp->ct3 = 0;
            epp->ct2 = 0;
            epp->ct1 = 0;
            epp->ct0 = 0;
            
            epp->lkwkp = NULL;
            
            epp->exp0 = NULL;
            epp->exp1 = NULL;
            epp->exp2 = NULL;
            
            epp->idx_ct = i;
#ifdef TARGET_PC
            if (i < 128) pc_owner[i] = pc_running && pc_running->id < 100 ? pc_running->id + 1 : 0;
#endif
            
            epp->sxb = 1.0f;
            epp->sx = 1.0f;
            
            epp->syb = 1.0f;
            epp->sy = 1.0f;
            
            epp->szb = 1.0f;
            epp->sz = 1.0f;
            
            epp->obj_a = NULL;
            epp->obj_b = NULL;
            
            epp->shp_ct = 0;
            
            epp->mtx = (float(*)[16])epp->mtxbuf;
            
            epp->frm_no = 0;
            epp->mtn_no = 0;
            epp->mdl_no = 0;
            
            epp->hokan_count = 0;
            epp->hokan_rate = 0;
            
            epp->frm_mode = 0;
            
            epp->mtn_no = 0;
            epp->mtn_add = 65536;
            epp->mtn_md = 0;
            epp->mtn_tp = NULL;
            
            epp->at_flg = 0;
            
            epp->clp_jno[0] = 1;
            epp->clp_jno[1] = -1;
            
            bhSetFloorNum(epp);
            
            bhCheckEneWorkNum();
            
            return epp;
        }
        
        i++;
        epp++;
        
        if (i >= 128) 
        {
            bhCheckEneWorkNum();
            
            return NULL;
        }
    }
}

// 100% matching!
void bhCheckEneWorkNum()
{
    BH_PWORK* epp;
    int i;

    sys->ewk_n = 0;
    
    epp = ene;

    for (i = 0; i < 128; i++, epp++)
    {
        if ((epp->flg & 0x1))
        {
            sys->ewk_n = i + 1;
        }
    }
}

// 100% matching!
void bhControlEnemy()
{
    BH_PWORK* ep;
    unsigned int i;
    
    plp->ofz = 0;
    plp->ofx = 0;
    
    if ((sys->sp_flg & 0x2)) 
    {
        ep = ene;
        
        sys->en_objn = 0;
        
        for (i = 0; i < sys->ewk_n; i++, ep++)
        {
            sys->enow = i;
            
            if ((!(ep->stflg & 0x41000000)) && (((!(ep->flg & 0x80)) || (!(((BH_PWORK*)ep->lkwkp)->stflg & 0x1000000))) && ((ep->flg & 0x1))))
            {
                ep->pxb = ep->px;
                ep->pyb = ep->py;
                ep->pzb = ep->pz;
                
                ep->axb = ep->ax;
                ep->ayb = ep->ay;
                ep->azb = ep->az;
                
                if (ep->id > 90) 
                {
                    bhControlMask(ep);
                }
                
#ifdef TARGET_PC
                /* co-op: the host retargets the enemy, the client animates the host's state */
                {
                    int ran = !coop_on || !coop_enemy_begin(ep);

                    if (ran)
                    {
                        pc_call_enemy(ep, i);
                    }

                    if (coop_on)
                    {
                        coop_enemy_end(ep);
                    }

                    pc_enemy_watch(ep, i, ran);   /* coop_glue.c: the frozen-enemy watchdog */
                }
#else
                bhJumpEnemy[ep->id](ep);
#endif

                bhEne_InitDamage(ep);
                
                if ((ep->flg & 0x80000000)) 
                {
                    sys->en_obj[sys->en_objn++] = ep;
                }
            }
        }
    }
}

// 98.23% matching (matches on GC)
void bhEne00(BH_PWORK* ep)
{
    ATR_WORK* hp; 
    NJS_POINT3 pos; 
    int i;          
    POINT pnt;      

    switch (ep->mode0)
    {       
    case 0:
        ep->flg |= 0x8120;
        ep->flg &= ~0x6;
        
        ep->flg2 |= 0x1;
        
        ep->mdflg &= ~0x1;
        ep->mdflg |= 0x20;
        
        ep->ar = 4.0f;
        ep->ah = 15.0f;
        
        ep->aox = 0;
        ep->aoy = 0;
        ep->aoz = 0;
        
        ep->car = 4.0f;
        ep->cah = 15.0f;
        
        ep->hp = 0;
        
        ep->cpcl = En00CapColTab;
        
        if (!(ep->flg2 & 0x80000000)) 
        {
            ep->flg2 |= 0x80000000;
            
            ep->ct0 = sys->mwal_n++;
            
            hp = &sys->mwalp[ep->ct0];
            
            hp->flg = 0x81;
            
            hp->type = 3;
            
            hp->flr_no = ep->flr_no;
            
            hp->attr = 8;
            
            hp->px = ep->px;
            hp->py = rom->grand[hp->flr_no + 2];
            hp->pz = ep->pz;
            
            hp->w = 3.5f;
            hp->h = 15.0f;
            
            hp->d = 0;
            
            hp->prm2 = 0;
            hp->prm1 = 0;
            hp->prm0 = 0;
            hp->prm3 = sys->enow;
            
            pnt.px = 0;
            pnt.py = 0;
            pnt.pz = 0;
            
            pnt.ox = 0;
            pnt.oy = 0;
            pnt.oz = 0;
            
            bhSetEffect(17, &pnt, (unsigned char*)ep, 0);
        } 
        else 
        {
            hp = &sys->mwalp[ep->ct0];
            
            hp->flg = 0x81;
            
            hp->attr = 8;
        }
        
        ep->mode0 = 1;
        break;
    case 1:
        hp = &sys->mwalp[ep->ct0];
        
        if (((ep->flg & 0x4)) && (ep->wpnr_no != 2))
        {
            pos.x = ep->px;
            pos.y = 10.0f + ep->py;
            pos.z = ep->pz;
            
            bhSetExplosion(&pos);
            
            ep->flg |= 0x2;
            
            ep->mdflg |= 0x1;
            
            ep->flg &= ~0x78;
            
            hp->flg &= ~0x1;
            
            ep->mode0 = 2;
        }
        
        hp->px = ep->px;
        hp->py = ep->py;
        hp->pz = ep->pz;
        break;
    case 2:
        break;
    }
   
    if ((ep->flg & 0x4)) 
    {
        for (i = 0; i < ep->mlwP->obj_num; i++) 
        {
            ep->dam[i] = 0;
        }
        
        ep->flg &= ~0x4;
    }
    
    if ((ep->flg & 0x40)) 
    {
        bhCheckPlayer(ep);
    }
    
    if ((ep->flg & 0x8)) 
    {
        bhCheckEnemies(ep);
    }
    
    if ((ep->flg & 0x10)) 
    {
        bhCheckWall(ep);
    }
    
    bhCalcModel(ep);
    
    ep->watr.c1.x = ep->mlwP->owP->mtx[12];
    ep->watr.c1.y = ep->mlwP->owP->mtx[13];
    ep->watr.c1.z = ep->mlwP->owP->mtx[14];
    
    ep->watr.c2.x = ep->watr.c1.x;
    ep->watr.c2.y = 15.0f + ep->watr.c1.y;
    ep->watr.c2.z = ep->watr.c1.z;
    
    ep->watr.r = ep->car;
}

// 99.96% matching
void bhEne56(BH_PWORK* ep)
{
	O_WRK* op;    
    NJS_POINT3 pos; 

    ep->flg |= 0x8000;
    
    switch (ep->mode0)
    {
    case 0:
        ep->mdflg |= 0x20;
        
        ep->mnwP = sys->rmthp;
        
        ep->mtn_no = 0;
        ep->mtn_md = 0; 
        ep->mtn_tp = 0;
        
        ep->hokan_rate = 32768;
        ep->hokan_count = 0;
        
        ep->mtn_add = 0;
        
        ep->frm_no = 0;
        
        sys->ufo_oidx = 4;
        
        if (bhCkFlg(sys->ev_flg, 0x3B) != 0) 
        {
            ep->px = sys->ufo_pos.x = 13.0f;
            ep->py = sys->ufo_pos.y = -15.0f;
            ep->pz = sys->ufo_pos.z = 11.6f;
        }
        else 
        {
            ep->px = sys->ufo_pos.x = 47.0f;
            ep->py = sys->ufo_pos.y = -15.0f;
            ep->pz = sys->ufo_pos.z = 60.0f;
        }
        
        if (!(ep->flg & 0x800)) 
        {
            ep->ct3 = bhSetShadow(NULL, (unsigned char*)ep, 1, 12.0f, 10.0f, 12.0f);
            
            ep->flg |= 0x800;
        }
        
        if (bhCkFlg(sys->ev_flg, 0x3B) != 0) 
        {
            op = &sys->obwp[sys->ufo_oidx];
            
            op->flg |= 0x80;
            
            op->lkono = 0;
            
            op->lox = 0;
            op->loy = 42.0f;
            op->loz = 0;
            
            op->lkwkp = (unsigned char*)ep;
            
            ep->mode0 = 2;
            break;
        }
        
        ep->mode0 = 1;
    case 1:
        ep->px = sys->ufo_pos.x;
        ep->py = sys->ufo_pos.y;
        ep->pz = sys->ufo_pos.z;
        
        bhSetMotion(ep, (int)ep->mtn_add, ep->mtn_md, NULL);
        
        bhCalcModel(ep);
        
        switch (sys->ufo_md) 
        {
        case 0:
            if (bhCkFlg(sys->ev_flg, 0x3B) != 0)
            {
                ep->mode0 = 2;
            }
            else
            {
                ep->mtn_add = 0;
                
                ep->frm_no = 0;
            }
            
            break;
        case 1:
            if ((ep->frm_no / 65536) < 40)
            {
                ep->mtn_add = 65536;
            }
            else
            {
                ep->mtn_add = 0;
            }
            
            break;
        case 2:
            pos = *(NJS_POINT3*)&ep->mlwP->owP[3].mtx[12];
            
            pos.x -= 0.6f;
            pos.y -= 10.0f;
            
            if (bhCheckWallType(&pos, 0, 2.0f, 3.0f) != NULL) 
            {
                sys->ufo_md = 3;
                break;
            }
            
            pos = *(NJS_POINT3*)&ep->mlwP->owP[5].mtx[12];
            
            pos.x -= 0.6f;
            pos.y -= 10.0f;
            
            if (bhCheckWallType(&pos, 0, 2.0f, 3.0f) != NULL) 
            {
                sys->ufo_md = 3;
                break;
            }
            
            pos = *(NJS_POINT3*)&ep->mlwP->owP[7].mtx[12];
            
            pos.x += 0.6f;
            pos.y -= 10.0f;
            
            if (bhCheckWallType(&pos, 0, 2.0f, 3.0f) != NULL) 
            {
                sys->ufo_md = 3;
                break;
            }
            
            pos = *(NJS_POINT3*)&ep->mlwP->owP[9].mtx[12];
            
            pos.x += 0.6f;
            pos.y -= 10.0f;
            
            if (bhCheckWallType(&pos, 0, 2.0f, 3.0f) != NULL) 
            {
                sys->ufo_md = 3;
            }
            
            break;
        case 4:
            if ((ep->frm_no / 65536) < 70)
            {
                ep->mtn_add = 65536;
            }
            else
            {
                ep->mtn_add = 0;
            }
            
            if ((sys->ufo_flg != 0) && ((sys->obwp[sys->ufo_oidx].px - ep->mlwP->owP[7].mtx[12]) < 9.0f)) 
            {
                sys->obwp[sys->ufo_oidx].px = ep->mlwP->owP[7].mtx[12] + 9.0f;
            }
            
            break;
        case 5:
            op = &sys->obwp[sys->ufo_oidx];
            
            if (sys->ufo_flg != 0)
            {
                if (!(op->flg & 0x80)) 
                {
                    op->flg |= 0x80;
                    
                    op->lkono = 0;
                    
                    op->lox = 0;
                    op->loy = op->py - ep->py;
                    op->loz = op->pz - ep->pz;
                    
                    op->lkwkp = (unsigned char*)ep;
                }
                
                if ((sys->ufo_flg == 1) && (ep->py >= -35.0f)) 
                {
                    op->flg &= ~0x80;
                    
                    op->px = op->mlwP->owP->mtx[12];
                    op->py = op->mlwP->owP->mtx[13];
                    op->pz = op->mlwP->owP->mtx[14];
                    
                    op->mode1 = 0;
                    
                    op->yn = 0;
                    
                    sys->ufo_md = 6;
                }
            }
            
            break;
        case 6:
            op = &sys->obwp[sys->ufo_oidx];
            
            switch (op->mode1) 
            {
            case 0:
                op->py = op->py + op->yn;
                
                if (op->yn > -1.0f) 
                {
                    op->yn -= 0.2f;
                }
                
                if (op->py <= 0) 
                {
                    op->yn = 1.0f;
                    
                    op->mode1 = 1;
                    
                    CallSystemSe(0, 0x80000211);
                }
                
                break;
            case 1:
                op->py = op->yn * (-rand() / -2.1474836e+09f);
                
                op->yn *= 0.8f;
                
                if (op->yn < 0.1f)
                {
                    op->mode1 = 2;
                    
                    op->py = 0;
                    
                    op->yn = 0;
                }
                
                break;
            case 2:
                break;
            }
            
            break;
        }
        
        break;
    case 2:
        ep->frm_no = 0x460000;
        
        bhSetMotion(ep, 0, ep->mtn_md, NULL);
        
        bhCalcModel(ep);
        break;
    }
}

// 100% matching!
void bhDrawEnemy() 
{
    BH_PWORK* ep;
    int i;
    unsigned int argb;
    
    ep = ene;
    
    for (i = 0; i < sys->ewk_n; i++, ep++)
    {
        if ((((ep->flg & 0x1)) && (!(ep->flg & 0x80000000))) && (!(ep->stflg & 0x41000000)) && (((sys->st_flg & 0x20)) || (!(ep->mdflg & 0x200))) && ((!(ep->flg & 0x80)) || (!(((BH_PWORK*)ep->lkwkp)->stflg & 0x1000000))))
        {
            if ((ep->mdflg & 0x4)) 
            {
                bhCalcModel(ep);
            }
            
            if ((((ep->mdflg & 0x20)) || (bhCheckClipModel(ep) == 0)) && (!(ep->mdflg & 0x1)) && (ep->mlwP->objP != NULL)) 
            {
                if ((sys->st_flg & 0x20))
                {
#ifdef TARGET_PC
                    if (!actor_on || !actor_put_enemy(ep))   /* the character played's cutscene double as another model */
#endif
                    bhPutModel(ep);
                }
                else
                {
                    if ((sys->gm_flg & 0x1000040)) 
                    {
                        if ((ep->mdflg & 0x400))
                        {
                            argb = npGetMatColor(ep->mlwP->objP, ep->mlwP->obj_num) & 0xFF;
                            
                            if (argb >= 0)
                            {
                                bhSetEasyDirLight(argb / 255.0f);
                            }
                            else
                            {
                                bhSetEasyDirLight((2.0f * ((argb >> 1) | (argb & 0x1))) / 255.0f);
                            }
                        }
                        else 
                        {
                            bhSetEasyDirLight(0.6f);
                        }
                    }
                    
                    if ((ep->mdflg & 0x2))
                    {
                        npPushMdlstr2(ep->obj_a, ep->mlwP->obj_num);
                        
                        npCalcMorphing(ep->obj_a, ep->obj_b, ep->shp_ct, ep->mlwP->obj_num);
                        
                        ep->mlwP->objP = ep->obj_a;
                        
                        bhPutModel(ep);
                        
                        npPopMdlstr2(ep->obj_a, ep->mlwP->obj_num);
                    } 
                    else 
                    {
#ifdef TARGET_PC
                        if (!actor_on || !actor_put_enemy(ep))
#endif
                        bhPutModel(ep);
                    }
                }
            }
        }
    }
}

// 100% matching!
void bhDrawEneObject()
{
    BH_PWORK* ep;
    int i;
	
    for (i = 0; i < sys->en_objn; i++) 
    {
        ep = sys->en_obj[i];
        
        if (((ep->flg & 0x1)) && (!(ep->stflg & 0x41000000)))
        {
            if ((ep->mdflg & 0x4))
            {
                bhCalcModel(ep);
            }
            
            if ((((ep->mdflg & 0x20)) || (bhCheckClipModel(ep) == 0)) && (ep->mlwP->objP != NULL) && (!(ep->mdflg & 0x1)))
            {
                if ((ep->mdflg & 0x2)) 
                {
                    npPushMdlstr2(ep->obj_a, ep->mlwP->obj_num);
                    
                    npCalcMorphing(ep->obj_a, ep->obj_b, ep->shp_ct, ep->mlwP->obj_num);
                    
                    ep->mlwP->objP = ep->obj_a;
                    
                    bhPutModel(ep);
                    
                    npPopMdlstr2(ep->obj_a, ep->mlwP->obj_num);
                }
                else 
                {
                    bhPutModel(ep);
                }
            }
        }
    }
}

// 100% matching!
void* bhEne_CallocWork(int size, int param) // second parameter not present on DWARF
{
    unsigned char* ret_addr; 
    unsigned char* addr;     

#ifdef TARGET_PC
    if ((&sys->memp[size] - sys->mempb) > 0xC40000 + PC_ARENA_EXTRA)   /* the arena's growth, as bhGetFreeMemory */
#else
    if ((&sys->memp[size] - sys->mempb) > 0xC40000)
#endif
    {
        return NULL;
    }
    
    sys->memp = (unsigned char*)(((unsigned int)sys->memp + 63) & ~0x3F);
    
    ret_addr = sys->memp;

    addr = ret_addr; 
    
    sys->memp = &ret_addr[size];
    
    while (size-- != 0) 
    {
        *addr++ = 0;
    } 
    
    return ret_addr;
}

// 100% matching!
void bhEne_SetCallFunc(void(*func)(BH_PWORK*), unsigned int no)
{
    bhJumpEnemy[no] = func;
#ifdef TARGET_PC
    if (pc_running && pc_running->id < 100 && no >= 31 && no <= 38) pc_helper[pc_running->id][no - 31] = func;
#endif
}

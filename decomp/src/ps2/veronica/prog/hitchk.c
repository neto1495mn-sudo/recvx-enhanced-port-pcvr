#include "../../../ps2/veronica/prog/hitchk.h"
#include "../../../ps2/veronica/prog/effsub3.h"
#include "../../../ps2/veronica/prog/main.h"
#include "../../../ps2/veronica/prog/player.h"
#include "../../../ps2/veronica/prog/ps2_NaColi.h"
#include "../../../ps2/veronica/prog/ps2_NaMath.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaView.h"
#include "../../../ps2/veronica/prog/pwksub.h"

// 99.40% matching
void bhCheckWall(BH_PWORK* pw) 
{
    NJS_POINT3* npos;  
    NJS_POINT3 pd;    
    NJS_LINE l;        
    ATR_WORK* hp;   
    ATR_WORK ht;      
    int i, j;            
    int hit;         
    int wal_n;      
    unsigned int attr; 
    float* psp;        
    float hpx, hpy, hpz;         
    float hw, hh, hd;         
    float px, pz;         
    float xn, zn;        
    float ln;         
    float par, pah;        
    float wpx, wpz;    
    float abx, abz;        
    int r;           
    NJS_VECTOR vec0, vec1;   
    float inn;         
    int eno; // not from DWARF
    
    npos = (NJS_POINT3*)&pw->px;
    
    npos->y = (int)(100.0f * (0.001f + npos->y)) / 100.0f;
    
    par = pw->ar;
    pah = pw->ah;
    
    if ((pw->stflg & 0x20000)) 
    {
        bhSetDansaLimitAtari(pw);
    }
    
    hit = 0;
    
    wal_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < wal_n; i++) 
    {
        if (i < rom->wal_n) 
        {
            hp = &rom->walp[i];
        } 
        else 
        {
            hp = &sys->mwalp[i - rom->wal_n];
        } 
        
        if ((hp->flg & 0x1))
        {
            attr = hp->attr;
            
            psp = &hp->px;
            
            hpx = psp[0];
            hpy = (int)(100.0f * (0.001f + psp[1])) / 100.0f;
            hpz = psp[2]; 
            
            hw = psp[3];
            hh = psp[4]; 
            hd = psp[5]; 
            
            if (!hh) 
            {
                hh = rom->h;
            }
            
            switch (hp->type) 
            {     
            case 0:                             
            case 1:                             
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) && ((!(attr & 0x2)) || (!(pw->flg & 0x4000))) && ((!(attr & 0x4)) || (!(pw->stflg & 0x40000000)))) 
                {
                    wpx = hpx + (0.5f * hw);
                    wpz = hpz + (0.5f * hd);
                    
                    px = hpx - par;
                    pz = hpz - par;
                    
                    xn = hw + (2.0f * par);
                    zn = hd + (2.0f * par);
                    
                    if ((((npos->x - px) >= 0) && ((npos->x - px) < xn)) && (((npos->z - pz) >= 0) && ((npos->z - pz) < zn)) && (((npos->y + pah) >= hpy) && (npos->y <= (hpy + hh))))
                    {
                        hit++;
                        
                        if (npos->x < wpx) 
                        {
                            if (npos->z < wpz) 
                            {
                                abx = fabsf(npos->x - px);
                                abz = fabsf(npos->z - pz);
                                
                                if ((abx < par) && (abz < par))
                                {
                                    px = hpx - npos->x;
                                    pz = hpz - npos->z;
                                    
                                    ln = njSqrt((px * px) + (pz * pz));
                                    
                                    if (ln <= par) 
                                    {
                                        r = 10430.381f * atan2f(px, pz);
                                        
                                        njSinCos(r, &npos->x, &npos->z);
                                        
                                        npos->x = hpx - (npos->x * par);
                                        npos->z = hpz - (npos->z * par);
                                    }
                                    
                                    pw->psh_ct = 0;
                                } 
                                else if (abx > abz) 
                                {
                                    npos->z = pz;
                                }
                                else 
                                {
                                    npos->x = px;
                                }
                            }
                            else 
                            {
                                abx = fabsf(npos->x - px);
                                abz = fabsf(npos->z - (pz + zn));
                                
                                if ((abx < par) && (abz < par))
                                {
                                    px = hpx        - npos->x;
                                    pz = (hpz + hd) - npos->z;
                                    
                                    ln = njSqrt((px * px) + (pz * pz));
                                    
                                    if (ln <= par) 
                                    {
                                        r = 10430.381f * atan2f(px, pz);
                                        
                                        njSinCos(r, &npos->x, &npos->z);
                                        
                                        npos->x = hpx        - (npos->x * par);
                                        npos->z = (hpz + hd) - (npos->z * par);
                                    }
                                    
                                    pw->psh_ct = 0;
                                } 
                                else if (abx > abz)
                                {
                                    npos->z = pz + zn;
                                } 
                                else
                                {
                                    npos->x = px;
                                }
                            }
                        } 
                        else 
                        {
                            if (npos->z < wpz)
                            {
                                abx = fabsf(npos->x - (px + xn));
                                abz = fabsf(npos->z - pz);
                                
                                if ((abx < par) && (abz < par)) 
                                {
                                    px = (hpx + hw) - npos->x;
                                    pz = hpz        - npos->z;
                                    
                                    ln = njSqrt((px * px) + (pz * pz));
                                    
                                    if (ln <= par) 
                                    {
                                        r = 10430.381f * atan2f(px, pz);
                                        
                                        njSinCos(r, &npos->x, &npos->z);
                                        
                                        npos->x = (hpx + hw) - (npos->x * par);
                                        npos->z = hpz        - (npos->z * par);
                                    }
                                    
                                    pw->psh_ct = 0;
                                } 
                                else if (abx > abz) 
                                {
                                    npos->z = pz;
                                } 
                                else 
                                {
                                    npos->x = px + xn;
                                }
                            }
                            else 
                            {
                                abx = fabsf(npos->x - (px + xn));
                                abz = fabsf(npos->z - (pz + zn));
                                
                                if ((abx < par) && (abz < par)) 
                                {
                                    px = (hpx + hw) - npos->x;
                                    pz = (hpz + hd) - npos->z;
                                    
                                    ln = njSqrt((px * px) + (pz * pz));
                                    
                                    if (ln <= par)
                                    {
                                        r = 10430.381f * atan2f(px, pz);
                                        
                                        njSinCos(r, &npos->x, &npos->z);
                                        
                                        npos->x = (hpx + hw) - (npos->x * par);
                                        npos->z = (hpz + hd) - (npos->z * par);
                                    }
                                    
                                    pw->psh_ct = 0;
                                } 
                                else if (abx > abz) 
                                {
                                    npos->z = pz + zn;
                                } 
                                else 
                                {
                                    npos->x = px + xn;
                                }
                            }
                        }
                    }
                }
                
                break;
            case 2:                                     
            case 3:                                     
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) && ((!(attr & 0x2)) || (!(pw->flg & 0x4000))) && ((!(attr & 0x4)) || (!(pw->stflg & 0x40000000))) && ((!(attr & 0x40)) || (!(pw->stflg & 0x4000))))
                {
                    px = hpx - npos->x;
                    pz = hpz - npos->z;
                    
                    ln = njSqrt((px * px) + (pz * pz));
                    
                    hw = par + hw; 
                    
                    if ((ln < hw) && (((npos->y + pah) >= hpy) && (npos->y <= (hpy + hh))))
                    {
                        hit++;
                        
                        if (((attr & 0x40)) && ((pw->stflg & 0x40000000)) && (!(plp->flg & 0x6)))
                        {
                            njSinCos(pw->ay, &vec0.x, &vec0.z);
                            
                            vec0.x = -vec0.x;
                            vec0.y = 0;
                            vec0.z = -vec0.z;
                            
                            vec1.x = px;
                            vec1.y = 0;
                            vec1.z = pz;
                            
                            njUnitVector(&vec1);
                            
                            inn = njInnerProduct(&vec0, &vec1);
                            
                            if (inn > 0) 
                            {
                                *(int*)&plp->mode0 = 2;
                            } 
                            else 
                            {
                                *(int*)&plp->mode0 = 0x102;
                            }
                            
                            plp->flg    |= 0x10004;
                            sys->st_flg |= 0x4;
                            
                            j = bhSetEffect(29, (POINT*)&plp->px, &hp->flg, 0);
                            
                            if (j != -1)
                            {
                                eff[j].flg &= ~0x80;
                                hp->attr   &= ~0x40;
                            }
                        }
                        
                        r = 10430.381f * atan2f(px, pz);
                        
                        njSinCos(r, &npos->x, &npos->z);
                        
                        npos->x = hpx - (npos->x * hw);
                        npos->z = hpz - (npos->z * hw);
                    }
                }
                
                break;
            case 4:                             
            case 5:                             
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) && ((!(attr & 0x2)) || (!(pw->flg & 0x4000))) && ((!(attr & 0x4)) || (!(pw->stflg & 0x40000000))))
                {
                    if (bhCheckInnerTriangle(hp, npos, par, pah) != 0) 
                    {
                        hit++;
            
                        l.vy = 0;
                        l.py = 0;
                        
                        switch (hp->id) 
                        { 
                        case 0:     
                            px = hpx + par;
                            pz = hpz + par;
                            break;
                        case 1:     
                            px = hpx + par;
                            pz = hpz - par;
                            break;
                        case 2:     
                            px = hpx - par;
                            pz = hpz + par;
                            break;
                        case 3:     
                            px = hpx - par;
                            pz = hpz - par;
                            break;
                        }
                        
                        l.px = px;
                        l.pz = pz + hd;
                        
                        l.vx = hw;
                        l.vz = -hd;
                        
                        njDistanceP2L(npos, &l, &pd);
                        
                        npos->x = pd.x; 
                        npos->z = pd.z;
                    } 
                    else 
                    {
                        switch (hp->id)
                        { 
                        case 0:     
                            ht.px = hpx;
                            ht.py = hpy;
                            ht.pz = hpz;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 12) != 0)
                            {
                                hit++;
                            }
                            
                            ht.w = hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 10) != 0) 
                            {
                                hit++;
                            } 
                            
                            break;
                        case 1:     
                            ht.px = hpx;
                            ht.py = hpy;
                            ht.pz = hpz + hd;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = -hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 12) != 0) 
                            {
                                hit++;
                            }
                            
                            r = 0;
                            
                            ht.pz = hpz - r;
                            
                            ht.w = hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 5) != 0)
                            {
                                hit++;
                            }
                            
                            break;
                        case 2:     
                            r = 0;
                            
                            ht.px = hpx - r;
                            ht.py = hpy;
                            ht.pz = hpz;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 3) != 0) 
                            {
                                hit++;
                            }
                            
                            ht.px = hpx + hw;
                            
                            ht.w = -hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 10) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        case 3:     
                            r = 0;
                            
                            ht.px = hpx - r;
                            ht.py = hpy;
                            ht.pz = hpz + hd;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = -hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 3) != 0) 
                            {
                                hit++;
                            }
                            
                            r = 0;
                            
                            ht.px = hpx + hw;
                            ht.pz = hpz - r; 
                            
                            ht.w = -hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 5) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        }
                    }
                }
    
                break;
            case 6:                             
                if (((((npos->x - hpx) >= 0) && ((npos->x - hpx) < hw)) && (((npos->z - hpz) >= 0) && ((npos->z - hpz) < hd)) && (((npos->y + pah) >= hpy) && (npos->y <= (hpy + hh)))) && ((pw->flg & 0x100))) 
                {
                    hit++;
                    
                    switch (hp->id) 
                    { 
                    case 0:             
                        l.px = hpx;
                        l.py = hpy;
                        l.pz = npos->z;
                        
                        l.vx = hw;
                        l.vy = hh;
                        l.vz = 0;
                        break;
                    case 1:             
                        l.px = npos->x;
                        l.py = hpy;
                        l.pz = hpz + hd;
                        
                        l.vx = 0;
                        l.vy = hh;
                        l.vz = -hd;
                        break;
                    case 2:             
                        l.px = npos->x;
                        l.py = hpy;
                        l.pz = hpz;
                        
                        l.vx = 0;
                        l.vy = hh;
                        l.vz = hd;
                        break;
                    case 3:             
                        l.px = hpx + hw;
                        l.py = hpy;
                        l.pz = npos->z;
                        
                        l.vx = -hw;
                        l.vy = hh;
                        l.vz = 0;
                        break;
                    }
                    
                    njDistanceP2L(npos, &l, &pd);
                    
                    npos->y = pd.y;
                    
                    for (j = 0; j < 31; j++) 
                    {
                        if (((rom->grand[j]) && (npos->y >= rom->grand[j])) || ((j == 2) && (npos->y >= rom->grand[j]))) 
                        {
                            pw->flr_no = j - 2;
                        }
                    } 
                    
                    if (pw->flr_no != hp->flr_no)
                    {
                        if (npos->y < rom->grand[pw->flr_no + 2]) 
                        {
                            npos->x = pd.x;
                            npos->z = pd.z;
                        } 
                        else 
                        {
                            npos->y = rom->grand[pw->flr_no + 2];
                        }
                    } 
                    else 
                    {
                        npos->x = pd.x;
                        npos->z = pd.z;
                        
                        if (npos->y < rom->grand[pw->flr_no + 2]) 
                        {
                            npos->y = rom->grand[pw->flr_no + 2];
                        }
                    }
                }
                
                break;
            case 7:                                     
                if ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) 
                {
                    wpx = hpx + (0.5f * hw);
                    wpz = hpz + (0.5f * hd);
                    
                    px = hpx - par;
                    pz = hpz - par;
                    
                    xn = hw + (2.0f * par);
                    zn = hd + (2.0f * par);
                    
                    if (((((npos->x - px) >= 0) && ((npos->x - px) < xn)) && (((npos->z - pz) >= 0) && ((npos->z - pz) < zn)) && ((npos->y < (hpy + hh)) && ((npos->y + pah) > (hpy + hh))) && ((pw->flg & 0x100))) || ((((npos->x - px) >= 0) && ((npos->x - px) < xn)) && (((npos->z - pz) >= 0) && ((npos->z - pz) < zn)) && (((npos->y + pah) >= (hpy + hh)) && (npos->y < hpy)) && (!(pw->flg & 0x100))))
                    {
                        hit++;
                        
                        if (npos->x < wpx)
                        {
                            if (npos->z < wpz) 
                            {
                                if (fabsf(npos->x - px) > fabsf(npos->z - pz)) 
                                {
                                    npos->z = pz;
                                }
                                else 
                                {
                                    npos->x = px;
                                }
                            }
                            else
                            {
                                if (fabsf(npos->x - px) > fabsf(npos->z - (pz + zn)))
                                {
                                    npos->z = pz + zn;
                                } 
                                else 
                                {
                                    npos->x = px;
                                }
                            }
                        }
                        else 
                        {
                            if (npos->z < wpz) 
                            {
                                if (fabsf(npos->x - (px + xn)) > fabsf(npos->z - pz))
                                {
                                    npos->z = pz;
                                } 
                                else
                                {
                                    npos->x = px + xn;
                                }
                            }
                            else
                            {
                                if (fabsf(npos->x - (px + xn)) > fabsf(npos->z - (pz + zn))) 
                                {
                                    npos->z = pz + zn; 
                                }
                                else 
                                {
                                    npos->x = px + xn; 
                                }
                            }
                        } 
                    }
                } 
                
                break;
            }
        }
    }
    
    if (hit != 0)
    {
        pw->stflg |= 0x1;
    }
    
    if ((hit == 0) && ((pw->flg & 0x100)))
    {
        npos->y = rom->grand[pw->flr_no + 2]; 
    }
    
    if ((pw->stflg & 0x20000)) 
    {
        sys->mwal_n -= sys->dla_n;
    }
}

#ifdef TARGET_PC
/* Ground height on the type-6 slope walls under pos (x/z inside the box, pos y within the box's
   height band extended down by the actor's height ah, as bhCheckWall's case 6 tests, and up by it:
   walking down onto the next box, last frame's y lies above that box's top). id: 0 rising
   +x, 1 rising -z, 2 rising +z, 3 rising -x, from py to py + h (h 0 = rom->h). Several slopes:
   the surface nearest to pos y. Unlike bhCheckWall's case 6 (enemies), which projects the actor
   onto the slope's line and so also moves it along x/z, only y changes: continuous terrain made of
   adjacent slope boxes is walked at the boxes' heights. Sets pos y and the actor's floor number
   (from the height, as bhSetFloorNum); 0 = on no slope, nothing changed. */
int pc_slope_ground(struct BH_PWORK* pw, void* pos, float ah)
{
    NJS_POINT3* p = (NJS_POINT3*)pos;
    ATR_WORK* hp;
    int i, fno, n = rom->wal_n + sys->mwal_n, on = 0;
    float u, v, h, s, y = 0;

    for (i = 0; i < n; i++)
    {
        hp = i < rom->wal_n ? &rom->walp[i] : &sys->mwalp[i - rom->wal_n];
        if (!(hp->flg & 0x1) || hp->type != 6 || hp->id > 3) continue;
        h = hp->h ? hp->h : rom->h;
        u = p->x - hp->px;
        v = p->z - hp->pz;
        if (u < 0 || u >= hp->w || v < 0 || v >= hp->d || p->y + ah < hp->py || p->y - fabsf(ah) > hp->py + h) continue;
        s = hp->id == 0 ? u / hp->w : hp->id == 1 ? 1.0f - v / hp->d : hp->id == 2 ? v / hp->d : 1.0f - u / hp->w;
        s = hp->py + s * h;
        if (!on || fabsf(s - p->y) < fabsf(y - p->y)) y = s;
        on = 1;
    }
    if (!on) return 0;
    p->y = y;
    fno = bhCheckFloorNum(y);
    if (fno != pw->flr_no)
    {
        pw->flr_nob = pw->flr_no;
        pw->flr_no = fno;
    }
    return 1;
}
#endif

// 100% matching!
int bhCheckWallEx(BH_PWORK* pw, NJS_POINT3* npos, NJS_POINT3* opos, float par, float pah)
{
    NJS_POINT3 pd;     
    NJS_LINE l;      
    ATR_WORK* hp;     
    ATR_WORK ht;       
    int i, j;             
    int hit;        
    int wal_n;       
    int pcflg;        
    unsigned int attr;
    float* psp;       
    float hpx, hpy, hpz;        
    float hw, hh, hd;        
    float px, pz;        
    float xn, zn;         
    float ln;         
    float wpx, wpz;       
    float abx, abz;        
    float pxx, pzz;        
    int r;            
    int ayp;         
    NJS_VECTOR vec0, vec1;  
    float inn;        

    if ((pw->stflg & 0x20000)) 
    {
        bhSetDansaLimitAtari(pw);
    }
    
    hit = 0;
    
    wal_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < wal_n; i++) 
    {
        if (i < rom->wal_n) 
        {
            hp = &rom->walp[i];
        }
        else
        {
            hp = &sys->mwalp[i - rom->wal_n];
        }
        
        if ((hp->flg & 0x1))
        {
            attr = hp->attr;
            
            hpx = hp->px;
            hpy = hp->py;
            hpz = hp->pz;
            
            hw = hp->w;
            hh = hp->h; 
            hd = hp->d;
            
            if (!hh) 
            {
                hh = rom->h;
            }
            
            switch (hp->type) 
            {                  
            case 0:                                     
            case 1:                                     
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) && ((!(attr & 0x2)) || (!(pw->flg & 0x4000))) && ((!(attr & 0x4)) || (!(pw->stflg & 0x40000000)))) 
                {
                    wpx = hpx + (0.5f * hw);
                    wpz = hpz + (0.5f * hd);
                    
                    px = hpx - par;
                    pz = hpz - par;
                    
                    xn = hw + (2.0f * par);
                    zn = hd + (2.0f * par);
                    
                    if ((((npos->x - px) >= 0) && ((npos->x - px) < xn)) && (((npos->z - pz) >= 0) && ((npos->z - pz) < zn)) && (((npos->y + pah) >= hpy) && (npos->y <= (hpy + hh))))
                    {
                        hit++;
                        
                        if (((attr & 0x10000)) && (pw == plp))
                        {
                            ayp = (pw->ay + 8192) & ~0x3FFF;
                            
                            pcflg = 0;
                            
                            switch (ayp & 0xC000) 
                            { 
                            case 0: 
                                if ((attr & 0x40000)) 
                                {
                                    pcflg = 1;
                                }
                                
                                break;
                            case 0x4000: 
                                if ((attr & 0x200000)) 
                                {
                                    pcflg = 1;
                                }
                                
                                break;
                            case 0x8000: 
                                if ((attr & 0x100000)) 
                                {
                                    pcflg = 1;
                                }
                                
                                break;
                            case 0xC000: 
                                if ((attr & 0x80000)) 
                                {
                                    pcflg = 1;
                                }
                                
                                break;
                            }
                            
                            if (pcflg != 0) 
                            {
                                pw->psh_ct = 0;
                            } 
                            else 
                            {
                                njSinCos(ayp, &pxx, &pzz);
                                
                                pxx = npos->x - (5.0f * pxx);
                                pzz = npos->z - (5.0f * pzz);
                                
                                if (((hpx <= pxx) && ((hpx + hw) >= pxx)) && ((hpz <= pzz) && ((hpz + hd) >= pzz))) 
                                {
                                    pw->psh_ct++;
                                } 
                                else 
                                {
                                    pw->psh_ct = 0;
                                }
                            }
                        }
                        
                        if ((!(attr & 0x20000)) && ((pw->stflg & 0x80))) 
                        {
                            hp->attr |= 0x20000;
                            
                            pw->psh_idx = i;
                        }
                        
                        if (opos->x < wpx) 
                        {
                            if (opos->z < wpz) 
                            {
                                abx = fabsf(npos->x - px);
                                abz = fabsf(npos->z - pz);
                                
                                if ((abx < par) && (abz < par)) 
                                {
                                    px = hpx - npos->x;
                                    pz = hpz - npos->z;
                                    
                                    ln = njSqrt((px * px) + (pz * pz));
                                    
                                    if (ln <= par) 
                                    {
                                        r = 10430.381f * atan2f(px, pz);
                                        
                                        njSinCos(r, &npos->x, &npos->z);
                                        
                                        npos->x = hpx - (npos->x * par);
                                        npos->z = hpz - (npos->z * par);
                                    }
                                    
                                    pw->psh_ct = 0;
                                }
                                else
                                {
                                    abx = fabsf(opos->x - px);
                                    abz = fabsf(opos->z - pz);
                                    
                                    if (abx > abz)
                                    {
                                        npos->z = pz;
                                    }
                                    else
                                    {
                                        npos->x = px;
                                    }
                                }
                            } 
                            else
                            {
                                abx = fabsf(npos->x - px);
                                abz = fabsf(npos->z - (pz + zn));
                                
                                if ((abx < par) && (abz < par))
                                {
                                    px = hpx        - npos->x;
                                    pz = (hpz + hd) - npos->z; 
                                    
                                    ln = njSqrt((px * px) + (pz * pz));
                                    
                                    if (ln <= par)
                                    {
                                        r = 10430.381f * atan2f(px, pz);
                                        
                                        njSinCos(r, &npos->x, &npos->z);
                                        
                                        npos->x = hpx        - (npos->x * par);
                                        npos->z = (hpz + hd) - (npos->z * par);
                                    }
                                    
                                    pw->psh_ct = 0;
                                } 
                                else 
                                {
                                    abx = fabsf(opos->x - px);
                                    abz = fabsf(opos->z - (pz + zn));
                                    
                                    if (abx > abz) 
                                    {
                                        npos->z = pz + zn;
                                    } 
                                    else
                                    {
                                        npos->x = px;
                                    }
                                }
                            }
                        } 
                        else if (opos->z < wpz)
                        {
                            abx = fabsf(npos->x - (px + xn));
                            abz = fabsf(npos->z - pz);
                            
                            if ((abx < par) && (abz < par)) 
                            {
                                px = (hpx + hw) - npos->x;
                                pz = hpz        - npos->z;
                                
                                ln = njSqrt((px * px) + (pz * pz));
                                
                                if (ln <= par) 
                                {
                                    r = 10430.381f * atan2f(px, pz);
                                    
                                    njSinCos(r, &npos->x, &npos->z);
                                    
                                    npos->x = (hpx + hw) - (npos->x * par);
                                    npos->z = hpz        - (npos->z * par);
                                }
                                
                                pw->psh_ct = 0;
                            } 
                            else 
                            {
                                abx = fabsf(opos->x - (px + xn));
                                abz = fabsf(opos->z - pz);
                                
                                if (abx > abz) 
                                {
                                    npos->z = pz;
                                }
                                else
                                {
                                    npos->x = px + xn;
                                }
                            }
                        } 
                        else 
                        {
                            abx = fabsf(npos->x - (px + xn));
                            abz = fabsf(npos->z - (pz + zn));
                            
                            if ((abx < par) && (abz < par)) 
                            {
                                px = (hpx + hw) - npos->x;
                                pz = (hpz + hd) - npos->z;
                                
                                ln = njSqrt((px * px) + (pz * pz));
                                
                                if (ln <= par)
                                {
                                    r = 10430.381f * atan2f(px, pz);
                                    
                                    njSinCos(r, &npos->x, &npos->z);
                                    
                                    npos->x = (hpx + hw) - (npos->x * par);
                                    npos->z = (hpz + hd) - (npos->z * par);
                                }
                                
                                pw->psh_ct = 0;
                            }
                            else
                            {
                                abx = fabsf(opos->x - (px + xn));
                                abz = fabsf(opos->z - (pz + zn));
                                
                                if (abx > abz)
                                {
                                    npos->z = pz + zn; 
                                } 
                                else
                                {
                                    npos->x = px + xn;
                                }
                            }
                        }
                    }
                }
                
                break;
            case 2:                                     
            case 3:                                     
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) && ((!(attr & 0x2)) || (!(pw->flg & 0x4000))) && ((!(attr & 0x4)) || (!(pw->stflg & 0x40000000))) && ((!(attr & 0x40)) || (!(pw->stflg & 0x4000))))
                {
                    px = hpx - npos->x;
                    pz = hpz - npos->z;
                    
                    ln = njSqrt((px * px) + (pz * pz));
                    
                    hw = par + hw; 
                    
                    if ((ln < hw) && (((npos->y + pah) >= hpy) && (npos->y <= (hpy + hh))))
                    {
                        hit++;
                        
                        if (((attr & 0x40)) && ((pw->stflg & 0x40000000)) && (!(plp->flg & 0x6)))
                        {
                            njSinCos(pw->ay, &vec0.x, &vec0.z);
                            
                            vec0.x = -vec0.x;
                            vec0.y = 0;
                            vec0.z = -vec0.z;
                            
                            vec1.x = px;
                            vec1.y = 0;
                            vec1.z = pz;
                            
                            njUnitVector(&vec1);
                            
                            inn = njInnerProduct(&vec0, &vec1);
                            
                            if (inn > 0) 
                            {
                                *(int*)&plp->mode0 = 2;
                            } 
                            else 
                            {
                                *(int*)&plp->mode0 = 0x102;
                            }
                            
                            plp->flg    |= 0x10004;
                            sys->st_flg |= 0x4;
                            
                            j = bhSetEffect(29, (POINT*)&plp->px, &hp->flg, 0);
                            
                            if (j != -1)
                            {
                                eff[j].flg &= ~0x80;
                                hp->attr   &= ~0x40;
                            }
                        }
                        
                        r = 10430.381f * atan2f(px, pz);
                        
                        njSinCos(r, &npos->x, &npos->z);
                        
                        npos->x = hpx - (npos->x * hw);
                        npos->z = hpz - (npos->z * hw);
                    }
                }
                
                break;
            case 4:                             
            case 5:                             
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) && ((!(attr & 0x2)) || (!(pw->flg & 0x4000))) && ((!(attr & 0x4)) || (!(pw->stflg & 0x40000000))))
                {
                    if (bhCheckInnerTriangle(hp, npos, par, pah) != 0) 
                    {
                        hit++;
                        
                        l.vy = 0;
                        l.py = 0;
                        
                        switch (hp->id) 
                        { 
                        case 0:     
                            px = hpx + par;
                            pz = hpz + par;
                            break;
                        case 1:     
                            px = hpx + par;
                            pz = hpz - par;
                            break;
                        case 2:     
                            px = hpx - par;
                            pz = hpz + par;
                            break;
                        case 3:     
                            px = hpx - par;
                            pz = hpz - par;
                            break;
                        }
                        
                        l.px = px;
                        l.pz = pz + hd;
                        
                        l.vx = hw;
                        l.vz = -hd;
                        
                        njDistanceP2L(npos, &l, &pd);
                        
                        npos->x = pd.x; 
                        npos->z = pd.z;
                    } 
                    else 
                    {
                        switch (hp->id)
                        { 
                        case 0:     
                            ht.px = hpx;
                            ht.py = hpy;
                            ht.pz = hpz;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 12) != 0)
                            {
                                hit++;
                            }
                            
                            ht.w = hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 10) != 0) 
                            {
                                hit++;
                            } 
                            
                            break;
                        case 1:     
                            ht.px = hpx;
                            ht.py = hpy;
                            ht.pz = hpz + hd;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = -hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 12) != 0) 
                            {
                                hit++;
                            }
                            
                            r = 0;
                            
                            ht.pz = hpz - r;
                            
                            ht.w = hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 5) != 0)
                            {
                                hit++;
                            }
                            
                            break;
                        case 2:     
                            r = 0;
                            
                            ht.px = hpx - r;
                            ht.py = hpy;
                            ht.pz = hpz;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 3) != 0) 
                            {
                                hit++;
                            }
                            
                            ht.px = hpx + hw;
                            
                            ht.w = -hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 10) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        case 3:     
                            r = 0;
                            
                            ht.px = hpx - r;
                            ht.py = hpy;
                            ht.pz = hpz + hd;
                            
                            ht.w = 0;
                            ht.h = hh;
                            ht.d = -hd;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 3) != 0) 
                            {
                                hit++;
                            }
                            
                            r = 0;
                            
                            ht.px = hpx + hw;
                            ht.pz = hpz - r; 
                            
                            ht.w = -hw;
                            ht.h = hh;
                            ht.d = 0;
                            
                            if (bhCheckBox(&ht, npos, par, pah, 5) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        }
                    }
                }
                
                break;
            case 7:                                     
                if ((!(attr & 0x1)) || (hp->flr_no == pw->flr_no)) 
                {
                    wpx = hpx + (0.5f * hw);
                    wpz = hpz + (0.5f * hd);
                    
                    px = hpx - par;
                    pz = hpz - par;
                    
                    xn = hw + (2.0f * par);
                    zn = hd + (2.0f * par);
                    
                    if (((((npos->x - px) >= 0) && ((npos->x - px) < xn)) && (((npos->z - pz) >= 0) && ((npos->z - pz) < zn)) && ((npos->y < (hpy + hh)) && ((npos->y + pah) > (hpy + hh))) && ((pw->flg & 0x100))) || ((((npos->x - px) >= 0) && ((npos->x - px) < xn)) && (((npos->z - pz) >= 0) && ((npos->z - pz) < zn)) && (((npos->y + pah) >= (hpy + hh)) && (npos->y < hpy)) && (!(pw->flg & 0x100))))
                    {
                        hit++;
                        
                        if (opos->x < wpx)
                        {
                            if (opos->z < wpz) 
                            {
                                if (fabsf(npos->x - px) > fabsf(npos->z - pz)) 
                                {
                                    npos->z = pz;
                                }
                                else 
                                {
                                    npos->x = px;
                                }
                            }
                            else
                            {
                                if (fabsf(npos->x - px) > fabsf(npos->z - (pz + zn)))
                                {
                                    npos->z = pz + zn;
                                } 
                                else 
                                {
                                    npos->x = px;
                                }
                            }
                        }
                        else 
                        {
                            if (opos->z < wpz) 
                            {
                                if (fabsf(npos->x - (px + xn)) > fabsf(npos->z - pz))
                                {
                                    npos->z = pz;
                                } 
                                else
                                {
                                    npos->x = px + xn;
                                }
                            }
                            else
                            {
                                if (fabsf(npos->x - (px + xn)) > fabsf(npos->z - (pz + zn))) 
                                {
                                    npos->z = pz + zn; 
                                }
                                else 
                                {
                                    npos->x = px + xn; 
                                }
                            }
                        } 
                    }
                } 
                
                break;
            }
        }
    }
    
    if (hit != 0)
    {
        pw->stflg |= 0x1;
    }
    
#ifdef TARGET_PC
    /* the player's (and these actors') wall check had no slope case: on the floor level they
       stayed. In "budget": "pc" rooms they stand on the slope instead (pc_slope_ground). */
    if (((pw->flg & 0x100)) && ((rom->flg & PC_ROOM_SLOPES)) && (pc_slope_ground(pw, npos, pah) != 0))
    {
    }
    else
#endif
    if ((hit == 0) && ((pw->flg & 0x100)))
    {
        npos->y = rom->grand[pw->flr_no + 2]; 
    }
    
    if ((pw->stflg & 0x20000)) 
    {
        sys->mwal_n -= sys->dla_n;
    }
    
    if (hit != 0) 
    {
        return 1;
    }
    else 
    { 
        return 0; 
    }
}

// 100% matching!
void bhCheckWall2Box(BH_PWORK* pw) 
{
    NJS_POINT3 pd; 
    NJS_LINE l;    
    ATR_WORK* hp; 
    ATR_WORK ht;  
    int i, j;        
    int hit;      
    int wal_n;     
    float px, pz;      
    float xn, zn;     
    float ln;     
    float wpx, wpz;   
    float h;       
    float abx, abz;   
    float pxx, pzz;     
    int r;     
  
    hit = 0;
    
    wal_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < wal_n; i++) 
    {
        if (i < rom->wal_n) 
        {
            hp = &rom->walp[i];
        } 
        else 
        {
            hp = &sys->mwalp[i - rom->wal_n];
        }
        
        if ((hp->flg & 0x1)) 
        {
            if (hp->h) 
            {
                h = hp->h;
            } 
            else 
            {
                h = rom->h;
            }
            
            switch (hp->type) 
            {                  
            case 0:                             
            case 1:                             
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(hp->attr & 0x1)) || (hp->flr_no == pw->flr_no)) && (!((hp->attr & 0x2)) || (!(pw->flg & 0x4000))) && (!((hp->attr & 0x4)) || (!(pw->stflg & 0x40000000))) && ((!(hp->attr & 0x10000)) || (!(pw->flg & 0x2000)) || (hp->prm3 != (unsigned char)pw->idx_ct))) 
                {
                    wpx = hp->px + (0.5f * hp->w);
                    wpz = hp->pz + (0.5f * hp->d);
                    
                    px = hp->px - pw->aw; 
                    pz = hp->pz - pw->ad;
                    
                    xn = hp->w + (2.0f * pw->aw);
                    zn = hp->d + (2.0f * pw->ad);
                    
                    if ((((pw->px - px) >= 0) && ((pw->px - px) < xn)) && (((pw->pz - pz) >= 0) && ((pw->pz - pz) < zn)) && (((pw->py + pw->ah) >= hp->py) && (pw->py <= (hp->py + h)))) 
                    {
                        hit++;
                        
                        if (((hp->attr & 0x10000)) && (pw == plp)) 
                        {
                            njSinCos((pw->ay + 8192) & ~0x3FFF, &pxx, &pzz);
                            
                            pxx = pw->px - (4.0f * pxx);
                            pzz = pw->pz - (4.0f * pzz); 
                            
                            if (((hp->px <= pxx) && ((hp->px + hp->w) >= pxx)) && ((hp->pz <= pzz) && ((hp->pz + hp->d) >= pzz)))
                            {
                                pw->psh_ct++;
                            } 
                            else 
                            {
                                pw->psh_ct = 0;
                            }
                        }
                        
                        if ((!(hp->attr & 0x20000)) && ((pw->stflg & 0x80)))
                        {
                            hp->attr |= 0x20000;
                            
                            pw->psh_idx = i;
                        }
                        
                        if (pw->px < wpx) 
                        {
                            if (pw->pz < wpz) 
                            {
                                abx = fabsf(pw->px - px);
                                abz = fabsf(pw->pz - pz);
                                
                                if (abx > abz)
                                {
                                    pw->pz = pz;
                                }
                                else 
                                {
                                    pw->px = px; 
                                }
                            }
                            else 
                            {
                                abx = fabsf(pw->px - px);
                                abz = fabsf(pw->pz - (pz + zn));
                                
                                if (abx > abz)
                                {
                                    pw->pz = pz + zn;
                                } 
                                else
                                {
                                    pw->px = px;
                                }
                            }
                        } 
                        else 
                        {
                            if (pw->pz < wpz)
                            { 
                                abx = fabsf(pw->px - (px + xn));
                                abz = fabsf(pw->pz - pz);
                                
                                if (abx > abz)
                                {
                                    pw->pz = pz;
                                } 
                                else 
                                {
                                    pw->px = px + xn;
                                }
                            } 
                            else 
                            {
                                abx = fabsf(pw->px - (px + xn));
                                abz = fabsf(pw->pz - (pz + zn));
                                
                                if (abx > abz) 
                                {
                                    pw->pz = pz + zn;
                                } 
                                else 
                                {
                                    pw->px = px + xn;
                                }
                            }
                        }
                    }
                }
                
                break;
            case 2:                             
            case 3:                             
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(hp->attr & 0x1)) || (hp->flr_no == pw->flr_no)) && (!((hp->attr & 0x2)) || (!(pw->flg & 0x4000))) && (!((hp->attr & 0x4)) || (!(pw->stflg & 0x40000000))) && (!((hp->attr & 0x40)) || (!(pw->flg & 0x2000))))
                {
                    px = (hp->px - pw->aw) - hp->w;
                    pz = (hp->pz - pw->ad) - hp->w;
                    
                    xn = (2.0f * hp->w) + (2.0f * pw->aw);
                    zn = (2.0f * hp->w) + (2.0f * pw->ad);
                    
                    if ((((pw->px - px) >= 0) && ((pw->px - px) < xn)) && (((pw->pz - pz) >= 0) && ((pw->pz - pz) < zn)) && (((pw->py + pw->ah) >= hp->py) && (pw->py <= (hp->py + h)))) 
                    {
                        if (hp->px < pw->px)
                        {
                            if (hp->pz < pw->pz) 
                            {
                                abx = fabsf(pw->px - (pw->aw + (hp->px + hp->w)));
                                abz = fabsf(pw->pz - (pw->ad + (hp->pz + hp->w)));
                                
                                if (abx < abz) 
                                {  
                                    if (fabsf(hp->pz - pw->pz) < pw->ad) 
                                    {
                                        hit++;
                                        
                                        pw->px = pw->aw + (hp->px + hp->w);
                                    }
                                } 
                                else if (fabsf(hp->px - pw->px) < pw->aw) 
                                {
                                    hit++;
                                    
                                    pw->pz = pw->ad + (hp->pz + hp->w);
                                }
                            } 
                            else 
                            {
                                abx = fabsf(pw->px - (pw->aw + (hp->px + hp->w)));
                                abz = fabsf(pw->pz - pz);
                                
                                if (abx < abz) 
                                {
                                    if (fabsf(hp->pz - pw->pz) < pw->ad)
                                    {
                                        hit++;
                                        
                                        pw->px = pw->aw + (hp->px + hp->w);
                                    }
                                } 
                                else if (fabsf(hp->px - pw->px) < pw->aw) 
                                {
                                    hit++;
                                    
                                    pw->pz = pz;
                                }
                            }
                        } 
                        else if (hp->pz < pw->pz) 
                        {
                            abx = fabsf(pw->px - px);
                            abz = fabsf(pw->pz - (pw->ad + (hp->pz + hp->w)));
                            
                            if (abx < abz) 
                            {
                                if (fabsf(hp->pz - pw->pz) < pw->ad)
                                {
                                    hit++;
                                    
                                    pw->px = px;
                                }
                            } 
                            else if (fabsf(hp->px - pw->px) < pw->aw) 
                            {
                                hit++;
                                
                                pw->pz = pw->ad + (hp->pz + hp->w);
                            }
                        } 
                        else 
                        {
                            abx = fabsf(pw->px - px);
                            abz = fabsf(pw->pz - pz);
                            
                            if (abx < abz) 
                            {
                                if (fabsf(hp->pz - pw->pz) < pw->ad) 
                                {
                                    hit++;
                                    
                                    pw->px = px;
                                }
                            } 
                            else if (fabsf(hp->px - pw->px) < pw->aw) 
                            {
                                hit++;
                                
                                pw->pz = pz;
                            }
                        }
                        
                        px = hp->px - (pw->px - pw->aw); 
                        pz = hp->pz - (pw->pz - pw->ad);
                        
                        ln = njSqrt((px * px) + (pz * pz));
                        
                        if (ln < hp->w)
                        {
                            hit++;
                            
                            r = 10430.381f * atan2f(px, pz);
                            
                            njSinCos(r, &pw->px, &pw->pz);
                            
                            pw->px = pw->aw + (hp->px - (pw->px * hp->w));
                            pw->pz = pw->ad + (hp->pz - (pw->pz * hp->w));
                        } 
                        else 
                        {
                            px = hp->px - (pw->px + pw->aw); 
                            pz = hp->pz - (pw->pz - pw->ad);
                            
                            ln = njSqrt((px * px) + (pz * pz));
                            
                            if (ln < hp->w) 
                            {
                                hit++;
                                
                                r = 10430.381f * atan2f(px, pz);
                                
                                njSinCos(r, &pw->px, &pw->pz);
                                
                                pw->px = (hp->px - (pw->px * hp->w)) - pw->aw;
                                pw->pz = pw->ad + (hp->pz - (pw->pz * hp->w));
                            } 
                            else 
                            {
                                px = hp->px - (pw->px - pw->aw); 
                                pz = hp->pz - (pw->pz + pw->ad);
                                
                                ln = njSqrt((px * px) + (pz * pz));
                                
                                if (ln < hp->w)
                                {
                                    hit++;
                                    
                                    r = 10430.381f * atan2f(px, pz);
                                    
                                    njSinCos(r, &pw->px, &pw->pz);
                                    
                                    pw->px = pw->aw + (hp->px - (pw->px * hp->w));
                                    pw->pz = (hp->pz - (pw->pz * hp->w)) - pw->ad;
                                } 
                                else 
                                {
                                    px = hp->px - (pw->px + pw->aw); 
                                    pz = hp->pz - (pw->pz + pw->ad);
                                    
                                    ln = njSqrt((px * px) + (pz * pz));
                                    
                                    if (ln < hp->w) 
                                    {
                                        hit++;

                                        r = 10430.381f * atan2f(px, pz);
                                        
                                        njSinCos(r, &pw->px, &pw->pz);
                                        
                                        pw->px = (hp->px - (pw->px * hp->w)) - pw->aw;
                                        pw->pz = (hp->pz - (pw->pz * hp->w)) - pw->ad;
                                    }
                                }
                            }
                        }
                    }
                }
                
                break;
            case 4:                             
            case 5:                             
                if (((!(hp->type & 0x1)) || (!(pw->flg & 0x400))) && ((!(hp->attr & 0x1)) || (hp->flr_no == pw->flr_no)) && (!((hp->attr & 0x2)) || (!(pw->flg & 0x4000))) && ((!(hp->attr & 0x4)) || (!(pw->stflg & 0x40000000))))
                {
                    if (bhCheckInnerTriangle2(hp, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                    {
                        l.vy = 0;
                        l.py = 0;
                        
                        hit++;
                        
                        switch (hp->id) 
                        { 
                        case 0:     
                            px = hp->px + pw->aw;
                            pz = hp->pz + pw->ad;
                            break;
                        case 1:     
                            px = hp->px + pw->aw;
                            pz = hp->pz - pw->ad;
                            break;
                        case 2:     
                            px = hp->px - pw->aw;
                            pz = hp->pz + pw->ad;
                            break;
                        case 3:     
                            px = hp->px - pw->aw;
                            pz = hp->pz - pw->ad;
                            break;
                        }
                        
                        l.px = px;
                        l.pz = pz + hp->d;
                        
                        l.vx =  hp->w;
                        l.vz = -hp->d;
                        
                        njDistanceP2L((NJS_POINT3*)&pw->px, &l, &pd);
                        
                        pw->px = pd.x;
                        pw->pz = pd.z; 
                    } 
                    else 
                    {
                        switch (hp->id)
                        { 
                        case 0:     
                            ht.px = hp->px;
                            ht.py = hp->py;
                            ht.pz = hp->pz;
                            
                            ht.w = 0;
                            ht.h = hp->h;
                            ht.d = hp->d;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0)
                            {
                                hit++;
                            }
                            
                            ht.w = hp->w;
                            ht.h = hp->h;
                            ht.d = 0;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        case 1:     
                            ht.px = hp->px;
                            ht.py = hp->py;
                            ht.pz = hp->pz + hp->d;
                            
                            ht.w =  0;
                            ht.h =  hp->h;
                            ht.d = -hp->d;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                            {
                                hit++;
                            }
                            
                            r = 0;
                            
                            ht.pz = hp->pz - r;
                            
                            ht.w = hp->w;
                            ht.h = hp->h;
                            ht.d = 0;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        case 2:     
                            r = 0;
                            
                            ht.px = hp->px - r;
                            ht.py = hp->py;
                            ht.pz = hp->pz;
                            
                            ht.w = 0;
                            ht.h = hp->h;
                            ht.d = hp->d;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                            {
                                hit++;
                            }
                            
                            ht.px = hp->px + hp->w;
                            
                            ht.w = -hp->w;
                            ht.h =  hp->h;
                            ht.d =  0;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        case 3:     
                            r = 0;
                            
                            ht.px = hp->px - r;
                            ht.py = hp->py;  
                            ht.pz = hp->pz + hp->d;
                            
                            ht.w =  0;
                            ht.h =  hp->h;
                            ht.d = -hp->d;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                            {
                                hit++;
                            }
                            
                            ht.px = hp->px + hp->w;
                            ht.pz = hp->pz - r;

                            ht.w = -hp->w;  
                            ht.h =  hp->h;
                            ht.d =  0;
                            
                            if (bhCheckBox2Box(&ht, (NJS_POINT3*)&pw->px, pw->aw, pw->ad, pw->ah) != 0) 
                            {
                                hit++;
                            }
                            
                            break;
                        }
                    }
                }
                
                break;
            case 6:                             
                if ((((pw->px - hp->px) >= 0) && ((pw->px - hp->px) < hp->w)) && (((pw->pz - hp->pz) >= 0) && ((pw->pz - hp->pz) < hp->d)) && (((pw->py + pw->ah) >= hp->py) && (pw->py <= (hp->py + h))) && ((pw->flg & 0x100)))
                {
                    hit++;
                    
                    switch (hp->id) 
                    { 
                    case 0:         
                        l.px = hp->px;
                        l.py = hp->py;
                        l.pz = pw->pz;
                        
                        l.vx = hp->w;
                        l.vy = hp->h;
                        l.vz = 0;
                        break;
                    case 1:         
                        l.px = pw->px;
                        l.py = hp->py;
                        l.pz = hp->pz + hp->d;
                        
                        l.vx =  0;
                        l.vy =  hp->h;
                        l.vz = -hp->d;
                        break;
                    case 2:         
                        l.px = pw->px;
                        l.py = hp->py;
                        l.pz = hp->pz;
                        
                        l.vx = 0;
                        l.vy = hp->h;
                        l.vz = hp->d;
                        break;
                    case 3:         
                        l.px = hp->px + hp->w;
                        l.py = hp->py;
                        l.pz = pw->pz;
                        
                        l.vx = -hp->w;
                        l.vy =  hp->h;
                        l.vz =  0;
                        break;
                    }
                    
                    njDistanceP2L((NJS_POINT3*)&pw->px, &l, &pd);
                    
                    pw->py = pd.y;
                    
                    for (j = 0; j < 31; j++) 
                    { 
                        if (((rom->grand[j]) && (pw->py >= rom->grand[j])) || ((j == 2) && (pw->py >= rom->grand[j]))) 
                        {
                            pw->flr_no = j - 2; 
                        } 
                    }
                    
                    if (pw->flr_no != hp->flr_no) 
                    {
                        if (pw->py < rom->grand[pw->flr_no + 2]) 
                        {
                            pw->px = pd.x;
                            pw->pz = pd.z;
                        }
                        else 
                        {
                            pw->py = rom->grand[pw->flr_no + 2];
                        }
                    }
                    else 
                    {
                        pw->px = pd.x;
                        pw->pz = pd.z;
                        
                        if (pw->py < rom->grand[pw->flr_no + 2]) 
                        {
                            pw->py = rom->grand[pw->flr_no + 2];
                        }
                    }
                }
                
                break;
            case 7:                             
                if ((!(hp->attr & 0x1)) || (hp->flr_no == pw->flr_no)) 
                {
                    wpx = hp->px + (0.5f * hp->w);
                    wpz = hp->pz + (0.5f * hp->d);
                    
                    px = hp->px - pw->aw; 
                    pz = hp->pz - pw->ad;  
                    
                    xn = hp->w + (2.0f * pw->aw);
                    zn = hp->d + (2.0f * pw->ad);   
                    
                    if (((((pw->px - px) >= 0) && ((pw->px - px) < xn)) && (((pw->pz - pz) >= 0) && ((pw->pz - pz) < zn)) && ((pw->py < (hp->py + hp->h)) && ((pw->py + pw->ah) > (hp->py + hp->h))) && ((pw->flg & 0x100))) || ((((pw->px - px) >= 0) && ((pw->px - px) < xn)) && (((pw->pz - pz) >= 0) && ((pw->pz - pz) < zn)) && ((((pw->py + pw->ah) >= (hp->py + hp->h)) && (pw->py < hp->py)) && (!(pw->flg & 0x100))))) 
                    {
                        hit++;
                        
                        if (pw->px < wpx) 
                        {
                            if (pw->pz < wpz) 
                            {
                                if (fabsf(pw->px - px) > fabsf(pw->pz - pz))
                                {
                                    pw->pz = pz;
                                } 
                                else 
                                {
                                    pw->px = px;
                                }
                            } 
                            else 
                            {
                                if (fabsf(pw->px - px) > fabsf(pw->pz - (pz + zn)))
                                {
                                    pw->pz = pz + zn;
                                } 
                                else 
                                {
                                    pw->px = px;
                                }
                            }
                        }
                        else 
                        {
                            if (pw->pz < wpz) 
                            {
                                if (fabsf(pw->px - (px + xn)) > fabsf(pw->pz - pz)) 
                                {
                                    pw->pz = pz;
                                } 
                                else 
                                {
                                    pw->px = px + xn;
                                }
                            } 
                            else 
                            {
                                if (fabsf(pw->px - (px + xn)) > fabsf(pw->pz - (pz + zn))) 
                                {
                                    pw->pz = pz + zn;
                                } 
                                else 
                                {
                                    pw->px = px + xn;
                                }
                            }
                        }
                    }
                }
                
                break;
            }
        }
    }
    
    if (hit != 0) 
    {
        pw->stflg |= 0x1;
    }
    
    if ((hit == 0) && ((pw->flg & 0x100))) 
    {
        pw->py = rom->grand[pw->flr_no + 2]; 
    }
    
    if (hit == 0)
    {
        pw->psh_ct = 0;
    }
}

// 100% matching!
ATR_WORK* bhCheckWallType(NJS_POINT3* pos, unsigned int flg, float ar, float ah)
{
    ATR_WORK* hp; 
    int i;      
    int wal_n;   
    float px, pz;   
    float xn, zn;    
    float ln;  
    float h;    

    wal_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < wal_n; i++) 
    {
        if (i < rom->wal_n) 
        {
            hp = &rom->walp[i];
        } 
        else 
        {
            hp = &sys->mwalp[i - rom->wal_n];
        }
        
        if ((hp->flg & 0x1)) 
        {
            if (hp->h)
            {
                h = hp->h;
            } 
            else 
            {
                h = rom->h;
            }
            
            switch (hp->type) 
            {
            case 0:
            case 1:
                if ((!(hp->type & 0x1)) || (!(flg & 0x400))) 
                {
                    pz = hp->pz - ar;
                    
                    xn = hp->w + (2.0f * ar);
                    zn = hp->d + (2.0f * ar);
                    
                    px = pos->x - (hp->px - ar);
                    
                    if (((px >= 0) && (px < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h)))) 
                    {
                        return hp;
                    }
                }
                
                break;
            case 2:
            case 3:
                if ((!(hp->type & 0x1)) || (!(flg & 0x400))) 
                {
                    px = pos->x - hp->px;
                    pz = pos->z - hp->pz;
                    
                    ln = njSqrt((px * px) + (pz * pz));
                    
                    if ((ln < (ar + hp->w)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h)))) 
                    {
                        return hp;
                    }
                }
                
                break;
            case 4:
            case 5:
                if (((!(hp->type & 0x1)) || (!(flg & 0x400))) && (bhCheckInnerTriangle(hp, pos, ar, ah) != 0)) 
                {
                    return hp;
                }
                
                break;
            case 6:
                if ((((pos->x - hp->px) >= 0) && ((pos->x - hp->px) < hp->w)) && (((pos->z - hp->pz) >= 0) && ((pos->z - hp->pz) < hp->d)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))  
                {
                    return hp;
                }
                
                break;
            case 7:
                pz = hp->pz - ar;
                
                xn = hp->w + (2.0f * ar);
                zn = hp->d + (2.0f * ar);
                
                px = hp->px - ar; 
                
                if ((((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && ((pos->y < (hp->py + hp->h)) && ((pos->y + ah) > (hp->py + hp->h)))) && ((flg & 0x100))) || (((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= (hp->py + hp->h)) && (pos->y < hp->py))) && (!(flg & 0x100)))) 
                {
                    return hp;
                }
                
                break;
            }
        } 
    } 
    
    return NULL;
}

// 100% matching!
ATR_WORK* bhCheckWallType2(NJS_POINT3* pos, unsigned int flg, float aw, float ad, float ah, int idx_ct)
{
    NJS_SPHERE sph; 
    NJS_BOX box;    
    ATR_WORK* hp; 
    int i;       
    int wal_n;    
    float px, pz;    
    float xn, zn;      
    float h;       

    wal_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < wal_n; i++) 
    {
        if (i < rom->wal_n) 
        {
            hp = &rom->walp[i];
        } 
        else 
        {
            hp = &sys->mwalp[i - rom->wal_n];
        }
        
        if (((hp->flg & 0x1)) && ((!(hp->attr & 0x40)) || (!(flg & 0x2000))))
        {
            if (hp->h)
            {
                h = hp->h;
            } 
            else 
            {
                h = rom->h;
            }
            
            switch (hp->type) 
            {
            case 0:
            case 1:
                if (((!(hp->type & 0x1)) || (!(flg & 0x400))) && ((!(hp->attr & 0x10000)) || (!(flg & 0x2000)) || (hp->prm3 != (unsigned char)idx_ct))) 
                {
                    pz = hp->pz - ad;
                    
                    xn = hp->w + (2.0f * aw);
                    zn = hp->d + (2.0f * ad);
                    
                    px = pos->x - (hp->px - aw);
                    
                    if (((px >= 0) && (px < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h)))) 
                    {
                        return hp;
                    } 
                }
                
                break;
            case 2:
            case 3:
                if ((!(hp->type & 0x1)) || (!(flg & 0x400))) 
                {
                    px = (hp->px - aw) - hp->w;
                    pz = (hp->pz - ad) - hp->w;
                    
                    xn = (2.0f * hp->w) + (2.0f * aw);
                    zn = (2.0f * hp->w) + (2.0f * ad);
                    
                    if ((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))  
                    {
                        sph.c.x = hp->px; 
                        sph.c.y = hp->py;
                        sph.c.z = hp->pz;
                        sph.r   = hp->w;
                        
                        box.v[0].y = box.v[1].y = box.v[2].y = box.v[3].y = 0.1f;
                        box.v[4].y = box.v[5].y = box.v[6].y = box.v[7].y = 0;
                        
                        box.v[0].x = box.v[4].x = pos->x - aw; 
                        box.v[0].z = box.v[4].z = pos->z + ad;
                        
                        box.v[1].x = box.v[5].x = pos->x - aw;
                        box.v[1].z = box.v[5].z = pos->z - ad;
                        
                        box.v[2].x = box.v[6].x = pos->x + aw;
                        box.v[2].z = box.v[6].z = pos->z - ad;
                        
                        box.v[3].x = box.v[7].x = pos->x + aw;
                        box.v[3].z = box.v[7].z = pos->z + ad;
                            
                        if (njCollisionCheckBS(&box, &sph) != 0) 
                        {
                            return hp;
                        }
                    }
                }
                
                break;
            case 4:
            case 5:
                if (((!(hp->type & 0x1)) || (!(flg & 0x400))) && (bhCheckInnerTriangle2(hp, pos, aw, ad, ah) != 0)) 
                {
                    return hp;
                }
                
                break;
            case 6:
                if ((((pos->x - hp->px) >= 0) && ((pos->x - hp->px) < hp->w)) && (((pos->z - hp->pz) >= 0) && ((pos->z - hp->pz) < hp->d)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))  
                {
                    return hp;
                }
                
                break;
            case 7:
                pz = hp->pz - ad;
                
                xn = hp->w + (2.0f * aw);
                zn = hp->d + (2.0f * ad);
                
                px = hp->px - aw; 
                
                if ((((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && ((pos->y < (hp->py + hp->h)) && ((pos->y + ah) > (hp->py + hp->h)))) && ((flg & 0x100))) || (((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= (hp->py + hp->h)) && (pos->y < hp->py))) && (!(flg & 0x100)))) 
                {
                    return hp;
                }
                
                break; 
            }
        } 
    } 
    
    return NULL;
}

// 99.95% matching
ATR_WORK* bhCheckWallRefAngle(NJS_POINT3* pos, unsigned int flg, float ar, float ah, int* ay)
{
    NJS_POINT3 pd; 
    NJS_LINE l;    
    ATR_WORK* hp; 
    ATR_WORK ht;  
    int i;         
    int wal_n;    
    float px, py, pz;    
    float xn, zn;    
    float ln, ln2;     
    float wpx, wpz;    
    float h;     
    float abx, aby, abz;    
    int r;         

    wal_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < wal_n; i++) 
    {
        if (i < rom->wal_n) 
        {
            hp = &rom->walp[i];
        }
        else
        {
            hp = &sys->mwalp[i - rom->wal_n];
        }
        
        if ((hp->flg & 0x1))
        {
            if (hp->h)
            {
                h = hp->h;
            }
            else
            {
                h = rom->h;
            }
            
            switch (hp->type)
            {                      
            case 0:                                 
            case 1:                                 
                if (((!(hp->type & 0x1)) || (!(flg & 0x400))) && ((!(hp->attr & 0x20)) || (!(flg & 0x200))))  
                {
                    wpx = hp->px + (0.5f * hp->w);
                    wpz = hp->pz + (0.5f * hp->d);
                    
                    px = hp->px - ar;
                    pz = hp->pz - ar;
                    
                    xn = hp->w + (2.0f * ar);
                    zn = hp->d + (2.0f * ar);
                    
                    if ((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))
                    {
                        aby = fabsf(pos->y - (hp->py + h));
                        
                        if (pos->x < wpx) 
                        {
                            if (pos->z < wpz)
                            {
                                abx = fabsf(pos->x - px);
                                abz = fabsf(pos->z - pz);
                                
                                if (abx > abz) 
                                {
                                    if (aby < abz)
                                    {
                                        pos->y = hp->py + h;
                                    }
                                    else 
                                    {
                                        pos->z = pz;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx)
                                {
                                    pos->y = hp->py + h;
                                } 
                                else
                                {
                                    pos->x = px;
                                    
                                    *ay = -*ay;
                                }
                            } 
                            else
                            {
                                abx = fabsf(pos->x - px);
                                abz = fabsf(pos->z - (pz + zn));
                                
                                if (abx > abz) 
                                {
                                    if (aby < abz) 
                                    {
                                        pos->y = hp->py + h;
                                    } 
                                    else 
                                    {
                                        pos->z = pz + zn;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx) 
                                {
                                    pos->y = hp->py + h;
                                } 
                                else
                                {
                                    pos->x = px;
                                    
                                    *ay = -*ay;
                                }
                            }
                        } 
                        else 
                        {
                            if (pos->z < wpz) 
                            {
                                abx = fabsf(pos->x - (px + xn));
                                abz = fabsf(pos->z - pz);
                                
                                if (abx > abz) 
                                {
                                    if (aby < abz) 
                                    {
                                        pos->y = hp->py + h;
                                    }
                                    else
                                    {
                                        pos->z = pz;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                }
                                else if (aby < abx)
                                {
                                    pos->y = hp->py + h;
                                } 
                                else 
                                {
                                    pos->x = px + xn;
                                    
                                    *ay = -*ay;
                                }
                            } 
                            else 
                            {
                                abx = fabsf(pos->x - (px + xn));
                                abz = fabsf(pos->z - (pz + zn));
                                
                                if (abx > abz) 
                                {
                                    if (aby < abz) 
                                    {
                                        pos->y = hp->py + h;
                                    }
                                    else
                                    {
                                        pos->z = pz + zn;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                }
                                else if (aby < abx)
                                {
                                    pos->y = hp->py + h;
                                } 
                                else 
                                {
                                    pos->x = px + xn;
                                    
                                    *ay = -*ay;
                                }
                            }
                        }
                        
                        return hp;
                    }
                }
                
                break;
            case 2:                                 
            case 3:                                 
                if (((!(hp->type & 0x1)) || (!(flg & 0x400))) && ((!(hp->attr & 0x20)) || (!(flg & 0x200)))) 
                {
                    px = hp->px - pos->x;
                    pz = hp->pz - pos->z;
                    
                    ln = njSqrt((px * px) + (pz * pz));
                    
                    if ((ln < (ar + hp->w)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))
                    {
                        r = 10430.381f * atan2f(px, pz); 
                        
                        ln2 = ar + hp->w;
                        
                        aby = fabsf(pos->y - (hp->py + h));
                        abx = fabsf(ln2    - ln); 
                        
                        if (abx < aby) 
                        {
                            njSinCos(r, &pos->x, &pos->z);
                            
                            pos->x = hp->px - (pos->x * ln2);
                            pos->z = hp->pz - (pos->z * ln2);
                            
                            *ay = r + ((r + 32768) - *ay);
                        }
                        else 
                        {
                            pos->y = hp->py + h;
                        }
                        
                        return hp;
                    }
                }
                
                break;
            case 4:                                 
            case 5:                                 
                if (((!(hp->type & 0x1)) || (!(flg & 0x400))) && ((!(hp->attr & 0x20)) || (!(flg & 0x200)))) 
                {
                    if (bhCheckInnerTriangle(hp, pos, ar, ah) != 0) 
                    {
                        l.vy = 0;
                        l.py = 0; 
                        
                        switch (hp->id) 
                        {          
                        case 0:                     
                            px = hp->px + ar;
                            pz = hp->pz + ar;
                            break;
                        case 1:                     
                            px = hp->px + ar;
                            pz = hp->pz - ar;
                            break;
                        case 2:                     
                            px = hp->px - ar;
                            pz = hp->pz + ar;
                            break;
                        case 3:                     
                            px = hp->px - ar;
                            pz = hp->pz - ar;
                            break;
                        }
                        
                        l.px = px;
                        l.pz = pz + hp->d;
                        
                        l.vx =  hp->w;
                        l.vz = -hp->d;
                        
                        njDistanceP2L(pos, &l, &pd);
                        
                        px = pd.x - pos->x; 
                        pz = pd.z - pos->z;
                        
                        ln2 = njSqrt((px * px) + (pz * pz));
                        
                        aby = fabsf(pos->y - (hp->py + h));
                        
                        if (aby >= ln2) 
                        {
                            pos->x = pd.x;
                            pos->z = pd.z;
                            
                            r = 10430.381f * atan2f(hp->w, hp->d);
                            
                            *ay = r + ((r + 32768) - *ay);
                        } 
                        else 
                        {
                            pos->y = hp->py + h;
                        }
                        
                        return hp;
                    }
                    
                    switch (hp->id) 
                    {            
                    case 0:                         
                        ht.px = hp->px;
                        ht.py = hp->py;
                        ht.pz = hp->pz;
                        
                        ht.w = 0;
                        ht.h = hp->h;
                        ht.d = hp->d;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 12) != 0) 
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        ht.w = hp->w;
                        ht.h = hp->h;
                        ht.d = 0;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 10) != 0)
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        break;
                    case 1:                         
                        ht.px = hp->px;
                        ht.py = hp->py;
                        ht.pz = hp->pz + hp->d;
                        
                        ht.w =  0;
                        ht.h =  hp->h;
                        ht.d = -hp->d;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 12) != 0) 
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        r = 0;
                        
                        ht.pz = hp->pz - r;
                        
                        ht.w = hp->w;
                        ht.h = hp->h;
                        ht.d = 0;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 5) != 0) 
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        break;
                    case 2:          
                        r = 0;
                        
                        ht.px = hp->px - r;
                        ht.py = hp->py;
                        ht.pz = hp->pz;
                        
                        ht.w = 0;
                        ht.h = hp->h;
                        ht.d = hp->d;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 3) != 0)
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        ht.px = hp->px + hp->w;
                        
                        ht.w = -hp->w;
                        ht.h =  hp->h;
                        ht.d =  0;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 10) != 0)
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        break;
                    case 3:                   
                        r = 0;
                        
                        ht.px = hp->px - r;
                        ht.py = hp->py;
                        ht.pz = hp->pz + hp->d;
                        
                        ht.w =  0;
                        ht.h =  hp->h;
                        ht.d = -hp->d;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 3) != 0)
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        ht.px = hp->px + hp->w;
                        ht.pz = hp->pz - r;
                        
                        ht.w = -hp->w;
                        ht.h =  hp->h;
                        ht.d =  0;
                        
                        if (bhCheckBox(&ht, pos, ar, ah, 5) != 0)
                        {
                            *ay = -*ay;
                            
                            return hp;
                        }
                        
                        break;
                    }
                }
                
                break;
            case 6:                                 
                if ((((pos->x - hp->px) >= 0) && ((pos->x - hp->px) < hp->w)) && (((pos->z - hp->pz) >= 0) && ((pos->z - hp->pz) < hp->d)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))
                {
                    // this switch is identical to the one found on bhSetWallRefAngle, however here registers won't match
                    switch (hp->id) 
                    { 
                    case 0:         
                        py = hp->py + ((hp->h / hp->w) * (pos->x - hp->px));
                        break;
                    case 1:         
                        py = hp->h + (hp->py + ((hp->h / hp->d) * (hp->pz - pos->z)));
                        break;
                    case 2:         
                        py = hp->py + ((hp->h / hp->d) * (pos->z - hp->pz));
                        break;
                    case 3:         
                        py = hp->h + (hp->py + ((hp->h / hp->w) * (hp->px - pos->x)));
                        break;
                    }
                    
                    if (pos->y < py) 
                    {
                        aby = fabsf(pos->y - py);
                        
                        wpx = hp->px + (0.5f * hp->w);
                        wpz = hp->pz + (0.5f * hp->d);
                        
                        px = hp->px - ar;
                        pz = hp->pz - ar;
                        
                        xn = hp->w + (2.0f * ar);
                        zn = hp->d + (2.0f * ar);
                        
                        if (pos->x < wpx) 
                        {
                            if (pos->z < wpz) 
                            {
                                abx = fabsf(pos->x - px);
                                abz = fabsf(pos->z - pz);
                                
                                if (abx > abz) 
                                {
                                    if (aby < abz) 
                                    {
                                        pos->y = py;
                                    } 
                                    else 
                                    {
                                        pos->z = pz;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx) 
                                {
                                    pos->y = py;
                                } 
                                else 
                                {
                                    pos->x = px;
                                    
                                    *ay = -*ay;
                                }
                            } 
                            else 
                            {
                                abx = fabsf(pos->x - px);
                                abz = fabsf(pos->z - (pz + zn));
                                
                                if (abx > abz)
                                {
                                    if (aby < abz)
                                    {
                                        pos->y = py;
                                    } 
                                    else 
                                    {
                                        pos->z = pz + zn;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx) 
                                {
                                    pos->y = py;
                                } 
                                else 
                                {
                                    pos->x = px;
                                    
                                    *ay = -*ay;
                                }
                            }
                        }
                        else if (pos->z < wpz) 
                        {
                            abx = fabsf(pos->x - (px + xn));
                            abz = fabsf(pos->z - pz);
                            
                            if (abx > abz) 
                            {
                                if (aby < abz) 
                                {
                                    pos->y = py;
                                } 
                                else 
                                {
                                    pos->z = pz;
                                    
                                    *ay = 32768 - *ay;
                                }
                            } 
                            else if (aby < abx) 
                            {
                                pos->y = py;
                            } 
                            else 
                            {
                                pos->x = px + xn;
                                
                                *ay = -*ay;
                            }
                        } 
                        else 
                        {
                            abx = fabsf(pos->x - (px + xn));
                            abz = fabsf(pos->z - (pz + zn));
                            
                            if (abx > abz) 
                            {
                                if (aby < abz) 
                                {
                                    pos->y = py;
                                }
                                else 
                                {
                                    pos->z = pz + zn;
                                    
                                    *ay = 32768 - *ay;
                                }
                            } 
                            else if (aby < abx) 
                            {
                                pos->y = py;
                            } 
                            else 
                            {
                                pos->x = px + xn;
                                
                                *ay = -*ay;
                            }
                        }
                        
                        return hp;
                    }
                    
                    break;
                }
                
                break;
            case 7:                                 
                if ((!(hp->attr & 0x20)) || (!(flg & 0x200))) 
                {
                    wpx = hp->px + (0.5f * hp->w);
                    wpz = hp->pz + (0.5f * hp->d);
                    
                    px = hp->px - ar;
                    pz = hp->pz - ar;
                    
                    xn = hp->w + (2.0f * ar);
                    zn = hp->d + (2.0f * ar);
                    
                    if (((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && ((pos->y < (hp->py + hp->h)) && ((pos->y + ah) > (hp->py + hp->h))) && ((flg & 0x100))) || ((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= (hp->py + hp->h)) && (pos->y < hp->py)) && (!(flg & 0x100)))) 
                    {
                        aby = fabsf(pos->y - hp->py);
                        
                        if (pos->x < wpx)
                        {
                            if (pos->z < wpz) 
                            {
                                abx = fabsf(pos->x - px);
                                abz = fabsf(pos->z - pz);
                                
                                if (abx > abz) 
                                {
                                    if (aby < abz) 
                                    {
                                        pos->y = hp->py;
                                    } 
                                    else 
                                    {
                                        pos->z = pz;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx) 
                                {
                                    pos->y = hp->py;
                                } 
                                else 
                                {
                                    pos->x = px;
                                    
                                    *ay = -*ay;
                                }
                            }
                            else 
                            {
                                abx = fabsf(pos->x - px);
                                abz = fabsf(pos->z - (pz + zn));
                                
                                if (abx > abz)
                                {
                                    if (aby < abz) 
                                    {
                                        pos->y = hp->py;
                                    } 
                                    else 
                                    {
                                        pos->z = pz + zn;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx) 
                                {
                                    pos->y = hp->py;
                                } 
                                else
                                {
                                    pos->x = px;
                                    
                                    *ay = -*ay;
                                }
                            }
                        } 
                        else
                        {
                            if (pos->z < wpz) 
                            {
                                abx = fabsf(pos->x - (px + xn));
                                abz = fabsf(pos->z - pz);
                                
                                if (abx > abz)
                                {
                                    if (aby < abz)
                                    {
                                        pos->y = hp->py;
                                    }
                                    else 
                                    {
                                        pos->z = pz;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx) 
                                {
                                    pos->y = hp->py;
                                }
                                else 
                                {
                                    pos->x = px + xn;
                                    
                                    *ay = -*ay;
                                }
                            } 
                            else 
                            {
                                abx = fabsf(pos->x - (px + xn));
                                abz = fabsf(pos->z - (pz + zn));
                                
                                if (abx > abz) 
                                {
                                    if (aby < abz) 
                                    {
                                        pos->y = hp->py;
                                    }
                                    else 
                                    {
                                        pos->z = pz + zn;
                                        
                                        *ay = 32768 - *ay;
                                    }
                                } 
                                else if (aby < abx) 
                                {
                                    pos->y = hp->py;
                                }
                                else 
                                {
                                    pos->x = px + xn;
                                    
                                    *ay = -*ay;
                                }
                            }
                        }
                        
                        return hp;
                    }
                }
            }
        } 
    } 
    
    return NULL;
}

// 100% matching!
void bhSetWallRefAngle(ATR_WORK* hp, NJS_POINT3* pos, float ar, float ah, int* ay) 
{
    NJS_POINT3 pd;
    NJS_LINE l;
    ATR_WORK ht;
    float px, py, pz;
    float xn, zn;
    float ln, ln2;
    float wpx, wpz;
    float h;
    float abx, aby, abz;
    int r;
    float zero; // not from DWARF
	
    if (!hp->h) 
    { 
        h = rom->h; 
    } 
    else 
    { 
        h = hp->h;
    }
    
    switch (hp->type) 
    {
    case 0:
    case 1:
        wpx = hp->px + (0.5f * hp->w);
        wpz = hp->pz + (0.5f * hp->d);
        
        px = hp->px - ar;
        pz = hp->pz - ar;
        
        xn = hp->w + (2.0f * ar);
        zn = hp->d + (2.0f * ar);
        
        aby = fabsf(pos->y - (hp->py + h));
        
        if (pos->x < wpx) 
        {
            if (pos->z < wpz) 
            {
                abx = fabsf(pos->x - px);
                abz = fabsf(pos->z - pz);
                
                if (abx > abz) 
                {
                    if (aby < abz) 
                    { 
                        pos->y = hp->py + h; 
                    }
                    else 
                    {
                        pos->z = pz;
                        
                        *ay = 32768 - *ay;
                    }
                }
                else if (aby < abx) 
                { 
                    pos->y = hp->py + h; 
                }
                else 
                {
                    pos->x = px;
                    
                    *ay = -*ay;
                }
            } 
            else 
            {
                abx = fabsf(pos->x - px);
                abz = fabsf(pos->z - (pz + zn));
                
                if (abx > abz) 
                {
                    if (aby < abz) 
                    { 
                        pos->y = hp->py + h; 
                    }
                    else 
                    {
                        pos->z = pz + zn;
                        
                        *ay = 32768 - *ay;
                    }
                }
                else if (aby < abx) 
                { 
                    pos->y = hp->py + h; 
                } 
                else
                {
                    pos->x = px;
                    
                    *ay = -*ay;
                }
            }
        }
        else if (pos->z < wpz) 
        {
            abx = fabsf(pos->x - (px + xn));
            abz = fabsf(pos->z - pz);
            
            if (abx > abz) 
            {
                if (aby < abz) 
                {
                    pos->y = hp->py + h; 
                } 
                else 
                {
                    pos->z = pz;
                    
                    *ay = 32768 - *ay;
                }
            } 
            else if (aby < abx) 
            { 
                pos->y = hp->py + h; 
            } 
            else
            {
                pos->x = px + xn;
                
                *ay = -*ay;
            }
        } 
        else 
        {
            abx = fabsf(pos->x - (px + xn));
            abz = fabsf(pos->z - (pz + zn));
            
            if (abx > abz) 
            {
                if (aby < abz)
                { 
                    pos->y = hp->py + h; 
                } 
                else 
                {
                    pos->z = pz + zn;
                    
                    *ay = 32768 - *ay;
                }
            }
            else if (aby < abx) 
            { 
                pos->y = hp->py + h; 
            } 
            else 
            {
                pos->x = px + xn;
                
                *ay = -*ay;
            }
        }
        
        break;
    case 2:
    case 3:
        px = hp->px - pos->x;
        pz = hp->pz - pos->z;
        
        ln = njSqrt((px * px) + (pz * pz));
        
        r = (65536.0f / PI_2) * atan2f(px, pz);
        
        ln2 = ar + hp->w;
        
        py = fabsf(pos->y - (hp->py + h));
        
        if (fabsf(ln2 - ln) < py) 
        {
            njSinCos(r, &pos->x, &pos->z);
            
            pos->x = hp->px - (pos->x * ln2);
            pos->z = hp->pz - (pos->z * ln2);
            
            *ay = r + ((r + (32767 + 1)) - *ay);
        } 
        else 
        {
            pos->y = hp->py + h;
        }
        
        break;
    case 4:
    case 5:
        if (bhCheckInnerTriangle(hp, pos, ar, ah) != 0) 
        {
            l.py = l.vy = 0;
            
            switch (hp->id) 
            {
            case 0:
                px = hp->px + ar;
                pz = hp->pz + ar;
                break;
            case 1:
                px = hp->px + ar;
                pz = hp->pz - ar;
                break;
            case 2:
                px = hp->px - ar;
                pz = hp->pz + ar;
                break;
            case 3:
                px = hp->px - ar;
                pz = hp->pz - ar;
                break;
            }
            
            l.px = px;
            l.pz = pz + hp->d;
            
            l.vx = hp->w;
            l.vz = -hp->d;
            
            njDistanceP2L(pos, &l, &pd);
            
            px = pd.x - pos->x;
            pz = pd.z - pos->z;
            
            abx = njSqrt((px * px) + (pz * pz));
            aby = fabsf(pos->y - (hp->py + h));
            
            if (aby >= abx) 
            {
                pos->x = pd.x;
                pos->z = pd.z;
                
                r = (65536.0f / PI_2) * atan2f(hp->w, hp->d);
                
                *ay = r + ((r + (32767 + 1)) - *ay);
            } 
            else 
            {
                pos->y = hp->py + h;
            }
        } 
        else 
        {
            zero = 0;
            
            switch (hp->id) 
            {
            case 0:
                ht.px = hp->px;
                ht.py = hp->py;
                ht.pz = hp->pz;
                
                ht.w = 0;
                ht.h = hp->h;
                
                ht.d = hp->d;
                
                if (bhCheckBox(&ht, pos, ar, ah, 12) != 0) 
                {
                    *ay = -*ay;
                    break;
                }

                ht.w = hp->w;
                ht.h = hp->h;
                
                ht.d = 0;
                
                if (bhCheckBox(&ht, pos, ar, ah, 10) != 0) 
                {
                    *ay = -*ay;
                }
                
                break;
            case 1:
                ht.px = hp->px;
                ht.py = hp->py;
                ht.pz = hp->pz + hp->d;
                
                ht.w = 0;
                ht.h = hp->h;
                
                ht.d = -hp->d;
                
                if (bhCheckBox(&ht, pos, ar, ah, 12) != 0) 
                {
                    *ay = -*ay;
                    break;
                }
                
                ht.pz = hp->pz - zero;
                
                ht.w = hp->w;
                ht.h = hp->h;
                
                ht.d = 0;
                
                if (bhCheckBox(&ht, pos, ar, ah, 5) != 0) 
                {
                    *ay = -*ay;
                }
                
                break;
            case 2:
                ht.px = hp->px - zero;
                ht.py = hp->py;
                ht.pz = hp->pz;
                
                ht.w = 0;
                ht.h = hp->h;
                
                ht.d = hp->d;
                
                if (bhCheckBox(&ht, pos, ar, ah, 3) != 0) 
                {
                    *ay = -*ay;
                    break;
                }
                
                ht.px = hp->px + hp->w;
                
                ht.w = -hp->w;
                ht.h = hp->h;
                
                ht.d = 0;
                
                if (bhCheckBox(&ht, pos, ar, ah, 10) != 0) 
                {
                    *ay = -*ay;
                }
                
                break;
            case 3:
                ht.px = hp->px - zero;
                ht.py = hp->py;
                ht.pz = hp->pz + hp->d;
                
                ht.w = 0;
                ht.h = hp->h;
                
                ht.d = -hp->d;
                
                if (bhCheckBox(&ht, pos, ar, ah, 3) != 0) 
                {
                    *ay = -*ay;
                    break;
                }
                
                ht.px = hp->px + hp->w;
                ht.pz = hp->pz - zero;
                
                ht.w = -hp->w;
                ht.h = hp->h;
                
                ht.d = 0;
                
                if (bhCheckBox(&ht, pos, ar, ah, 5) != 0) 
                {
                    *ay = -*ay;
                }
            }
        }
        
        break;
    case 6:
        switch (hp->id) 
        {
        case 0:
            py = hp->py + ((hp->h / hp->w) * (pos->x - hp->px));
            break;
        case 1:
            py = hp->h + (hp->py + ((hp->h / hp->d) * (hp->pz - pos->z)));
            break;
        case 2:
            py = hp->py + ((hp->h / hp->d) * (pos->z - hp->pz));
            break;
        case 3:
            py = hp->h + (hp->py + ((hp->h / hp->w) * (hp->px - pos->x)));
            break;
        }
        
        if (pos->y < py) 
        {
            aby = fabsf(pos->y - py);
            
            wpx = hp->px + (0.5f * hp->w);
            wpz = hp->pz + (0.5f * hp->d);
            
            px = hp->px - ar;
            pz = hp->pz - ar;
            
            xn = hp->w + (2.0f * ar);
            zn = hp->d + (2.0f * ar);
            
            if (pos->x < wpx) 
            {
                if (pos->z < wpz) 
                {
                    abx = fabsf(pos->x - px);
                    abz = fabsf(pos->z - pz);
                    
                    if (abx > abz) 
                    {
                        if (aby < abz) 
                        { 
                            pos->y = py; 
                        } 
                        else 
                        {
                            pos->z = pz;
                            
                            *ay = 32768 - *ay;
                        }
                    }
                    else if (aby < abx) 
                    { 
                        pos->y = py; 
                    } 
                    else 
                    {
                        pos->x = px;
                        
                        *ay = -*ay;
                    }
                } 
                else 
                {
                    abx = fabsf(pos->x - px);
                    abz = fabsf(pos->z - (pz + zn));
                    
                    if (abx > abz) 
                    {
                        if (aby < abz) 
                        { 
                            pos->y = py; 
                        } 
                        else 
                        {
                            pos->z = pz + zn;
                            
                            *ay = 32768 - *ay;
                        }
                    }
                    else if (aby < abx) 
                    { 
                        pos->y = py; 
                    } 
                    else 
                    {
                        pos->x = px;
                        
                        *ay = -*ay;
                    }
                }
            }
            else if (pos->z < wpz) 
            {
                abx = fabsf(pos->x - (px + xn));
                abz = fabsf(pos->z - pz);
                
                if (abx > abz) 
                {
                    if (aby < abz) 
                    { 
                        pos->y = py; 
                    } 
                    else 
                    {
                        pos->z = pz;
                        
                        *ay = 32768 - *ay;
                    }
                }
                else if (aby < abx) 
                { 
                    pos->y = py;
                }
                else 
                {
                    pos->x = px + xn;
                    
                    *ay = -*ay;
                }
            } 
            else 
            {
                abx = fabsf(pos->x - (px + xn));
                abz = fabsf(pos->z - (pz + zn));
                
                if (abx > abz)
                {
                    if (aby < abz)
                    { 
                        pos->y = py; 
                    } 
                    else 
                    {
                        pos->z = pz + zn;
                        
                        *ay = 32768 - *ay;
                    }
                }
                else if (aby < abx) 
                { 
                    pos->y = py; 
                }
                else 
                {
                    pos->x = px + xn;
                    
                    *ay = -*ay;
                }
            }
        }
        
        break;
    case 7:
        wpx = hp->px + (0.5f * hp->w);
        wpz = hp->pz + (0.5f * hp->d);
        
        px = hp->px - ar;
        pz = hp->pz - ar;
        
        xn = hp->w + (2.0f * ar);
        zn = hp->d + (2.0f * ar);
        
        aby = fabsf(pos->y - hp->py);
        
        if (pos->x < wpx) 
        {
            if (pos->z < wpz) 
            {
                abx = fabsf(pos->x - px);
                abz = fabsf(pos->z - pz);
                
                if (abx > abz) 
                {
                    if (aby < abz) 
                    {
                        pos->y = hp->py;
                    } 
                    else 
                    {
                        pos->z = pz;
                        
                        *ay = 32768 - *ay;
                    }
                }
                else if (aby < abx) 
                {
                    pos->y = hp->py;
                }
                else 
                {
                    pos->x = px;
                    
                    *ay = -*ay;
                }
            } 
            else 
            {
                abx = fabsf(pos->x - px);
                abz = fabsf(pos->z - (pz + zn));
                
                if (abx > abz) 
                {
                    if (aby < abz) 
                    {
                        pos->y = hp->py;
                    } 
                    else 
                    {
                        pos->z = pz + zn;
                        
                        *ay = 32768 - *ay;
                    }
                }
                else if (aby < abx) 
                {
                    pos->y = hp->py;
                } 
                else 
                {
                    pos->x = px;
                    
                    *ay = -*ay;
                }
            }
        }
        else if (pos->z < wpz) 
        {
            abx = fabsf(pos->x - (px + xn));
            abz = fabsf(pos->z - pz);
            
            if (abx > abz) 
            {
                if (aby < abz) 
                {
                    pos->y = hp->py;
                } 
                else 
                {
                    pos->z = pz;
                    
                    *ay = 32768 - *ay;
                }
            }
            else if (aby < abx) 
            { 
                pos->y = hp->py; 
            }
            else 
            {
                pos->x = px + xn;
                
                *ay = -*ay;
            }
        } 
        else 
        {
            abx = fabsf(pos->x - (px + xn));
            abz = fabsf(pos->z - (pz + zn));
            
            if (abx > abz) 
            {
                if (aby < abz) 
                {
                    pos->y = hp->py;
                } 
                else 
                {
                    pos->z = pz + zn;
                    
                    *ay = 32768 - *ay;
                }
            }
            else if (aby < abx) 
            {
                pos->y = hp->py;
            } 
            else 
            {
                pos->x = px + xn;
                
                *ay = -*ay;
            }
        }
        
        break;
    }
}

// 99.79% matching
float bhGetGroundPosition(NJS_POINT3* pos) 
{
    NJS_POINT3 pd; 
    ATR_WORK* hp;  
    int i;        
    int wal_n;     
    float px, pz;     
    float ln;    
    float h;     
    float yy;     
    float yn, nr;     

    sys->htp = NULL;
    
    yn = -1000.0f;
    nr = 10000.0f;
    
    wal_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < wal_n; i++)
    {
        if (i < rom->wal_n) 
        {
            hp = &rom->walp[i];
        } 
        else
        {
            hp = &sys->mwalp[i - rom->wal_n];
        }
        
        if ((hp->flg & 0x1)) 
        {
            if (hp->h) 
            {
                h = hp->h;
            } 
            else 
            {
                h = rom->h;
            }
            
            switch (hp->type) 
            {                 
            case 0:                             
                yy = pos->y - (hp->py + h);
                
                if ((((pos->x - hp->px) >= 0) && ((pos->x - hp->px) < hp->w)) && (((pos->z - hp->pz) >= 0) && ((pos->z - hp->pz) < hp->d)) && ((yy >= 0) && (yy < nr)))  
                {
                    nr = yy;
                    yn = hp->py + h;
                    
                    sys->htp = hp;
                }
                
                break;
            case 2:                             
                yy = pos->y - (hp->py + h);
                
                px = hp->px - pos->x;
                pz = hp->pz - pos->z;
                
                if ((njSqrt((px * px) + (pz * pz)) < hp->w) && ((yy >= 0) && (yy < nr)))
                {
                    nr = yy;
                    yn = hp->py + h;
                    
                    sys->htp = hp;
                }
                
                break;
            case 4:                             
                yy = pos->y - (hp->py + h);
                
                pd.x = pos->x;
                pd.y = hp->py; 
                pd.z = pos->z;
                
                if ((bhCheckInnerTriangle3(hp, &pd) != 0) && ((yy >= 0) && (yy < nr))) 
                {
                    nr = yy;
                    yn = hp->py + h;
                    
                    sys->htp = hp;
                }
                
                break;
            case 6:                             
                yy = pos->y - hp->py;
                
                if ((((pos->x - hp->px) >= 0) && ((pos->x - hp->px) < hp->w)) && (((pos->z - hp->pz) >= 0) && ((pos->z - hp->pz) < hp->d)) && (yy >= 0)) 
                {
                    switch (hp->id)  
                    { 
                    case 0:         
                        ln = hp->py + ((hp->h / hp->w) * (pos->x - hp->px));
                        break;
                    case 1:         
                        ln = hp->h + (hp->py + ((hp->h / hp->d) * (hp->pz - pos->z)));
                        break;
                    case 2:         
                        ln = hp->py + ((hp->h / hp->d) * (pos->z - hp->pz));
                        break;
                    case 3:         
                        ln = hp->h + (hp->py + ((hp->h / hp->w) * (hp->px - pos->x)));
                        break;
                    }
                    
                    if ((pos->y - ln) < nr) 
                    {
                        nr = pos->y - ln;
                        yn = ln;
                        
                        sys->htp = hp;
                    }
                }
                
                break;
            case 7:                             
                yy = pos->y - hp->py;
                
                if ((yy <= 0) && (yy >= -0.0001f))
                {
                    yy = 0;
                }
                
                if ((((pos->x - hp->px) >= 0) && ((pos->x - hp->px) < hp->w)) && (((pos->z - hp->pz) >= 0) && ((pos->z - hp->pz) < hp->d)) && ((yy >= 0) && (yy < nr))) 
                {
                    nr = yy;
                    yn = hp->py;
                
                    sys->htp = hp;
                }
                
                break;
            }
        }
    }
    
    return yn;
}

// 100% matching!
int bhCheckInnerTriangle(ATR_WORK* hp, NJS_POINT3* pos, float ar, float ah) 
{
    NJS_POINT2 ps, pa, pb, pc, pd;     
    float px, pz;        
    float h, w, d;    // w and d are not from DWARF
    float minx, maxx; // not from DWARF
	float minz, maxz; // not from DWARF
    float abx, abz;   // not from DWARF
  
    if (hp->h) 
    {
        h = hp->h;
    } 
    else 
    {
        h = rom->h;
    }
    
    if ((hp->py > (pos->y + ah)) || ((hp->py + h) < pos->y)) 
    {
        return 0;
    } 
    
    w = hp->w;
    
    if (w > 0) 
    {
        minx = hp->px;
        maxx = ar + (minx + w);
    } 
    else 
    {
        maxx = hp->px;
        minx = (maxx + w) - ar;
    }
    
    d = hp->d;
    
    if (d > 0) 
    { 
        minz = hp->pz;
        maxz = ar + (minz + d);
    } 
    else 
    {
        maxz = hp->pz;
        minz = (maxz + d) - ar;
    }
    
    if (((minx > pos->x) || (maxx <= pos->x)) || ((minz > pos->z) || (maxz <= pos->z)))
    {
    	return 0;
	}
    
    if (!ar) 
    {
        abx = fabsf(d / (w / (pos->x - hp->px)));
        abz = fabsf((hp->pz + hp->d) - pos->z);
        
        if (abz <= abx) 
        {
            return 0;
        }
    }
    else 
    {
        px = hp->px;
        pz = hp->pz;
        
        ps.x = pos->x;
        ps.y = pos->z;
        
        switch (hp->id) 
        {                     
        case 0:
            pa.x = px + hp->w;
            pa.y = pz;
            
            pb.x = pa.x + ar;
            pb.y = pa.y + ar;
            
            pc.x = px;
            pc.y = pz + hp->d;
            
            pd.x = pc.x + ar;
            pd.y = pc.y + ar;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px += ar;
            pz += ar;
            
            if ((px > pos->x) || (pz > pos->z)) 
            {
                return 0;
            }
            
            break;
        case 1:
            pa.x = px + ar;
            pa.y = (pz + hp->d) - ar;
            
            pb.x = pa.x - ar;
            pb.y = pa.y + ar;
            
            pc.x = ar + (px + hp->w);
            pc.y = pz - ar;
            
            pd.x = pc.x - ar;
            pd.y = pc.y + ar;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px += ar;
            pz -= ar;
            
            if ((px > pos->x) || (pz <= pos->z)) 
            {
                return 0;
            }
            
            break;
        case 2:
            pa.x = px + hp->w;
            pa.y = pz;
            
            pb.x = pa.x - ar;
            pb.y = pa.y + ar;
            
            pc.x = px;
            pc.y = pz + hp->d;
            
            pd.x = pc.x - ar;
            pd.y = pc.y + ar;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px -= ar;
            pz += ar;
            
            if ((px <= pos->x) || (pz > pos->z)) 
            {
                return 0;
            }
            
            break;
        case 3:
            pa.x = px - ar;
            pa.y = (pz + hp->d) - ar;
            
            pb.x = pa.x + ar;
            pb.y = pa.y + ar;
            
            pc.x = (px + hp->w) - ar;
            pc.y = pz - ar;
            
            pd.x = pc.x + ar;
            pd.y = pc.y + ar;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px -= ar;
            pz -= ar;
            
            if ((px <= pos->x) || (pz <= pos->z)) 
            {
                return 0;
            }
            
            break;
        }
        
        abx = fabsf(d / (w / (pos->x - px)));
        abz = fabsf((pz + d) - pos->z);
        
        if (abz <= abx) 
        {
            return 0; 
        }
    }
    
    return 1;
}

// 100% matching!
int bhCheckInnerTriangle2(ATR_WORK* hp, NJS_POINT3* pos, float aw, float ad, float ah)
{
    NJS_POINT2 ps, pa, pb, pc, pd;     
    float px, pz;        
    float h, w, d;    // w and d are not from DWARF
    float minx, maxx; // not from DWARF
	float minz, maxz; // not from DWARF
    float abx, abz;   // not from DWARF
  
    if (hp->h) 
    {
        h = hp->h;
    } 
    else 
    {
        h = rom->h;
    }
    
    if ((hp->py > (pos->y + ah)) || ((hp->py + h) < pos->y)) 
    {
        return 0;
    } 
    
    w = hp->w;
    
    if (w > 0) 
    {
        minx = hp->px;
        maxx = aw + (minx + w);
    } 
    else 
    {
        maxx = hp->px;
        minx = (maxx + w) - aw;
    }
    
    d = hp->d;
    
    if (d > 0) 
    { 
        minz = hp->pz;
        maxz = ad + (minz + d);
    } 
    else 
    {
        maxz = hp->pz;
        minz = (maxz + d) - ad;
    }
    
    if (((minx > pos->x) || (maxx <= pos->x)) || ((minz > pos->z) || (maxz <= pos->z)))
    {
    	return 0;
	}
    
    if ((!aw) && (!ad)) 
    {
        abx = fabsf(d / (w / (pos->x - hp->px)));
        abz = fabsf((hp->pz + hp->d) - pos->z);
        
        if (abz <= abx) 
        {
            return 0;
        }
    }
    else 
    {
        px = hp->px;
        pz = hp->pz;
        
        ps.x = pos->x;
        ps.y = pos->z;
        
        switch (hp->id) 
        {                     
        case 0:
            pa.x = px + hp->w;
            pa.y = pz;
            
            pb.x = pa.x + aw;
            pb.y = pa.y + ad;
            
            pc.x = px;
            pc.y = pz + hp->d;
            
            pd.x = pc.x + aw;
            pd.y = pc.y + ad;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px += aw;
            pz += ad;
            
            if ((px > pos->x) || (pz > pos->z)) 
            {
                return 0;
            }
            
            break;
        case 1:
            pa.x = px + aw;
            pa.y = (pz + hp->d) - ad;
            
            pb.x = pa.x - aw;
            pb.y = pa.y + ad;
            
            pc.x = aw + (px + hp->w);
            pc.y = pz - ad;
            
            pd.x = pc.x - aw;
            pd.y = pc.y + ad;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px += aw;
            pz -= ad;
            
            if ((px > pos->x) || (pz <= pos->z)) 
            {
                return 0;
            }
            
            break;
        case 2:
            pa.x = px + hp->w;
            pa.y = pz;
            
            pb.x = pa.x - aw;
            pb.y = pa.y + ad;
            
            pc.x = px;
            pc.y = pz + hp->d;
            
            pd.x = pc.x - aw;
            pd.y = pc.y + ad;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px -= aw;
            pz += ad;
            
            if ((px <= pos->x) || (pz > pos->z)) 
            {
                return 0;
            }
            
            break;
        case 3:
            pa.x = px - aw;
            pa.y = (pz + hp->d) - ad;
            
            pb.x = pa.x + aw;
            pb.y = pa.y + ad;
            
            pc.x = (px + hp->w) - aw;
            pc.y = pz - ad;
            
            pd.x = pc.x + aw;
            pd.y = pc.y + ad;
            
            if (bhCheckInnerP4(&ps, &pa, &pb, &pc, &pd) != 0) 
            {
                return 1;
            }
            
            px -= aw;
            pz -= ad;
            
            if ((px <= pos->x) || (pz <= pos->z)) 
            {
                return 0;
            }
            
            break;
        }
    
        abx = fabsf(d / (w / (pos->x - px)));
        abz = fabsf((pz + d) - pos->z);
        
        if (abz <= abx) 
        {
            return 0; 
        }
    }
    
    return 1;
}

// 100% matching!
int bhCheckInnerTriangle3(ATR_WORK* hp, NJS_POINT3* pos)
{
	float h;
	float min_x, max_x; // not from DWARF
	float min_z, max_z; // not from DWARF
	float abx; // not from DWARF
	float abz; // not from DWARF

	h = hp->h ? hp->h : rom->h;

	if (hp->py > pos->y || (hp->py + h) < pos->y) {
    	return 0;
	}

	if (hp->w > 0.0f) {
		max_x = hp->px + hp->w;
		min_x = hp->px;
	}
	else {
		min_x = hp->px + hp->w;
    	max_x = hp->px;
	}

	if (hp->d > 0.0f) {
		min_z = hp->pz;
		max_z = hp->pz + hp->d;
	}
	else {
		max_z = hp->pz;
		min_z = hp->pz + hp->d;
	}

	if (min_x > pos->x || max_x <= pos->x || min_z > pos->z || max_z <= pos->z) {
    	return 0;
	}

	abx = fabsf(hp->d / (hp->w / (pos->x - hp->px)));
	abz = fabsf((hp->pz + hp->d) - pos->z);
	if (abz <= abx) {
    	return 0;
	}

	return 1;
}

// 100% matching!
int bhCheckBox(ATR_WORK* hp, NJS_POINT3* pos, float ar, float ah, unsigned int aflg)
{
    float px, pz;  
    float xn, zn;  
    float ln;  
    float wpx, wpz; 
    float h;  
    float abx, abz; 
    int r;    
    
    if (hp->h) 
    {
        h = hp->h;
    } 
    else
    {
        h = rom->h;
    }
    
    wpx = hp->px + (0.5f * hp->w);
    wpz = hp->pz + (0.5f * hp->d);
    
    px = hp->px - ar;
    pz = hp->pz - ar;
    
    xn = hp->w + (2.0f * ar);
    zn = hp->d + (2.0f * ar); 
    
    if ((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))  
    {
        if (pos->x < wpx)
        {
            if (pos->z < wpz) 
            {
                abx = fabsf(pos->x - px);
                abz = fabsf(pos->z - pz);
                
                if (((abx < ar) && (abz < ar)) && (!(aflg & 0x1))) 
                {
                    px = hp->px - pos->x;
                    pz = hp->pz - pos->z;
                    
                    ln = njSqrt((px * px) + (pz * pz));
                    
                    if (ln <= ar) 
                    {
                        r = 10430.381f * atan2f(px, pz);
                        
                        njSinCos(r, &pos->x, &pos->z);
                        
                        pos->x = hp->px - (pos->x * ar); 
                        pos->z = hp->pz - (pos->z * ar);
                        
                        return 1;
                    }
                    
                    return 0;
                }
                
                if (abx > abz) 
                {
                    pos->z = pz;
                } 
                else
                {
                    pos->x = px;
                }
                
                return 1;
            }
            
            abx = fabsf(pos->x - px);
            abz = fabsf(pos->z - (pz + zn));
            
            if (((abx < ar) && (abz < ar)) && (!(aflg & 0x2))) 
            {
                px = hp->px - pos->x;
                pz = (hp->pz + hp->d) - pos->z;
                
                ln = njSqrt((px * px) + (pz * pz));
                
                if (ln <= ar) 
                {
                    r = 10430.381f * atan2f(px, pz);
                    
                    njSinCos(r, &pos->x, &pos->z);
                    
                    pos->x = hp->px - (pos->x * ar);
                    pos->z = (hp->pz + hp->d) - (pos->z * ar);
                    
                    return 1;
                }
                
                return 0;
            }
            
            if (abx > abz) 
            {
                pos->z = pz + zn;
            } 
            else 
            {
                pos->x = px;
            }
            
            return 1;
        }
        
        if (pos->z < wpz)
        {
            abx = fabsf(pos->x - (px + xn));
            abz = fabsf(pos->z - pz);
            
            if (((abx < ar) && (abz < ar)) && (!(aflg & 0x4))) 
            {
                px = (hp->px + hp->w) - pos->x;
                pz = hp->pz - pos->z;
                
                ln = njSqrt((px * px) + (pz * pz));
                
                if (ln <= ar)
                {
                    r = 10430.381f * atan2f(px, pz);
                    
                    njSinCos(r, &pos->x, &pos->z);
                    
                    pos->x = (hp->px + hp->w) - (pos->x * ar);
                    pos->z = hp->pz - (pos->z * ar);
                    
                    return 1;
                }
                
                return 0;
            }
            
            if (abx > abz) 
            {
                pos->z = pz;
            }
            else
            {
                pos->x = px + xn;
            }
            
            return 1;
        }
        
        abx = fabsf(pos->x - (px + xn));
        abz = fabsf(pos->z - (pz + zn));
        
        if (((abx < ar) && (abz < ar)) && (!(aflg & 0x8))) 
        {
            px = (hp->px + hp->w) - pos->x;
            pz = (hp->pz + hp->d) - pos->z; 
            
            ln = njSqrt((px * px) + (pz * pz));
            
            if (ln <= ar) 
            {
                r = 10430.381f * atan2f(px, pz);
                
                njSinCos(r, &pos->x, &pos->z);
                
                pos->x = (hp->px + hp->w) - (pos->x * ar); 
                pos->z = (hp->pz + hp->d) - (pos->z * ar); 
                
                return 1; 
            }
            
            return 0;
        }
        
        if (abx > abz) 
        {
            pos->z = pz + zn;
        } 
        else
        {
            pos->x = px + xn;
        }
        
        return 1;
    }
    
    return 0;
}

// 100% matching!
int bhCheckBox2Box(ATR_WORK* hp, NJS_POINT3* pos, float aw, float ad, float ah)
{
    float px, pz;  
    float xn, zn;  
    float wpx, wpz; 
    float h;   
    float abx, abz; 

    if (hp->h)
    {
        h = hp->h;
    } 
    else 
    {
        h = rom->h;
    }
    
    wpx = hp->px + (0.5f * hp->w);
    wpz = hp->pz + (0.5f * hp->d);
    
    px = hp->px - aw;
    pz = hp->pz - ad;
    
    xn = hp->w + (2.0f * aw);
    zn = hp->d + (2.0f * ad);
    
    if ((((pos->x - px) >= 0) && ((pos->x - px) < xn)) && (((pos->z - pz) >= 0) && ((pos->z - pz) < zn)) && (((pos->y + ah) >= hp->py) && (pos->y <= (hp->py + h))))
    {
        if (pos->x < wpx) 
        {
            if (pos->z < wpz)
            {
                abx = fabsf(pos->x - px);
                abz = fabsf(pos->z - pz);
                
                if (abx > abz)
                {
                    pos->z = pz;
                } 
                else 
                {
                    pos->x = px;
                }
                
                return 1;
            }
            
            abx = fabsf(pos->x - px);
            abz = fabsf(pos->z - (pz + zn));
            
            if (abx > abz)
            {
                pos->z = pz + zn;
            }
            else
            {
                pos->x = px;
            }
            
            return 1;
        }
        
        if (pos->z < wpz) 
        {
            abx = fabsf(pos->x - (px + xn));
            abz = fabsf(pos->z - pz);
            
            if (abx > abz)
            {
                pos->z = pz;
            } 
            else
            {
                pos->x = px + xn;
            }
            
            return 1;
        }
        
        abx = fabsf(pos->x - (px + xn));
        abz = fabsf(pos->z - (pz + zn));
        
        if (abx > abz) 
        {
            pos->z = pz + zn;
        } 
        else 
        {
            pos->x = px + xn;
        }
        
        return 1;
    } 
    
    return 0;
}

// 100% matching!
int bhCheckInnerP4(NJS_POINT2* pos, NJS_POINT2* p0, NJS_POINT2* p1, NJS_POINT2* p2, NJS_POINT2* p3) 
{
    float min, max; 
    float swp; 
    float nn;
    float x0, x1;  
    float y0, y1, y2, y3;  

    if ((pos->y < p0->y) || (pos->y >= p3->y)) 
    {
        return 0;
    }

    nn = p1->x - p0->x;
    
    x0 = x1 = 0;
    
    y0 = p1->y - p0->y;
    y1 = p2->y - p0->y;
    y2 = p3->y - p1->y;
    y3 = p3->y - p2->y;

    if (nn > 0)
    {
        if (y1) 
        {
            x0 = (p2->x - p0->x) / y1;
        }
        
        if (y0) 
        {
            x1 = nn / y0;
        }
    }
    else
    {
        if (y0) 
        {
            x0 = nn / y0;
        }
        
        if (y1) 
        {
            x1 = (p2->x - p0->x) / y1;
        }
    }

    if (pos->y < p1->y) 
    {
        min = p0->x + (x0 * (pos->y - p0->y));
        max = p0->x + (x1 * (pos->y - p0->y));
        
        if (min > max) 
        {
            swp = min;
            
            min = max;
            
            max = swp;
        }
        
        if ((min > pos->x) || (max <= pos->x)) 
        {
            return 0;
        }
    }
    else if (pos->y < p2->y) 
    {
        if (nn > 0) 
        {
            x1 = (y2) ? (p3->x - p1->x) / y2 : 0;
            
            min = p0->x + (x0 * (pos->y - p0->y));
            max = p1->x + (x1 * (pos->y - p1->y));
            
            if (min > max) 
            {
                swp = min;
                
                min = max;
                
                max = swp;
            }
            
            if ((min > pos->x) || (max <= pos->x)) 
            {
                return 0;
            }
        }
        else 
        {
            x0 = (y2) ? (p3->x - p1->x) / y2 : 0;
            
            min = p1->x + (x0 * (pos->y - p1->y));
            max = p0->x + (x1 * (pos->y - p0->y));
            
            if (min > max) 
            {
                swp = min;
                
                min = max;
                
                max = swp;
            }
            
            if ((min > pos->x) || (max <= pos->x)) 
            {
                return 0;
            }
        }
    }
    else 
    {
        if (nn > 0) 
        {
            x0 = (y2) ? (p3->x - p1->x) / y2 : 0;
            x1 = (y3) ? (p3->x - p2->x) / y3 : 0;
            
            min = p2->x + (x1 * (pos->y - p2->y));
            max = p1->x + (x0 * (pos->y - p1->y));
            
            if (min > max)
            {
                swp = min;
                
                min = max;
                
                max = swp;
            }
            
            if ((min > pos->x) || (max <= pos->x)) 
            {
                return 0;
            }
        }
        else 
        {
            x0 = (y2) ? (p3->x - p1->x) / y2 : 0;
            x1 = (y3) ? (p3->x - p2->x) / y3 : 0;
            
            min = p1->x + (x0 * (pos->y - p1->y));
            max = p2->x + (x1 * (pos->y - p2->y));
            
            if (min > max) 
            {
                swp = min;
                
                min = max;
                
                max = swp;
            }
            
            if ((min > pos->x) || (max <= pos->x)) 
            {
                return 0;
            }
        }
    }
    
    return 1;
}

// 100% matching!
void bhCheckExmAtari(BH_PWORK* pp)
{
    NJS_POINT3 ps; 
    ATR_WORK* exp; 
    ATR_WORK* ckp; 
    O_WRK* op;   
    float px, pz;      
    float pxx, pzz;    
    float pxn[5], pzn[5];  
    int i;      
    int etc_n;   
    int danf;    
    int ang;     
    
    njSinCos(pp->ay, &px, &pz);
    
    danf = 0;
    
    pxn[0] = ((EXP_WORK*)plp->exp0)->fpx - (px * (1.5f + (3.0f - ((EXP_WORK*)plp->exp0)->arn)));
    pzn[0] = ((EXP_WORK*)plp->exp0)->fpz - (pz * (1.5f + (3.0f - ((EXP_WORK*)plp->exp0)->arn)));
    
    pxn[1] = plp->px - (0.75f * px);
    pzn[1] = plp->pz - (0.75f * pz);
    
    pxn[2] = plp->px - (4.5f * px);
    pzn[2] = plp->pz - (4.5f * pz); 
    
    pxn[3] = ((EXP_WORK*)plp->exp0)->fpx - (px * (3.0f + (3.0f - ((EXP_WORK*)plp->exp0)->arn)));
    pzn[3] = ((EXP_WORK*)plp->exp0)->fpz - (pz * (3.0f + (3.0f - ((EXP_WORK*)plp->exp0)->arn)));
    
    pxn[4] = plp->px - (2.0f * px);
    pzn[4] = plp->pz - (2.0f * pz);
    
    sys->cb_flg &= ~0x100;
    
    etc_n = rom->etc_n + sys->metc_n;
    
    for (i = 0; i < etc_n; i++)
    {
        if (i < rom->etc_n) 
        {
            exp = &rom->etcp[i];
        } 
        else
        {
            exp = &sys->metcp[i - rom->etc_n];
        }
        
        if ((exp->flg & 0x1)) 
        {
            px = pxn[exp->type];
            pz = pzn[exp->type];
            
            if (((exp->px <= px) && ((exp->px + exp->w) >= px)) && ((exp->pz <= pz) && ((exp->pz + exp->d) >= pz)) && (exp->flr_no == pp->flr_no)) 
            {
                sys->cb_flg |= 0x100;
                
                sys->etc_idx = i;
                
                switch (exp->type) 
                {       
                case 0:                                 
                    ang = pp->ay + 8192;
                    
                    if ((((ang & 0xC000) == 0x8000) && ((exp->attr & 0x400))) || (((ang & 0xC000) == 0x4000) && ((exp->attr & 0x800))) || ((!(ang & 0xC000)) && ((exp->attr & 0x1000))) || (((ang & 0xC000) == 0xC000) && ((exp->attr & 0x2000)))) 
                    {
                        sys->cb_flg &= ~0x100;
                        break;
                    }
                    
                    if ((!(sys->gm_flg & 0x1)) && (!(sys->gm_flg & 0x8))) 
                    {
                        pp->stflg |= 0x80000000;
                        
                        sys->cb_flg |= 0x1;
                        sys->st_flg |= 0x4;
                        
                        sys->ddmd = 0;
                        
                        sys->door.flg = (unsigned short)exp->attr; 
                        
                        sys->door.stg_no = exp->prm0;
                        sys->door.rom_no = exp->prm1;
                        sys->door.pos_no = exp->prm2;
                    
                        sys->door.dor_tp = exp->prm3;
                    } 
                    
                    return;
                case 1:                                 
                    if ((!(pp->stflg & 0x10)) && (!(exp->attr & 0x400000))) 
                    {
                        bhSetUseKaidanFlag(pp, exp, i);
                        
                        pp->stflg |= 0x80010010;
                        
                        pp->flg |=  0x10400;
                        pp->flg &= ~0x110;
                        
                        sys->st_flg |= 0x4;
                        
                        sys->pl_htp = exp;
                        
                        if (exp->prm0 == 0) 
                        {
                            pp->mode0 = 1;
                            pp->mode1 = 0;
                            
                            if ((exp->attr & 0x1))
                            {
                                pp->mode2 = 20;
                            }
                            else 
                            {
                                pp->mode2 = 14;
                            }
                            
                            pp->mode3 = 0;
                        } 
                        else
                        {
                            pp->mode0 = 1;
                            pp->mode1 = 0;
                            
                            if ((exp->attr & 0x1))
                            {
                                pp->mode2 = 21;
                            }
                            else
                            {
                                pp->mode2 = 15;
                            }
                            
                            pp->mode3 = 0;
                            
                            pp->flr_no = bhCheckFloorNum(pp->py - (2.0f * exp->prm2));
                        }
                    }
                    
                    return;
                case 2:                                 
                    if ((!(pp->stflg & 0x20)) && (exp->prm0 == 0)) 
                    {
                        pp->ayp = (pp->ay + 8192) & ~0x3FFF;
                        
                        njSinCos(pp->ayp, &pxx, &pzz);
                        
                        pxx = plp->px - (6.0f * pxx);
                        pzz = plp->pz - (6.0f * pzz);
                        
                        if ((((1.0f + exp->px) <= pxx) && (((exp->px + exp->w) - 1.0f) >= pxx)) && (((1.0f + exp->pz) <= pzz) && (((exp->pz + exp->d) - 1.0f) >= pzz)))
                        {
                            njSinCos(pp->ayp, &ps.x, &ps.z), 
                                
                            ps.x = plp->px - (ps.x * (2.0f + pp->ar));
                            ps.y = 9.1f + plp->py;
                            ps.z = plp->pz - (ps.z * (2.0f + pp->ar)); 
                            
                            ckp = bhCheckWallType(&ps, pp->flg, pp->ar, pp->ah); 
                            
                            if ((ckp == NULL) || ((ckp->attr & 0x4))) 
                            {
                                pp->stflg |= 0x80010020;
                                
                                pp->flg |=  0x10400;
                                pp->flg &= ~0x110;
                                
                                sys->st_flg |= 0x4;
                                
                                sys->pl_htp = exp;
                                
                                pp->mode0 = 1;
                                pp->mode1 = 0;
                                pp->mode2 = 16;
                                pp->mode3 = 0;
                                return;
                            }
                        }
                    }
                    else
                    {
                        if ((!(pp->stflg & 0x20)) && (exp->prm0 != 0)) 
                        {
                            danf = 1;
                        }
                        
                        break;
                    }
                    
                    return;
                case 3:                                 
                    ang = pp->ay + 8192;
                    
                    if ((((ang & 0xC000) == 0x8000) && ((exp->attr & 0x400))) || (((ang & 0xC000) == 0x4000) && ((exp->attr & 0x800))) || ((!(ang & 0xC000)) && ((exp->attr & 0x1000))) || (((ang & 0xC000) == 0xC000) && ((exp->attr & 0x2000)))) 
                    {
                        sys->cb_flg &= ~0x100;
                        break;
                    }
                    
                    if (!(sys->st_flg & 0x80))
                    {
                        if ((exp->attr & 0x9))
                        {
                            sys->fixcno = exp->prm0;
                            sys->fixkno = exp->prm2;
                            
                            sys->exm_attr = exp->attr;
                            
                            sys->pad_onb  = sys->pad_on;
                            sys->pad_psb  = sys->pad_ps;
                            sys->pad_oldb = sys->pad_old;
                            
                            sys->pad_ps = 0;
                            
                            sys->gm_flg |= 0x1000;
                            
                            if ((exp->attr & 0x8)) 
                            {
                                sys->gm_flg |=  0x10000;
                            }
                            else 
                            {
                                sys->gm_flg &= ~0x10000;
                            }
                            
                            sys->st_flg |= 0x80;
                            
                            sys->cb_flg |= 0x8;
                            
                            sys->st_flg |= 0x4;
                            
                            if ((cam.flg & 0x20)) 
                            {
                                sys->ef_flg |= 0x10;
                            }
                        }
                        
                        if ((exp->attr & 0x8000)) 
                        {
                            bhSetMessage(0, exp->prm1);
                            
                            if (!(exp->attr & 0x9)) 
                            {
                                sys->pad_onb  = sys->pad_on;
                                sys->pad_psb  = sys->pad_ps;
                                sys->pad_oldb = sys->pad_old;
                                
                                sys->pad_ps = 0;
                            }
                            
                            sys->cb_flg |= 0x20;
                            
                            sys->st_flg |= 0x2000;
                            sys->st_flg |= 0x4;
                            
                        }
                    }
                    
                    return;
                case 4:                                 
                    ang = pp->ay + 8192;
                    
                    if ((((ang & 0xC000) == 0x8000) && ((exp->attr & 0x400))) || (((ang & 0xC000) == 0x4000) && ((exp->attr & 0x800))) || ((!(ang & 0xC000)) && ((exp->attr & 0x1000))) || (((ang & 0xC000) == 0xC000) && ((exp->attr & 0x2000)))) 
                    {
                        sys->cb_flg &= ~0x100;
                        break;
                    }
                    
                    if (!(exp->attr & 0x2)) 
                    {
                        if ((exp->attr & 0x10))
                        {
                            sys->cb_flg |= 0x20000;
                        }
                        
                        op = &sys->itwp[exp->prm0];
                        
                        if ((op->flg & 0x1)) 
                        {
                            sys->sb_id = op->id;
                            
                            if ((exp->attr & 0x1))
                            {
                                pp->flg |= 0x10000;
                                
                                pp->stflg   |= 0x10000;
                                sys->st_flg |= 0x4;
                                
                                pp->mode0 = 1;
                                pp->mode1 = 0;
                                pp->mode2 = 19;
                                pp->mode3 = 0;
                            } 
                            else
                            {
                                sys->st_flg &= ~0x4;
                                
                                sys->cb_flg |= 0x10;
                            }
                            
                            return;
                        }
                    }
                    else if ((exp->attr & 0xC))
                    {
                        if ((exp->attr & 0x4)) 
                        {
                            sys->cb_flg |= 0x80000;
                        } 
                        else 
                        {
                            sys->cb_flg |= 0x100000;
                        }
                    }
                    
                    if (exp->prm0 == 0xFF)
                    {
                        sys->cb_flg |= 0x40000;
                    } 
                    else
                    {
                        op = &sys->obwp[exp->prm0];
                        
                        if ((op->flg & 0x1)) 
                        {
                            op->type = 100;
                            
                            *(int*)&op->mode0 = 0; 
                            
                            pp->flg |= 0x10000;
                            
                            pp->stflg   |= 0x18000;
                            sys->st_flg |= 0x4;
                            
                            pp->mode0 = 1;
                            pp->mode1 = 0;
                            pp->mode2 = 0;
                            pp->mode3 = 0;
                        } 
                    }
                    
                    return;
                }
            }
        } 
    } 
    
    if (((!(pp->stflg & 0x20)) && ((pp->stflg & 0x20000))) && (danf == 0)) 
    {
        pp->ayp = (pp->ay + 8192) & ~0x3FFF;
        
        njSinCos(pp->ayp, &ps.x, &ps.z);
        
        ps.x = plp->px - (ps.x * (3.0f + pp->ar));
        ps.y = plp->py - 8.9f;
        ps.z = plp->pz - (ps.z * (3.0f + pp->ar)); 
        
        ckp = bhCheckWallType(&ps, pp->flg, pp->ar, pp->ah);
        
        if ((ckp == NULL) || ((ckp->attr & 0x4))) 
        {
            pp->stflg |= 0x80010020;
            
            pp->flg |=  0x10400;
            pp->flg &= ~0x110;
            
            sys->st_flg |= 0x4;
            
            pp->mode0 = 1;
            pp->mode1 = 0;
            pp->mode2 = 17;
            pp->mode3 = 0;
        }
    }
}

// 100% matching!
void bhSetUseKaidanFlag(BH_PWORK* pp, ATR_WORK* exp, int idx)
{
    ATR_WORK* exp2;
    
    pp->kdnp   = &exp->flg;
    pp->kdnidx = idx;
    
    exp->attr |= 0x400000;
    
    if (exp->prm3 == 0xFF) 
    {
        if (idx < rom->etc_n)
        {
            exp2 = &rom->etcp[idx];
        } 
        else 
        {
            exp2 = &sys->metcp[idx - rom->etc_n];
        }
        
        if (exp->prm0 == 0) 
        {
            exp2++;
        } 
        else 
        {
            exp2--;
        }
        
        exp2->attr |= 0x400000;
    }
    else 
    {
        exp2 = &rom->etcp[exp->prm3];
        
        exp2->attr |= 0x400000;
    }
}

// 100% matching!
void bhClrUseKaidanFlag(BH_PWORK* pp) 
{
    ATR_WORK* exp, *exp2;
    int idx;
	
    exp = (ATR_WORK*)pp->kdnp;
    
    if (exp != NULL) 
    {
        idx = pp->kdnidx;
        
        exp->attr &= ~0x400000;
        
        if (exp->prm3 == 0xFF) 
        {
            if (idx < rom->etc_n) 
            {
                exp2 = &rom->etcp[idx];
            } 
            else 
            {
                exp2 = &sys->metcp[idx - rom->etc_n];
            }
            
            if (exp->prm0 == 0)
            {
                exp2++;
            } 
            else 
            {
                exp2--;
            }
            
            exp2->attr &= ~0x400000;
        }
        else
        {
            exp = &rom->etcp[exp->prm3];
            
            exp->attr &= ~0x400000;
        }
    }
}

// 99.21% matching
void bhSetDansaLimitAtari(BH_PWORK* pp) 
{
    // modified order of local variables in regards to DWARF
    ATR_WORK* hp;  
    ATR_WORK* exp; 
    ATR_WORK* pop; 
    float px1;      
    float pz2;      
    float px3;      
    float tmp; // not from DWARF 
    float arh;      
    float px0;      
    float pz0;      
    int i;          
    int etc_n;      
    float pz1; // not from DWARF       

    sys->dla_n = 0;
    
    px0 = pp->px;
    pz0 = pp->pz;
    
    arh = pp->ar;
    
    pz1 = pz0 - arh;
    px1 = px0 + arh;
    pz2 = pz0 + arh;
    px3 = px0 - arh;
    
    tmp = 0.5f * arh;
    
    pop = NULL;
    
    etc_n = rom->etc_n + sys->metc_n;
    
    for (i = 0; i < etc_n; i++) 
    {
        if (i < rom->etc_n) 
        {
            exp = &rom->etcp[i];
        } 
        else 
        {
            exp = &sys->metcp[i - rom->etc_n];
        }
        
        if (((exp->flg & 0x1)) && (exp->type == 2) && (exp->prm0 != 0) && (exp->flr_no == pp->flr_no) && ((exp->px <= px0) && ((exp->px + exp->w) >= px0)) && ((exp->pz <= pz0) && ((exp->pz + exp->d) >= pz0))) 
        {
            pop = exp;
            break; 
        } 
    }
    
    if (pop == NULL) 
    {
        pop = pp->dan_ap;
    } 
    else 
    {
        pp->dan_ap = pop;
    }
    
    if (pop == NULL) 
    {
        pp->stflg &= ~0x20000;
        return;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px0, pz1) == NULL) 
    {
        sys->dla_n++;
        
        hp = &sys->mwalp[sys->mwal_n++];
        
        hp->flg  = 1;
        hp->type = 1;
        
        hp->flr_no = pp->flr_no;
        
        hp->attr = 0x800000;
        
        hp->px = pop->px - (2.0f * arh);
        hp->py = pop->py;
        hp->pz = pop->pz - ((2.0f * arh) + tmp);
        
        hp->w = pop->w + (4.0f * arh);
        hp->h = 1.0f;
        hp->d = 2.0f * arh;
        
        hp->prm0 = hp->prm1 = hp->prm2 = hp->prm3 = 0;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px1, pz0) == NULL)
    {
        sys->dla_n++;
        
        hp = &sys->mwalp[sys->mwal_n++];
        
        hp->flg  = 1;
        hp->type = 1;
        
        hp->flr_no = pp->flr_no;
        
        hp->attr = 0x800000;
        
        hp->px = tmp + (pop->px + pop->w);
        hp->py = pop->py;
        hp->pz = pop->pz - (2.0f * arh);
        
        hp->w = 2.0f * arh;
        hp->h = 1.0f;
        hp->d = pop->d + (4.0f * arh);
        
        hp->prm0 = hp->prm1 = hp->prm2 = hp->prm3 = 0;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px0, pz2) == NULL)
    {
        sys->dla_n++;
        
        hp = &sys->mwalp[sys->mwal_n++];
        
        hp->flg  = 1;
        hp->type = 1;
        
        hp->flr_no = pp->flr_no;
        
        hp->attr = 0x800000;
        
        hp->px = pop->px - (2.0f * arh);
        hp->py = pop->py;
        hp->pz = tmp + (pop->pz + pop->d);
        
        hp->w = pop->w + (4.0f * arh);
        hp->h = 1.0f;
        hp->d = 2.0f * arh;
        
        hp->prm0 = hp->prm1 = hp->prm2 = hp->prm3 = 0;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px3, pz0) == NULL) 
    {
        sys->dla_n++;
        
        hp = &sys->mwalp[sys->mwal_n++];
        
        hp->flg  = 1;
        hp->type = 1;
        
        hp->flr_no = pp->flr_no;
        
        hp->attr = 0x800000;
        
        hp->px = pop->px - ((2.0f * arh) + tmp);
        hp->py = pop->py;
        hp->pz = pop->pz - (2.0f * arh);
        
        hp->w = 2.0f * arh;
        hp->h = 1.0f;
        hp->d = pop->d + (4.0f * arh);
        
        hp->prm0 = hp->prm1 = hp->prm2 = hp->prm3 = 0;
    }
}

// 100% matching! 
ATR_WORK* bhCheckDansaAtari(int flr_no, float px, float pz)
{
    ATR_WORK* exp; 
    int i;        
    int etc_n;     
    
    etc_n = rom->etc_n + sys->metc_n;
    
    for (i = 0; i < etc_n; i++) 
    {
        if (i < rom->etc_n) 
        {
            exp = rom->etcp + i;
        } 
        else 
        {
            exp = &sys->metcp[i - rom->etc_n];
        }
        
        if (((exp->flg & 0x1)) && (exp->type == 2) && (exp->prm0 != 0) && (((exp->px <= px) && ((exp->px + exp->w) >= px)) && ((exp->pz <= pz) && ((exp->pz + exp->d) >= pz))) && (exp->flr_no == flr_no)) 
        {
            return exp;
        }
    }
    
    return NULL;
}

// 100% matching!
void bhCheckFloorP(BH_PWORK* pp) 
{
    ATR_WORK* fp; 
    float px, py, pz;    
    float spx, spz;    
    float* dp;   
    int i;        
    int flr_n;   
    int ang;     
  
    px = pp->px;
    py = pp->py;
    pz = pp->pz;
    
    njSinCos(pp->ay, &spx, &spz);
    
    dp = (float*)plp->exp0;
    spx = dp[9] - (spx * (3.0f + (3.0f - dp[7])));
    
    dp = (float*)plp->exp0;
    spz = dp[11] - (spz * (3.0f + (3.0f - dp[7])));
    
    pp->stflg &= ~0x100000;
    pp->flg2  &= ~0x8;
    
    sys->cb_flg &= ~0x8000200;
    
    flr_n = rom->flr_n + sys->mflr_n;
    
    for (i = 0; i < flr_n; i++)
    {
        if (i < rom->flr_n) 
        {
            fp = &rom->flrp[i];
        } 
        else 
        {
            fp = &sys->mflrp[i - rom->flr_n];
        }
        
        if ((fp->flg & 0x1)) 
        {
            if ((fp->type == 0) && ((fp->attr & 0x1))) 
            {
                if ((((fp->px <= spx) && ((fp->px + fp->w) >= spx)) && ((fp->pz <= spz) && ((fp->pz + fp->d) >= spz)) && (fp->flr_no == pp->flr_no)) && ((!(plp->flg & 0x4)) && (!(plp->flg & 0x2)))) 
                {
                    ang = (pp->ay + 8192) & 0xC000; 
                    
                    if ((((ang == 32768) == 0) || (!(fp->attr & 0x400))) && ((ang != 16384) || (!(fp->attr & 0x800))) && ((ang != 0) || (!(fp->attr & 0x1000))) && ((ang != 49152) || (!(fp->attr & 0x2000)))) 
                    {
                        sys->cb_flg |= 0x200;
                        
                        sys->flr_idx = i; 
                    }
                }
            } 
            else 
            {
                if (((fp->px <= px) && ((fp->px + fp->w) >= px)) && ((fp->py <= py) && ((fp->py + fp->h) >= py)) && ((fp->pz <= pz) && ((fp->pz + fp->d) >= pz))) 
                {
                    switch ((unsigned short)fp->type) 
                    { 
                    case 0:         
                        if ((!(plp->flg & 0x4)) && (!(plp->flg & 0x2)))
                        {
                            ang = (pp->ay + 8192) & 0xC000; 
                            
                            if (((ang != 32768) || (!(fp->attr & 0x400))) && ((ang != 16384) || (!(fp->attr & 0x800))) && ((ang != 0) || (!(fp->attr & 0x1000))) && ((ang != 49152) || (!(fp->attr & 0x2000))))
                            {
                                sys->cb_flg |= 0x200;
                                
                                sys->flr_idx = i; 
                            }
                        } 
                        
                        break;
                    case 1:             
                        if ((!(fp->attr & 0x1)) || ((pp->stflg & 0x10)))
                        {
                            pp->flr_snd = fp->prm0;
                        } 
                        
                        break;
                    case 3:        
                        switch (fp->prm0) 
                        { 
                        case 1:         
                            pp->stflg |= 0x100000;
                            break;
                        case 3:         
                            pp->flg2 |= 0x8;
                            
                            pp->footeff = 2;
                            break;
                        case 4:         
                            sys->flr_idx = i;
                            
                            if ((pp->flg & 0x10000000)) 
                            {
                                sys->cb_flg |= 0x8000000;
                            }
                            
                            break;
                        }
                        
                        break;
                    }
                }
            }
        }
    }
    
    pp->stflg &= ~0x20000;
    
    if (bhCheckDansaAtari(pp->flr_no, px, pz) != NULL) 
    {
        pp->stflg |= 0x20000;
        return;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px, pz - pp->ar) != NULL) 
    {
        pp->stflg |= 0x20000;
        return;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px + pp->ar, pz) != NULL) 
    {
        pp->stflg |= 0x20000;
        return;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px, pz + pp->ar) != NULL) 
    {
        pp->stflg |= 0x20000;
        return;
    }
    
    if (bhCheckDansaAtari(pp->flr_no, px - pp->ar, pz) != NULL) 
    {
        pp->stflg |= 0x20000;
        return;
    }
}

// 100% matching!
void bhCheckDansa(BH_PWORK* pp) 
{
    ATR_WORK* exp; 
    float px, pz;     
    float* dp;  
    int i;       
    int etc_n;     
 
    px = pp->px;
    pz = pp->pz;
    
    pp->stflg &= ~0x20000;
    
    etc_n = rom->etc_n + sys->metc_n;
    
    for (i = 0; i < etc_n; i++) 
    {
        if (i < rom->etc_n) 
        {
            exp = rom->etcp + i;
        }
        else
        {
            exp = &sys->metcp[i - rom->etc_n];
        }
        
        if (((exp->flg & 0x1)) && (((exp->px <= px) && ((exp->px + exp->w) >= px)) && ((exp->pz <= pz) && ((exp->pz + exp->d) >= pz))) && (exp->flr_no == pp->flr_no))
        {
            switch (exp->type) 
            {
            case 2:
                if ((!(pp->stflg & 0x30)) && (exp->prm0 != 0)) 
                { 
                    pp->stflg |= 0x20000;  
                }
                
                break;
            }
        }
    } 
}

// 100% matching! 
int bhCheckFloorSound(BH_PWORK* pp, int flr_no, float px, float pz)
{
    ATR_WORK* fp; 
    int i;        
    int flr_n;    
    int sno;     
    
    sno = 0;
    
    flr_n = rom->flr_n + sys->mflr_n;
    
    for (i = 0; i < flr_n; i++) 
    {
        if (i < rom->flr_n) 
        {
            fp = &rom->flrp[i];
        } 
        else 
        {
            fp = &sys->mflrp[i - rom->flr_n]; 
        }
        
        if ((((fp->flg & 0x1)) && (fp->type == 1)) && ((fp->px <= px) && ((fp->px + fp->w) >= px)) && (((fp->pz <= pz) && ((fp->pz + fp->d) >= pz)) && (fp->flr_no == flr_no))) 
        {
            if (((fp->attr & 0x1)) && ((pp->stflg & 0x10))) 
            {
                return fp->prm0;
            }
            
            if (!(fp->attr & 0x1)) 
            {
                sno = fp->prm0;
            }
        }
    }
    
    return sno;
}

// 100% matching!
ATR_WORK* bhCheckFloorEnemy(int flr_no, float px, float pz)
{
    ATR_WORK* fp;
    int i;
    int flr_n;
	
    flr_n = rom->flr_n + sys->mflr_n;
    
    for (i = 0; i < flr_n; i++) 
    {
        if (i < rom->flr_n)
        {
            fp = rom->flrp + i;
        } 
        else 
        {
            fp = &sys->mflrp[i - rom->flr_n];
        }
        
        if ((((fp->flg & 0x1)) && (fp->type == 2)) && (((fp->px <= px) && ((fp->px + fp->w) >= px)) && ((fp->pz <= pz) && ((fp->pz + fp->d) >= pz))) && (fp->flr_no == flr_no)) 
        {
            return fp;
        }
    }
    
    return NULL;
}

// 100% matching!
ATR_WORK* bhCheckFloorEffect(int flr_no, float px, float pz)
{
    ATR_WORK* fp;
    int i;
    int flr_n;
	
    flr_n = rom->flr_n + sys->mflr_n;
    
    for (i = 0; i < flr_n; i++) 
    {
        if (i < rom->flr_n)
        {
            fp = rom->flrp + i;
        } 
        else 
        {
            fp = &sys->mflrp[i - rom->flr_n];
        }
        
        if ((((fp->flg & 0x1)) && (fp->type == 3)) && (((fp->px <= px) && ((fp->px + fp->w) >= px)) && ((fp->pz <= pz) && ((fp->pz + fp->d) >= pz))) && (fp->flr_no == flr_no)) 
        {
            return fp;
        }
    }
    
    return NULL;
}

// 100% matching!
ATR_WORK* bhCheckWater(NJS_POINT3* pos) 
{
    ATR_WORK* fp; 
    int i;        
    int flr_n;   

    flr_n = rom->flr_n + sys->mflr_n;
    
    for (i = 0; i < flr_n; i++) 
    {
        if (i < rom->flr_n)
        {
            fp = rom->flrp + i;
        }
        else 
        {
            fp = &sys->mflrp[i - rom->flr_n];
        }
        
        if ((((fp->flg & 0x1)) && (fp->type == 3)) && (fp->prm0 == 1) && (((fp->px <= pos->x) && ((fp->px + fp->w) >= pos->x)) && ((fp->pz <= pos->z) && ((fp->pz + fp->d) >= pos->z)) && ((fp->py <= pos->y) && ((fp->py + fp->h) >= pos->y)))) 
        {
            return fp;
        }
    }
    
    return NULL;
}

// 100% matching!
ATR_WORK* bhCheckL2Water(NJS_LINE* lp, NJS_POINT3* pos)
{
    ATR_WORK* fp;  
    NJS_LINE pl;  
    NJS_POINT3 ps, pt; 
    NJS_POINT3 ll; 
    float px, pz;     
    float sca;   
    int i;         
    int flr_n;     

    sca = njScalor((NJS_VECTOR*)&lp->vx);
    
    flr_n = rom->flr_n + sys->mflr_n;
    
    for (i = 0; i < flr_n; i++) 
    {
        if (i < rom->flr_n) 
        {
            fp = &rom->flrp[i];
        } 
        else 
        {
            fp = &sys->mflrp[i - rom->flr_n];
        }
        
        if ((((fp->flg & 0x1)) && (fp->type == 3)) && (fp->prm0 == 1)) 
        {
            pl.px = fp->px;
            pl.py = fp->py + fp->h;
            pl.pz = fp->pz;
            
            pl.vx = 0;
            pl.vy = 1.0f;
            pl.vz = 0;
            
            if (!njDistanceL2PL(lp, &pl, &pt)) 
            {
                ps.x = pt.x - lp->px;
                ps.y = pt.y - lp->py;
                ps.z = pt.z - lp->pz;
                
                if ((njInnerProduct((NJS_VECTOR*)&lp->vx, &ps) > 0))
                {
                    px = pt.x - fp->px;
                    pz = pt.z - fp->pz;
                    
                    ll.x = lp->px - pt.x;
                    ll.y = lp->py - pt.y;
                    ll.z = lp->pz - pt.z;
                    
                    if ((((px >= 0) && (px <= fp->w)) && ((pz >= 0) && (pz <= fp->d))) && (njScalor(&ll) <= sca)) 
                    {
                        pos->x = pt.x;
                        pos->y = pt.y;
                        pos->z = pt.z;
                        
                        return fp;
                    }
                }
            }
        }
    }
    
    return NULL;
}

// 100% matching! 
void bhResetAtariAttr()
{
    ATR_WORK* hp; 
    int i;        
    int atr_n;    

    atr_n = rom->wal_n + sys->mwal_n;
    
    for (i = 0; i < atr_n; i++)
    {
        if (i < rom->wal_n)
        {
            hp = &rom->walp[i];
        }
        else
        {
            hp = &sys->mwalp[i - rom->wal_n];
        }
        
        hp->attr &= 0x1FFFF;
        
        if ((hp->flg & 0x2)) 
        {
            hp->flg &= 0xFE;
        }
        
        if (((hp->flg & 0x1)) && ((hp->type <= 1) || (hp->type == 7))) 
        {
            hp->w = 0.1f * ceilf(10.0f * hp->w);
            hp->d = 0.1f * ceilf(10.0f * hp->d);
        }
    }
    
    atr_n = rom->etc_n + sys->metc_n;
    
    for (i = 0; i < atr_n; i++) 
    {
        if (i < rom->etc_n)
        {
            hp = &rom->etcp[i];
        } 
        else 
        {
            hp = &sys->metcp[i - rom->etc_n];
        }
        
        *(int*)&hp->attr = (unsigned short)hp->attr; // ???
        
        if ((hp->flg & 0x2))
        {
            hp->flg &= 0xFE;
        }
        
        hp->w = 0.1f * ceilf(10.0f * hp->w);
        hp->d = 0.1f * ceilf(10.0f * hp->d);
    }
    
    atr_n = rom->flr_n + sys->mflr_n;
    
    for (i = 0; i < atr_n; i++) 
    {
        if (i < rom->flr_n) 
        {
            hp = &rom->flrp[i]; 
        } 
        else 
        {
            hp = &sys->mflrp[i - rom->flr_n];
        }
        
        *(int*)&hp->attr = (unsigned short)hp->attr; // ???
        
        if ((hp->flg & 0x2)) 
        {
            hp->flg &= 0xFE;
        }
        
        hp->w = 0.1f * ceilf(10.0f * hp->w);
        hp->d = 0.1f * ceilf(10.0f * hp->d);
    } 
}

// 100% matching!
void bhCheckPlayer(BH_PWORK* pp)
{
    NJS_VECTOR vec;
    float px, pz;     
    float ey;    
    float ln;       
    float ppx, ppy, ppz;     
    int r;        
    float car; // not from DWARF

    if ((!(pp->flg & 0x8)) || ((pp->stflg & 0x40000000)) || ((pp->flg2 & 0x1)) || (!(pp->flg & 0x40))) 
    {
        return;
    }
    
    if (!(plp->flg & 0x8)) 
    {
        ppx = ((EXP_WORK*)plp->exp0)->nlxb + plp->aox; 
        ppy = ((EXP_WORK*)plp->exp0)->nlyb + plp->aoy;
        ppz = ((EXP_WORK*)plp->exp0)->nlzb + plp->aoz;
    }
    else 
    {
        ppx = plp->px + plp->aox;
        ppy = plp->py + plp->aoy;
        ppz = plp->pz + plp->aoz;
    }

    ey = pp->py + pp->aoy;
    
    px = ppx - (pp->px + pp->aox); 
    pz = ppz - (pp->pz + pp->aoz);
    
    ln = njSqrt((px * px) + (pz * pz)); 
    
    car = plp->car + pp->car;
    
    if ((ln < car) && ((ppy <= (ey + pp->cah)) && ((ppy + plp->cah) >= ey))) 
    {
        if (((plp->flg2 & 0x1)) || ((pp->flg2 & 0x40))) 
        {
            r = 10430.381f * atan2f(px, pz);
            
            car = plp->car + pp->car;
            
            njSinCos(r, &pp->px, &pp->pz);
            
            pp->px = (ppx - (pp->px * car)) - pp->aox;
            pp->pz = (ppz - (pp->pz * car)) - pp->aoz;
        } 
        else 
        {
            car = 0.5f * (car - ln);
            
            vec.x = px;
            vec.y = 0;
            vec.z = pz;
            
            njUnitVector(&vec);
            
            vec.x *= car;
            vec.z *= car;
            
            plp->px += vec.x;
            plp->pz += vec.z;
            
            pp->px -= vec.x;
            pp->pz -= vec.z;
        }
        
        pp->stflg  |= 0x4;
        plp->stflg |= 0x2;
    }
}

// 100% matching!
void bhCheckEnemies(BH_PWORK* pp)
{
    float px;         
    float pz;        
    float ln;          
    float ex0;        
    float ey0;    
    float ez0;        
    float ex1;     
    float ey1;         
    float ez1;        
    int r;            
    int i;           
    int hct;            
    NJS_POINT3 ps[128];
    BH_PWORK* enep; // not from DWARF
    float car;      // not from DWARF

    if (!(pp->flg2 & 0x1)) 
    {
        ex0 = pp->px + pp->aox;
        ey0 = pp->py + pp->aoy;
        ez0 = pp->pz + pp->aoz;
        
        hct = 0;
        
        for (i = 0; i < sys->ewk_n; i++) 
        {
            enep = &ene[i];
            
            if (((((enep->flg & 0x1)) && ((enep->flg & 0x8))) && (!(enep->stflg & 0x1000000))) && ((((unsigned int)pp & ~0x80000000)) != (((unsigned int)enep & ~0x80000000))) && ((((((unsigned int)pp & ~0x80000000)) != (((unsigned int)plp & ~0x80000000))) || ((enep->flg & 0x40))) && ((!(enep->flg & 0x80)) || (!(((O_WRK*)enep->lkwkp)->stflg & 0x1000000))))) 
            {
                ex1 = enep->px + enep->aox;
                ey1 = enep->py + enep->aoy;
                ez1 = enep->pz + enep->aoz;
                
                px = ex0 - ex1;
                pz = ez0 - ez1;
                
                ln = njSqrt((px * px) + (pz * pz));
                
                car = pp->car + enep->car;
                
                if ((ln < car) && ((ey1 <= (ey0 + pp->cah)) && ((ey1 + enep->cah) >= ey0)))
                {
                    r = 10430.381f * atan2f(ex1 - ex0, ez1 - ez0);
                    
                    car = pp->car + enep->car;
                    
                    njSinCos(r, &ps[hct].x, &ps[hct].z);
                    
                    ps[hct].x = (ex1 - (car * ps[hct].x)) - pp->aox; 
                    ps[hct].z = (ez1 - (car * ps[hct].z)) - pp->aoz;
                    
                    hct++;
                    
                    if (pp == plp) 
                    { 
                        enep->stflg |= 0x4;
                    } 
                    else 
                    {
                        enep->stflg |= 0x2;
                    }
                } 
            }
        }
        
        if (hct != 0) 
        {
            if (hct > 1) 
            {
                px = pz = 0;
                
                for (i = 0; i < hct; i++) 
                {
                    px += ps[i].x;
                    pz += ps[i].z;
                } 
                
                pp->px = px / hct;
                pp->pz = pz / hct;
            } 
            else 
            {
                pp->px = ps->x;
                pp->pz = ps->z;
            }
        }
    }
}

// 100% matching!
int bhCheckWallAttrB89(ATR_WORK* hp)
{
    int i;
    unsigned char* cnop;
    
    if ((!(hp->attr & 0x100)) || ((sys->st_flg & 0x1))) 
    {
        return 1;
    }
    
    if (!(hp->attr & 0x200))
    {
        return 0;
    }
    
    cnop = &hp->prm1;
    
    for (i = 0; i < 3; i++, cnop++) 
    {
        if (*cnop == (unsigned char)cam.ncut) 
        {
            return 0;
        }
    }
    
    return 1;
}

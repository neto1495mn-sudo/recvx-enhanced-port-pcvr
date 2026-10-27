#include "../../../ps2/veronica/prog/ps2_NaColi.h"
#include "../../../ps2/veronica/prog/ps2_NaMath.h"

// 100% matching!
Bool	njIsParalellL2L(NJS_LINE *l1, NJS_LINE *l2)
{
    float fX1, fY1, fZ1;
    float fX2, fY2, fZ2;
    float fLength;
    
    fX1 = l1->vx;
    fY1 = l1->vy;  
    fZ1 = l1->vz;
    
    fLength = njInvertSqrt((fX1 * fX1) + (fY1 * fY1) + (fZ1 * fZ1));
    
    fX1 *= fLength;
    fY1 *= fLength;
    fZ1 *= fLength;
    
    fX2 = l2->vx;
    fY2 = l2->vy;
    fZ2 = l2->vz;
    
    fLength = njInvertSqrt((fX2 * fX2) + (fY2 * fY2) + (fZ2 * fZ2));
    
    fX2 *= fLength;
    fY2 *= fLength;
    fZ2 *= fLength;
    
    return (0.995f <= fabsf((fX1 * fX2) + (fY1 * fY2) + (fZ1 * fZ2))) ? 1 : 0;
}

// 100% matching!
Bool	njIsParalellL2PL(NJS_LINE *l, NJS_PLANE *pl)
{
    float fLength;
    float fPx, fPy, fPz;
    float fLx, fLy, fLz;
    
    fPx = pl->vx;
    fPy = pl->vy;  
    fPz = pl->vz;
    
    fLength = njInvertSqrt((fPx * fPx) + (fPy * fPy) + (fPz * fPz));
    
    fPx *= fLength;
    fPy *= fLength;
    fPz *= fLength;

    fLx = l->vx;
    fLy = l->vy;  
    fLz = l->vz;

    fLength = njInvertSqrt((fLx * fLx) + (fLy * fLy) + (fLz * fLz));
    
    fLx *= fLength;
    fLy *= fLength;
    fLz *= fLength;

    return (fabsf((fPx * fLx) + (fPy * fLy) + (fPz * fLz)) <= 0.025f) ? 1 : 0;
}

// 100% matching!
Float	njDistanceP2P(NJS_POINT3 *p1, NJS_POINT3 *p2)
{
    float fDeltaX, fDeltaY, fDeltaZ;

    fDeltaX = p2->x - p1->x;
    fDeltaY = p2->y - p1->y;
    fDeltaZ = p2->z - p1->z;

    return njSqrt((fDeltaX * fDeltaX) + (fDeltaY * fDeltaY) + (fDeltaZ * fDeltaZ));
}

// 100% matching! 
Float	njDistanceP2L(NJS_POINT3 *p, NJS_LINE *l, NJS_POINT3 *cp) 
{
    float fLx, fLy, fLz;   
    float fLl;  
    float fDx, fDy, fDz;    
    float fDl;  

    fDx = l->vx;
    fDy = l->vy;
    fDz = l->vz;

    fDl = njInvertSqrt((fDx * fDx) + (fDy * fDy) + (fDz * fDz));
    
    fDx *= fDl;
    fDy *= fDl;
    fDz *= fDl;

    fLx = p->x - l->px;
    fLy = p->y - l->py;
    fLz = p->z - l->pz;

    fLl = (fLx * fLx) + (fLy * fLy) + (fLz * fLz); 

    if (fLl == 0) 
    {
        if (cp != NULL) 
        {
            cp->x = l->px;
            cp->y = l->py;
            cp->z = l->pz;
        }
        
        return 0;
    }

    fDl = (fDx * fLx) + (fDy * fLy) + (fDz * fLz);
    
    if (cp != NULL) 
    {
        cp->x = l->px + (fDx * fDl);
        cp->y = l->py + (fDy * fDl);
        cp->z = l->pz + (fDz * fDl);
    }

    return njSqrt(fLl - (fDl * fDl));
}

// 100% matching!
Float	njDistanceP2PL(NJS_POINT3 *p, NJS_PLANE *pl, NJS_POINT3 *cp) 
{
    float fLength; 
    float fVx, fVy, fVz;     

    fVx = pl->vx;
    fVy = pl->vy;
    fVz = pl->vz;
    
    fLength = njInvertSqrt((fVx * fVx) + (fVy * fVy) + (fVz * fVz));
    
    fVx *= fLength;
    fVy *= fLength; 
    fVz *= fLength;
    
    fLength = (fVx * (p->x - pl->px)) + (fVy * (p->y - pl->py)) + (fVz * (p->z - pl->pz));
    
    if (cp != NULL) 
    {
        cp->x = p->x - (fVx * fLength);
        cp->y = p->y - (fVy * fLength);
        cp->z = p->z - (fVz * fLength);
    }
    
    return fabsf(fLength);
}

// 100% matching!
Float	njDistanceL2L(NJS_LINE *l1, NJS_LINE *l2, NJS_POINT3 *cp1, NJS_POINT3 *cp2)
{
    float fT;   
    float fIV; 
    float fV1x, fV1y, fV1z; 
    float fV2x, fV2y, fV2z; 
    float fNx, fNy, fNz; 
    float fDx, fDy, fDz;
  
    fV1x = l1->vx;
    fV1y = l1->vy;
    fV1z = l1->vz;
    
    fT = njInvertSqrt((fV1x * fV1x) + (fV1y * fV1y) + (fV1z * fV1z));
    
    fV2x = l2->vx;
    fV2y = l2->vy;
    fV2z = l2->vz;
    
    fV1x *= fT;
    fV1y *= fT;
    fV1z *= fT;
    
    fT = njInvertSqrt((fV2x * fV2x) + (fV2y * fV2y) + (fV2z * fV2z)); 
    
    fV2x *= fT;
    fV2y *= fT;
    fV2z *= fT;
    
    fIV = (fV1x * fV2x) + (fV1y * fV2y) + (fV1z * fV2z);
    
    if (0.975f < fabsf(fIV)) 
    {
        if (cp1 != NULL) 
        {
            cp1->x = l1->px;
            cp1->y = l1->py;
            cp1->z = l1->pz;
        }
        
        return njDistanceP2L((NJS_POINT3*)l1, l2, cp2);
    }
    else 
    {
        fDx = l2->px - l1->px;
        fDy = l2->py - l1->py;
        fDz = l2->pz - l1->pz;
        
        if ((cp1 != NULL) || (cp2 != NULL)) 
        {
            fNx = (fDx * fV1x) + (fDy * fV1y) + (fDz * fV1z);
            fNy = (fDx * fV2x) + (fDy * fV2y) + (fDz * fV2z);
            fNz = 1.0f - (fIV * fIV);
            
            if (cp1 != NULL) 
            {
                fT = (fNx - (fIV * fNy)) / fNz;
                
                cp1->x = l1->px + (fV1x * fT);
                cp1->y = l1->py + (fV1y * fT);
                cp1->z = l1->pz + (fV1z * fT);
            }
            
            if (cp2 != NULL) 
            {
                fT = ((fNx * fIV) - fNy) / fNz;
                
                cp2->x = l2->px + (fV2x * fT);
                cp2->y = l2->py + (fV2y * fT);
                cp2->z = l2->pz + (fV2z * fT);
            }
        }
        
        fNx = (fV1y * fV2z) - (fV1z * fV2y);
        fNy = (fV1z * fV2x) - (fV1x * fV2z);
        fNz = (fV1x * fV2y) - (fV1y * fV2x);
        
        fT = fabsf((fNx * fDx) + (fNy * fDy) + (fNz * fDz));
        
        fV1x = njInvertSqrt((fNx * fNx) + (fNy * fNy) + (fNz * fNz));
        
        fIV = fT * fV1x;
        
        if (fIV < 0.158f) 
        {
            fIV = 0;
        }
    }
    
    return fIV;
}

// 100% matching!
Float	njDistanceL2PL(NJS_LINE *l, NJS_PLANE *pl, NJS_POINT3 *cp)
{
    float fA, fB, fC;      
    float fL, fM, fN;    
    float fX, fY, fZ;      
    float fT;      
  
    fA = pl->vx;
    fB = pl->vy;
    fC = pl->vz;

    fT = njInvertSqrt((fA * fA) + (fB * fB) + (fC * fC));

    fA *= fT;
    fB *= fT;
    fC *= fT;

    fL = l->vx;
    fM = l->vy;
    fN = l->vz;

    fT = njInvertSqrt((fL * fL) + (fM * fM) + (fN * fN));

    fL *= fT;
    fM *= fT;
    fN *= fT;

    fT = (fA * fL) + (fB * fM) + (fC * fN);
    
    if (cp != NULL) 
    {
        if (0.025f < fabsf(fT)) 
        {
            fX = l->px;
            fY = l->py;
            fZ = l->pz;
            
            fT = -((fA * (fX - pl->px)) + (fB * (fY - pl->py)) + (fC * (fZ - pl->pz))) / fT;
            
            cp->x = fX + (fL * fT);
            cp->y = fY + (fM * fT);
            cp->z = fZ + (fN * fT);

            return 0;
        }
        else 
        {
            return njDistanceP2PL((NJS_POINT3*)&l->px, pl, cp);
        }
    }
    
    return fT;
}

// 100% matching!
void	njGetPlaneNormal(NJS_POINT3 *p, NJS_VECTOR *v)
{
	NJS_POINT3* pP2, *pP3;

	pP2 = &p[1];
	pP3 = &p[2];

	v->x = ((pP2->y - p->y) * (pP3->z - pP2->z)) - ((pP2->z - p->z) * (pP3->y - pP2->y));
    v->y = ((pP2->z - p->z) * (pP3->x - pP2->x)) - ((pP2->x - p->x) * (pP3->z - pP2->z));
    v->z = ((pP2->x - p->x) * (pP3->y - pP2->y)) - ((pP2->y - p->y) * (pP3->x - pP2->x));
}

// 100% matching!
void	njGetPlaneNormal2(NJS_POINT3 *p0, NJS_POINT3 *p1, NJS_POINT3 *p2, NJS_VECTOR *v)
{
	v->x = ((p1->y - p0->y) * (p2->z - p1->z)) - ((p1->z - p0->z) * (p2->y - p1->y));
    v->y = ((p1->z - p0->z) * (p2->x - p1->x)) - ((p1->x - p0->x) * (p2->z - p1->z));
    v->z = ((p1->x - p0->x) * (p2->y - p1->y)) - ((p1->y - p0->y) * (p2->x - p1->x));
}

// 100% matching!
Int     njCollisionCheckSS(NJS_SPHERE *sphere1, NJS_SPHERE *sphere2)
{
	NJS_POINT3* pCenter1, *pCenter2;
    float fDx, fDy, fDz;
	float fR;

	pCenter1 = &sphere1->c;
	pCenter2 = &sphere2->c;

	fDx = pCenter2->x - pCenter1->x;
	fDy = pCenter2->y - pCenter1->y;
	fDz = pCenter2->z - pCenter1->z;

	fR = sphere1->r + sphere2->r;
	
	if (((fDx * fDx) + (fDy * fDy) + (fDz * fDz)) <= (fR * fR)) 
	{
		return 1;
	}

	return 0;
}

// 100% matching!
Int     njCollisionCheckCC(NJS_CAPSULE *h1, NJS_CAPSULE *h2)
{
    NJS_POINT3* pP12, *pP22;  
    NJS_LINE Line1, Line2;    
    float fLength;   
    NJS_POINT3 Point1, Point2; 
    NJS_SPHERE Sphere; 

    pP12 = &h1->c2;
    pP22 = &h2->c2;
    
    Line1.vx = h1->c2.x - h1->c1.x;
    Line1.vy = h1->c2.y - h1->c1.y;
    Line1.vz = h1->c2.z - h1->c1.z;
    
    Line1.px = h1->c1.x;
    Line1.py = h1->c1.y;
    Line1.pz = h1->c1.z;
    
    Line2.vx = h2->c2.x - h2->c1.x;
    Line2.vy = h2->c2.y - h2->c1.y;
    Line2.vz = h2->c2.z - h2->c1.z;
    
    Line2.px = h2->c1.x;
    Line2.py = h2->c1.y;
    Line2.pz = h2->c1.z;
    
    if (njIsParalellL2L(&Line1, &Line2) != 0) 
    {
        Sphere.r = h1->r;
        
        Sphere.c.x = h1->c1.x;
        Sphere.c.y = h1->c1.y;
        Sphere.c.z = h1->c1.z;
        
        if (njCollisionCheckSC(&Sphere, h2) != 0) 
        {
            return 1;
        }
        
        Sphere.c.x = pP12->x;
        Sphere.c.y = pP12->y;
        Sphere.c.z = pP12->z;
        
        if (njCollisionCheckSC(&Sphere, h2) != 0)
        {
            return 1;
        }
        else 
        {
            return 0;
        }
    }
    
    fLength = h1->r + h2->r;
    
    if (fLength <= njDistanceL2L(&Line1, &Line2, &Point1, &Point2)) 
    {
        return 0;
    }
    
    fLength = njDistanceP2P(&h1->c1, pP12);
    
    if (fLength < njDistanceP2P(&Point1, &h1->c1)) 
    {
        Point1.x = pP12->x;
        Point1.y = pP12->y;
        Point1.z = pP12->z;
    }
    else if (fLength < njDistanceP2P(&Point1, pP12)) 
    {
        Point1.x = h1->c1.x;
        Point1.y = h1->c1.y;
        Point1.z = h1->c1.z;
    }
    
    fLength = njDistanceP2P(&h2->c1, pP22);
    
    if (fLength < njDistanceP2P(&Point2, &h2->c1)) 
    {
        Point2.x = pP22->x;
        Point2.y = pP22->y;
        Point2.z = pP22->z;
    }
    else if (fLength < njDistanceP2P(&Point2, pP22)) 
    {
        Point2.x = h2->c1.x;
        Point2.y = h2->c1.y;
        Point2.z = h2->c1.z;
    }
    
    if (njDistanceP2P(&Point1, &Point2) < (h1->r + h2->r)) 
    {
        return 1;
    }
    
    return 0;
}

// 100% matching!
Int     njCollisionCheckSC(NJS_SPHERE *sphere, NJS_CAPSULE *capsule) 
{
    NJS_POINT3* pP2;  
    NJS_LINE Line;    
    float fLength;    
    NJS_POINT3 Point; 

    pP2 = &capsule->c2;
    
    Line.vx = capsule->c2.x - capsule->c1.x;
    Line.vy = capsule->c2.y - capsule->c1.y;
    Line.vz = capsule->c2.z - capsule->c1.z;
    
    Line.px = capsule->c1.x;
    Line.py = capsule->c1.y;
    Line.pz = capsule->c1.z;
    
    fLength = capsule->r + sphere->r;
    
    if (fLength <= njDistanceP2L(&sphere->c, &Line, &Point)) 
    {
        return 0;
    }
    
    fLength = njDistanceP2P(&capsule->c1, pP2);
    
    if (fLength < njDistanceP2P(&Point, &capsule->c1)) 
    {
        Point.x = pP2->x;
        Point.y = pP2->y;
        Point.z = pP2->z;
    }
    else if (fLength < njDistanceP2P(&Point, pP2))
    {
        Point.x = capsule->c1.x;
        Point.y = capsule->c1.y;
        Point.z = capsule->c1.z;
    }
    
    if (njDistanceP2P(&sphere->c, &Point) < (capsule->r + sphere->r)) 
    {
        return 1;
    }
    
    return 0;
}

// 100% matching!
Int     njCollisionCheckBS(NJS_BOX *box, NJS_SPHERE *sphere)
{
    float fCx, fCy, fCz;          
    float fR;            
    NJS_CAPSULE Capsule; 

    fR = sphere->r;
    
    fCx = sphere->c.x;
    fCy = sphere->c.y;
    fCz = sphere->c.z;
    
    if (((fCx <= (box->v[0].x - fR)) || ((box->v[6].x + fR) <= fCx)) || ((fCy <= (box->v[6].y - fR)) || ((box->v[0].y + fR) <= fCy)) || ((fCz <= (box->v[6].z - fR)) || ((box->v[0].z + fR) <= fCz))) 
    {
        return 0;
    }
    
    if (((box->v[0].x <= fCx) && (fCx <= box->v[6].x)) && ((box->v[6].z <= fCz) && (fCz <= box->v[0].z)))
    {
        return 1;
    }
    
    if (((box->v[6].y <= fCy) && (fCy <= box->v[0].y)) && ((box->v[6].z <= fCz) && (fCz <= box->v[0].z))) 
    {
        return 1;
    }
    
    if (((box->v[0].x <= fCx) && (fCx <= box->v[6].x)) && ((box->v[6].y <= fCy) && (fCy <= box->v[0].y))) 
    {
        return 1;
    }
    
    Capsule.r = 0;
    
    Capsule.c1.x = box->v[0].x;
    Capsule.c1.y = box->v[0].y;
    Capsule.c1.z = box->v[0].z;
    
    Capsule.c2.x = box->v[3].x;
    Capsule.c2.y = box->v[3].y;
    Capsule.c2.z = box->v[3].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[4].x;
    Capsule.c2.y = box->v[4].y;
    Capsule.c2.z = box->v[4].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[1].x;
    Capsule.c2.y = box->v[1].y;
    Capsule.c2.z = box->v[1].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = box->v[5].x;
    Capsule.c1.y = box->v[5].y;
    Capsule.c1.z = box->v[5].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[4].x;
    Capsule.c2.y = box->v[4].y;
    Capsule.c2.z = box->v[4].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[6].x;
    Capsule.c2.y = box->v[6].y;
    Capsule.c2.z = box->v[6].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = box->v[2].x;
    Capsule.c1.y = box->v[2].y;
    Capsule.c1.z = box->v[2].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[1].x;
    Capsule.c2.y = box->v[1].y;
    Capsule.c2.z = box->v[1].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0)
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[3].x;
    Capsule.c2.y = box->v[3].y;
    Capsule.c2.z = box->v[3].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = box->v[7].x;
    Capsule.c1.y = box->v[7].y;
    Capsule.c1.z = box->v[7].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[4].x;
    Capsule.c2.y = box->v[4].y;
    Capsule.c2.z = box->v[4].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[6].x;
    Capsule.c2.y = box->v[6].y;
    Capsule.c2.z = box->v[6].z;
    
    if (njCollisionCheckSC(sphere, &Capsule) != 0) 
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

// 100% matching!
Int     njCollisionCheckBC(NJS_BOX *box, NJS_CAPSULE *capsule)
{
    NJS_CAPSULE Capsule;            
    NJS_LINE Line;             
    float fX1, fY1, fZ1;                    
    float fX2, fY2, fZ2;                          
    float fR;                       
    float fMinX, fMaxX;                      
    float fMinY, fMaxY;                   
    float fMinZ, fMaxZ;                       
    unsigned int ulCnt;                 
    static unsigned int ulVertex[6][4] = 
    {
        { 0, 1, 2, 3 },
        { 4, 7, 6, 5 },
        { 0, 3, 7, 4 },
        { 2, 1, 5, 6 },
        { 1, 0, 4, 5 },
        { 3, 2, 6, 7 }
    };
    static NJS_POINT3 Normal[6] = 
    {
        {  0,     1.0f,     0 },
        {  0,    -1.0f,     0 },
        {  0,        0,  1.0f },
        {  0,        0, -1.0f },
        { -1.0f,     0,     0 },
        {  1.0f,        0,  0 }
    };    

    fX1 = capsule->c1.x;
    fX2 = capsule->c2.x;
    fY1 = capsule->c1.y;
    fZ1 = capsule->c1.z;
    fY2 = capsule->c2.y;
    fZ2 = capsule->c2.z;
    
    fR = capsule->r; 
    
    if (fX1 < fX2) 
    {
        fMinX = fX1 - fR;
        fMaxX = fX2 + fR;
    }
    else
    {
        fMinX = fX2 - fR;
        fMaxX = fX1 + fR;
    }
    
    if (fY1 < fY2) 
    {
        fMinY = fY1 - fR;
        fMaxY = fY2 + fR;
    }
    else 
    {
        fMinY = fY2 - fR;
        fMaxY = fY1 + fR;
    }
    
    if (fZ1 < fZ2) 
    {
        fMinZ = fZ1 - fR;
        fMaxZ = fZ2 + fR;
    }
    else
    {
        fMinZ = fZ2 - fR;
        fMaxZ = fZ1 + fR;
    }
    
    if (((fMaxX <= box->v[0].x) || (box->v[6].x <= fMinX)) || ((fMaxY <= box->v[6].y) || (box->v[0].y <= fMinY)) || ((fMaxZ <= box->v[6].z) || (box->v[0].z <= fMinZ))) 
    {
        return 0;
    }
    
    if (((((box->v[0].x - fR) < fX1) && (fX1 < (box->v[6].x + fR))) && ((box->v[6].y <= fY1) && (fY1 <= box->v[0].y)) && ((box->v[6].z <= fZ1) && (fZ1 <= box->v[0].z))) || (((box->v[0].x <= fX1) && (fX1 <= box->v[6].x)) && (((box->v[6].y - fR) < fY1) && (fY1 < (box->v[0].y + fR))) && ((box->v[6].z <= fZ1) && (fZ1 <= box->v[0].z))) || (((box->v[0].x <= fX1) && (fX1 <= box->v[6].x)) && ((box->v[6].y <= fY1) && (fY1 <= box->v[0].y)) && (((box->v[6].z - fR) < fZ1) && (fZ1 < (box->v[0].z + fR))))) 
    {
        return 1;
    }
    
    if (((((box->v[0].x - fR) < fX2) && (fX2 < (box->v[6].x + fR))) && ((box->v[6].y <= fY2) && (fY2 <= box->v[0].y)) && ((box->v[6].z <= fZ2) && (fZ2 <= box->v[0].z))) || (((box->v[0].x <= fX2) && (fX2 <= box->v[6].x)) && (((box->v[6].y - fR) < fY2) && (fY2 < (box->v[0].y + fR))) && ((box->v[6].z <= fZ2) && (fZ2 <= box->v[0].z))) || (((box->v[0].x <= fX2) && (fX2 <= box->v[6].x)) && ((box->v[6].y <= fY2) && (fY2 <= box->v[0].y)) && (((box->v[6].z - fR) < fZ2) && (fZ2 < (box->v[0].z + fR))))) 
    {
        return 1;
    }
    
    Capsule.r = 0;
    
    Capsule.c1.x = box->v[0].x;
    Capsule.c1.y = box->v[0].y;
    Capsule.c1.z = box->v[0].z;
    
    Capsule.c2.x = box->v[3].x;
    Capsule.c2.y = box->v[3].y;
    Capsule.c2.z = box->v[3].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[4].x;
    Capsule.c2.y = box->v[4].y;
    Capsule.c2.z = box->v[4].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[1].x;
    Capsule.c2.y = box->v[1].y;
    Capsule.c2.z = box->v[1].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = box->v[5].x;
    Capsule.c1.y = box->v[5].y;
    Capsule.c1.z = box->v[5].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[4].x;
    Capsule.c2.y = box->v[4].y;
    Capsule.c2.z = box->v[4].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[6].x;
    Capsule.c2.y = box->v[6].y;
    Capsule.c2.z = box->v[6].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = box->v[2].x;
    Capsule.c1.y = box->v[2].y;
    Capsule.c1.z = box->v[2].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0)
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[1].x;
    Capsule.c2.y = box->v[1].y;
    Capsule.c2.z = box->v[1].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[3].x;
    Capsule.c2.y = box->v[3].y;
    Capsule.c2.z = box->v[3].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = box->v[7].x;
    Capsule.c1.y = box->v[7].y;
    Capsule.c1.z = box->v[7].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[4].x;
    Capsule.c2.y = box->v[4].y;
    Capsule.c2.z = box->v[4].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = box->v[6].x;
    Capsule.c2.y = box->v[6].y;
    Capsule.c2.z = box->v[6].z;
    
    if (njCollisionCheckCC(capsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Line.px = fX1;
    Line.py = fY1;
    Line.pz = fZ1;
    
    Line.vx = fX2 - fX1;
    Line.vy = fY2 - fY1;
    Line.vz = fZ2 - fZ1;
    
    for (ulCnt = 0; ulCnt < 6; ulCnt++) 
    { 
        if (njCheckPlane4AndLine(&box->v[ulVertex[ulCnt][0]], &box->v[ulVertex[ulCnt][1]], &box->v[ulVertex[ulCnt][2]], &box->v[ulVertex[ulCnt][3]], &Normal[ulCnt], &Line) != 0) 
        {
            return 1;
        }
    }
    
    return 0;
}

// 100% matching!
int njCheckPlane4AndLine(NJS_POINT3* pP1, NJS_POINT3* pP2, NJS_POINT3* pP3, NJS_POINT3* pP4, NJS_POINT3* pPN, NJS_LINE* pLine)
{
    float fVX1, fVY1, fVZ1;
    float fVX2, fVY2, fVZ2;
    float fVX3, fVY3, fVZ3; 
    float fVX4, fVY4, fVZ4; 
    float fT1, fT2, fT3, fT4;  

    fVX2 = pLine->vx;
    fVY2 = pLine->vy;
    fVZ2 = pLine->vz;
    
    fVX1 = pPN->x;
    fVY1 = pPN->y;
    fVZ1 = pPN->z;
    
    fVX3 = pLine->px;
    fVY3 = pLine->py;
    fVZ3 = pLine->pz;

    fT3 = (fVX2 * fVX2) + (fVY2 * fVY2) + (fVZ2 * fVZ2);
    fT1 = njInvertSqrt(fT3);
    
    fVX2 *= fT1;
    fVY2 *= fT1;
    fVZ2 *= fT1;

    fT2 = (fVX1 * fVX2) + (fVY1 * fVY2) + (fVZ1 * fVZ2);
    
    if (fabsf(fT2) <= 0.025f) 
    {
        return 0;
    }

    fT2 = -((fVX1 * (fVX3 - pP1->x)) + (fVY1 * (fVY3 - pP1->y)) + (fVZ1 * (fVZ3 - pP1->z))) / fT2;
    
    fVX4 = fVX3 + (fVX2 * fT2);
    fVY4 = fVY3 + (fVY2 * fT2);
    fVZ4 = fVZ3 + (fVZ2 * fT2);

    fVX1 = fVX4 - fVX3;
    fVY1 = fVY4 - fVY3;
    fVZ1 = fVZ4 - fVZ3;
    
    fT2 = (fVX1 * fVX1) + (fVY1 * fVY1) + (fVZ1 * fVZ1);
    
    if (fT3 < fT2) 
    {
        return 0;
    }

    fVX1 -= pLine->vx;
    fVY1 -= pLine->vy;
    fVZ1 -= pLine->vz;

    if (fT3 < ((fVX1 * fVX1) + (fVY1 * fVY1) + (fVZ1 * fVZ1))) 
    {
        return 0;
    }
          
    fVX2 = pP1->x - fVX4;
    fVY2 = pP1->y - fVY4;
    fVZ2 = pP1->z - fVZ4;
    
    fT1 = (fVX2 * fVX2) + (fVY2 * fVY2) + (fVZ2 * fVZ2);
    
    if (fT1 < 0.001f) 
    {
        return 1;
    }
    
    fT1 = njInvertSqrt(fT1);
    
    fVX2 *= fT1;
    fVY2 *= fT1;
    fVZ2 *= fT1;

    fVX3 = pP2->x - fVX4;
    fVY3 = pP2->y - fVY4;
    fVZ3 = pP2->z - fVZ4;
    
    fT2 = (fVX3 * fVX3) + (fVY3 * fVY3) + (fVZ3 * fVZ3);
    
    if (fT2 < 0.001f) 
    {
        return 1;
    }
    
    fT2 = njInvertSqrt(fT2);

    fVX3 *= fT2;
    fVY3 *= fT2;
    fVZ3 *= fT2;

    fVX1 = pP3->x - fVX4;
    fVY1 = pP3->y - fVY4;
    fVZ1 = pP3->z - fVZ4;
    
    fT3 = (fVX1 * fVX1) + (fVY1 * fVY1) + (fVZ1 * fVZ1);
    
    if (fT3 < 0.001f)
    {
        return 1;
    }
    
    fT3 = njInvertSqrt(fT3);
    
    fVX1 *= fT3;
    fVY1 *= fT3;
    fVZ1 *= fT3;

    fVX4 = pP4->x - fVX4;
    fVY4 = pP4->y - fVY4;
    fVZ4 = pP4->z - fVZ4;
    
    fT4 = (fVX4 * fVX4) + (fVY4 * fVY4) + (fVZ4 * fVZ4);
    
    if (fT4 < 0.001f) 
    {
        return 1;
    }
    
    fT4 = njInvertSqrt(fT4);
    
    fVX4 *= fT4;
    fVY4 *= fT4;
    fVZ4 *= fT4;

    fT1 = (fVX2 * fVX3) + (fVY2 * fVY3) + (fVZ2 * fVZ3);
    
    if ((fT1 < -1.0f) || (1.0f < fT1)) 
    {
        return 0;
    }
    
    fT2 = (fVX3 * fVX1) + (fVY3 * fVY1) + (fVZ3 * fVZ1);
    
    if ((fT2 < -1.0f) || (1.0f < fT2)) 
    {
        return 0;
    }

    fT3 = (fVX1 * fVX4) + (fVY1 * fVY4) + (fVZ1 * fVZ4);
    
    if ((fT3 < -1.0f) || (1.0f < fT3))
    {
        return 0;
    }
    
    fT4 = (fVX4 * fVX2) + (fVY4 * fVY2) + (fVZ4 * fVZ2);
    
    if ((fT4 < -1.0f) || (1.0f < fT4))
    {
        return 0;
    }

    if ((acosf(fT1) + acosf(fT2) + acosf(fT3) + acosf(fT4)) < 6.25f)
    {
        return 0;
    }

    return 1;
}

// 100% matching!
int njCollisionCheckBC2(NJS_BOX* pBox, NJS_CAPSULE* pCapsule) 
{
    NJS_CAPSULE Capsule;                
    NJS_LINE Line;                      
    NJS_PLANE Plane;                    
    NJS_POINT3 Cross;                   
    float fX1, fY1, fZ1;                          
    float fX2, fY2, fZ2;                         
    float fR;                        
    unsigned int ulCnt;                 
    static unsigned int ulVertex[6][4] = 
    {
        { 0, 1, 2, 3 }, 
        { 4, 7, 6, 5 }, 
        { 0, 3, 7, 4 }, 
        { 2, 1, 5, 6 }, 
        { 1, 0, 4, 5 }, 
        { 3, 2, 6, 7 }  
    }; 
    static NJS_POINT3 Normal[6] = 
    {
        {     0,  1.0f,     0 }, 
        {     0, -1.0f,     0 }, 
        {     0,     0,  1.0f }, 
        {     0,     0, -1.0f }, 
        { -1.0f,     0,     0 }, 
        {  1.0f,     0,     0 } 
    };        

    fX1 = pCapsule->c1.x;
    fY1 = pCapsule->c1.y;
    fZ1 = pCapsule->c1.z;
    
    fX2 = pCapsule->c2.x;
    fY2 = pCapsule->c2.y;
    fZ2 = pCapsule->c2.z;
    
    fR = pCapsule->r;
    
    Capsule.r = 0;
    
    Capsule.c1.x = pBox->v[0].x;
    Capsule.c1.y = pBox->v[0].y;
    Capsule.c1.z = pBox->v[0].z;
    
    Capsule.c2.x = pBox->v[3].x;
    Capsule.c2.y = pBox->v[3].y;
    Capsule.c2.z = pBox->v[3].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[4].x;
    Capsule.c2.y = pBox->v[4].y;
    Capsule.c2.z = pBox->v[4].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[1].x;
    Capsule.c2.y = pBox->v[1].y;
    Capsule.c2.z = pBox->v[1].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = pBox->v[5].x;
    Capsule.c1.y = pBox->v[5].y;
    Capsule.c1.z = pBox->v[5].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[4].x;
    Capsule.c2.y = pBox->v[4].y;
    Capsule.c2.z = pBox->v[4].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[6].x;
    Capsule.c2.y = pBox->v[6].y;
    Capsule.c2.z = pBox->v[6].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = pBox->v[2].x;
    Capsule.c1.y = pBox->v[2].y;
    Capsule.c1.z = pBox->v[2].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[1].x;
    Capsule.c2.y = pBox->v[1].y;
    Capsule.c2.z = pBox->v[1].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[3].x;
    Capsule.c2.y = pBox->v[3].y;
    Capsule.c2.z = pBox->v[3].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c1.x = pBox->v[7].x;
    Capsule.c1.y = pBox->v[7].y;
    Capsule.c1.z = pBox->v[7].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[4].x;
    Capsule.c2.y = pBox->v[4].y;
    Capsule.c2.z = pBox->v[4].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Capsule.c2.x = pBox->v[6].x;
    Capsule.c2.y = pBox->v[6].y;
    Capsule.c2.z = pBox->v[6].z;
    
    if (njCollisionCheckCC(pCapsule, &Capsule) != 0) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[0].x;
    Plane.py = pBox->v[0].y;
    Plane.pz = pBox->v[0].z;
    
    Plane.vx = Normal->x;
    Plane.vy = Normal->y;
    Plane.vz = Normal->z;
    
    if ((njDistanceP2PL(&pCapsule->c1, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(pBox->v, &pBox->v[1], &pBox->v[2], &pBox->v[3], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[4].x;
    Plane.py = pBox->v[4].y;
    Plane.pz = pBox->v[4].z;
    
    Plane.vx = Normal[1].x;
    Plane.vy = Normal[1].y;
    Plane.vz = Normal[1].z;
    
    if ((njDistanceP2PL(&pCapsule->c1, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(&pBox->v[4], &pBox->v[5], &pBox->v[6], &pBox->v[7], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[0].x;
    Plane.py = pBox->v[0].y;
    Plane.pz = pBox->v[0].z;
    
    Plane.vx = Normal[2].x;
    Plane.vy = Normal[2].y;
    Plane.vz = Normal[2].z;
    
    if ((njDistanceP2PL(&pCapsule->c1, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(pBox->v, &pBox->v[3], &pBox->v[7], &pBox->v[4], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[1].x;
    Plane.py = pBox->v[1].y;
    Plane.pz = pBox->v[1].z;
    
    Plane.vx = Normal[3].x;
    Plane.vy = Normal[3].y;
    Plane.vz = Normal[3].z;
    
    if ((njDistanceP2PL(&pCapsule->c1, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(&pBox->v[1], &pBox->v[2], &pBox->v[6], &pBox->v[5], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[0].x;
    Plane.py = pBox->v[0].y;
    Plane.pz = pBox->v[0].z;
    
    Plane.vx = Normal[4].x;
    Plane.vy = Normal[4].y;
    Plane.vz = Normal[4].z;
    
    if ((njDistanceP2PL(&pCapsule->c1, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(pBox->v, &pBox->v[1], &pBox->v[5], &pBox->v[4], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[2].x;
    Plane.py = pBox->v[2].y;
    Plane.pz = pBox->v[2].z;
    
    Plane.vx = Normal[5].x;
    Plane.vy = Normal[5].y;
    Plane.vz = Normal[5].z;
    
    if ((njDistanceP2PL(&pCapsule->c1, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(&pBox->v[2], &pBox->v[3], &pBox->v[7], &pBox->v[6], &Cross) != 0))
    {
        return 1;
    }
    
    Plane.px = pBox->v[0].x;
    Plane.py = pBox->v[0].y;
    Plane.pz = pBox->v[0].z;
    
    Plane.vx = Normal->x;
    Plane.vy = Normal->y;
    Plane.vz = Normal->z;
    
    if ((njDistanceP2PL(&pCapsule->c2, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(pBox->v, &pBox->v[1], &pBox->v[2], &pBox->v[3], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[4].x;
    Plane.py = pBox->v[4].y;
    Plane.pz = pBox->v[4].z;
    
    Plane.vx = Normal[1].x;
    Plane.vy = Normal[1].y;
    Plane.vz = Normal[1].z;
    
    if ((njDistanceP2PL(&pCapsule->c2, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(&pBox->v[4], &pBox->v[5], &pBox->v[6], &pBox->v[7], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[0].x;
    Plane.py = pBox->v[0].y;
    Plane.pz = pBox->v[0].z;
    
    Plane.vx = Normal[2].x;
    Plane.vy = Normal[2].y;
    Plane.vz = Normal[2].z;
    
    if ((njDistanceP2PL(&pCapsule->c2, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(pBox->v, &pBox->v[3], &pBox->v[7], &pBox->v[4], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[1].x;
    Plane.py = pBox->v[1].y;
    Plane.pz = pBox->v[1].z;
    
    Plane.vx = Normal[3].x;
    Plane.vy = Normal[3].y;
    Plane.vz = Normal[3].z;
    
    if ((njDistanceP2PL(&pCapsule->c2, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(&pBox->v[1], &pBox->v[2], &pBox->v[6], &pBox->v[5], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[0].x;
    Plane.py = pBox->v[0].y;
    Plane.pz = pBox->v[0].z;
    
    Plane.vx = Normal[4].x;
    Plane.vy = Normal[4].y;
    Plane.vz = Normal[4].z;
    
    if ((njDistanceP2PL(&pCapsule->c2, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(pBox->v, &pBox->v[1], &pBox->v[5], &pBox->v[4], &Cross) != 0)) 
    {
        return 1;
    }
    
    Plane.px = pBox->v[2].x;
    Plane.py = pBox->v[2].y;
    Plane.pz = pBox->v[2].z;
    
    Plane.vx = Normal[5].x;
    Plane.vy = Normal[5].y;
    Plane.vz = Normal[5].z;
    
    if ((njDistanceP2PL(&pCapsule->c2, &Plane, &Cross) <= fR) && (njCheckPlane4IncludePoint(&pBox->v[2], &pBox->v[3], &pBox->v[7], &pBox->v[6], &Cross) != 0)) 
    {
        return 1;
    }
    
    Line.px = fX1;
    Line.py = fY1;
    Line.pz = fZ1;
    
    Line.vx = fX2 - fX1;
    Line.vy = fY2 - fY1;
    Line.vz = fZ2 - fZ1;
    
    for (ulCnt = 0; ulCnt < 6; ulCnt++) 
    { 
        if (njCheckPlane4AndLine(&pBox->v[ulVertex[ulCnt][0]], &pBox->v[ulVertex[ulCnt][1]], &pBox->v[ulVertex[ulCnt][2]], &pBox->v[ulVertex[ulCnt][3]], &Normal[ulCnt], &Line) != 0) 
        {
            return 1;
        }
    }

    return 0;
}

// 100% matching!
int njCheckPlane4IncludePoint(NJS_POINT3* pP1, NJS_POINT3* pP2, NJS_POINT3* pP3, NJS_POINT3* pP4, NJS_POINT3* pPC)
{
    float fVX1, fVY1, fVZ1; 
    float fVX2, fVY2, fVZ2;
    float fVX3, fVY3, fVZ3; 
    float cy, cz, cx; // not from DWARF
    float fVX4, fVY4, fVZ4; 
    float fT1, fT2, fT3, fT4;  

    cx = pPC->x;
    cy = pPC->y;
    cz = pPC->z;

    fVX1 = pP1->x - cx;
    fVY1 = pP1->y - cy;
    fVZ1 = pP1->z - cz;
    
    fT1 = (fVX1 * fVX1) + (fVY1 * fVY1) + (fVZ1 * fVZ1);
    
    if (fT1 < 0.001f)
    {
        return 1;
    }
    
    fT1 = njInvertSqrt(fT1);
    
    fVX1 *= fT1;
    fVY1 *= fT1;
    fVZ1 *= fT1;

    fVX2 = pP2->x - cx;
    fVY2 = pP2->y - cy;
    fVZ2 = pP2->z - cz;
    
    fT2 = (fVX2 * fVX2) + (fVY2 * fVY2) + (fVZ2 * fVZ2);
    
    if (fT2 < 0.001f) 
    {
        return 1;
    }
    
    fT2 = njInvertSqrt(fT2);
    
    fVX2 *= fT2;
    fVY2 *= fT2;
    fVZ2 *= fT2;

    fVX3 = pP3->x - cx;
    fVY3 = pP3->y - cy;
    fVZ3 = pP3->z - cz;
    
    fT3 = (fVX3 * fVX3) + (fVY3 * fVY3) + (fVZ3 * fVZ3);
    
    if (fT3 < 0.001f)
    {
        return 1;
    }
    
    fT3 = njInvertSqrt(fT3);
    
    fVX3 *= fT3;
    fVY3 *= fT3;
    fVZ3 *= fT3;

    fVX4 = pP4->x - cx;
    fVY4 = pP4->y - cy;
    fVZ4 = pP4->z - cz;
    
    fT4 = (fVX4 * fVX4) + (fVY4 * fVY4) + (fVZ4 * fVZ4);
    
    if (fT4 < 0.001f) 
    {
        return 1;
    }
    
    fT4 = njInvertSqrt(fT4);
    
    fVX4 *= fT4;
    fVY4 *= fT4;
    fVZ4 *= fT4;

    fT1 = (fVX1 * fVX2) + (fVY1 * fVY2) + (fVZ1 * fVZ2);
    
    if ((fT1 < -1.0f) || (1.0f < fT1)) 
    {
        return 0;
    }
    
    fT2 = (fVX2 * fVX3) + (fVY2 * fVY3) + (fVZ2 * fVZ3);
    
    if ((fT2 < -1.0f) || (1.0f < fT2)) 
    {
        return 0;
    }

    fT3 = (fVX3 * fVX4) + (fVY3 * fVY4) + (fVZ3 * fVZ4);
    
    if ((fT3 < -1.0f) || (1.0f < fT3)) 
    {
        return 0;
    }
    
    fT4 = (fVX4 * fVX1) + (fVY4 * fVY1) + (fVZ4 * fVZ1);
    
    if ((fT4 < -1.0f) || (1.0f < fT4)) 
    {
        return 0;
    }

    if ((acosf(fT1) + acosf(fT2) + acosf(fT3) + acosf(fT4)) < 6.25f) 
    {
        return 0;
    }

    return 1;
}

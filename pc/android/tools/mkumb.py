import math, random
from PIL import Image, ImageDraw, ImageFilter, ImageChops, ImageOps
N=2048
a=math.radians(22.5); gap=0.06; Rc=0.97; Rm=0.80
def rout(phi): return Rm+(Rc-Rm)*min(phi,1.3)**2
def seg_poly(k):
    th=math.radians(k*45-90); c,s=math.cos(th),math.sin(th)
    h=gap/2
    def to_xy(u,v):
        return (N/2+(u*c-v*s)*N/2, N/2+(u*s+v*c)*N/2)
    apex_u=h/math.sin(a)
    # corner: along line v=(u sin a - h)/cos a until r >= rout(phi)
    u=apex_u
    pts_edge=[]
    cu=None
    while u<1.2:
        v=(u*math.sin(a)-h)/math.cos(a)
        r=math.hypot(u,v); phi=abs(math.atan2(v,u))/a
        if r>=rout(phi): cu=(u,v); break
        u+=0.0005
    pu,pv=cu
    phi0=abs(math.atan2(pv,pu))/a
    pts=[(apex_u,0.0),(pu,pv)]
    # the concave outer arc from +v corner to -v corner
    m=60
    for i in range(1,m):
        t=1-2*i/m
        ph=abs(t)*phi0
        ang=t*a*phi0
        r=rout(ph)
        pts.append((r*math.cos(ang), r*math.sin(ang)))
    pts.append((pu,-pv))
    return [to_xy(u,v) for u,v in pts]
random.seed(7)
def noise(blur,seed,contrast=True):
    random.seed(seed)
    n=Image.effect_noise((N//4,N//4),90).filter(ImageFilter.GaussianBlur(blur/4)).resize((N,N),Image.BICUBIC)
    return ImageOps.autocontrast(n,cutoff=1) if contrast else n
n_big=noise(46,1); n_mid=noise(18,2); n_fine=noise(7,3)
veinsA=ImageChops.difference(n_mid.filter(ImageFilter.GaussianBlur(3)),n_fine.filter(ImageFilter.GaussianBlur(9)))
veins=ImageOps.autocontrast(veinsA,cutoff=2).point(lambda v: 255 if v<40 else max(0,255-(v-40)*5))  # bright where the difference is small = a vein
marble=ImageChops.blend(n_big,n_mid,0.35)
red_tex=ImageOps.colorize(marble,black=(150,10,16),mid=(184,16,22),white=(214,28,34))
wht_tex=ImageOps.colorize(marble,black=(208,208,214),mid=(224,224,230),white=(240,240,244))
vein_r=ImageChops.multiply(Image.new('L',(N,N),255),veins)
rad=Image.radial_gradient('L').resize((N,N),Image.BICUBIC)       # 0 centre .. 255 rim
shade=rad.point(lambda v: int(150+v*0.42))
canvas=Image.new('RGBA',(N,N),(0,0,0,0))
union=Image.new('L',(N,N),0)
emb=ImageFilter.Kernel((3,3),(-2,-1,0,-1,0,1,0,1,2),scale=1,offset=128)   # an emboss from the top-left
for k in range(8):
    red=(k%2==1)
    mask=Image.new('L',(N,N),0); ImageDraw.Draw(mask).polygon(seg_poly(k),fill=255)
    union=ImageChops.lighter(union,mask)
    tex=(red_tex if red else wht_tex).copy()
    # shading toward the rim
    sh=Image.merge('RGB',[shade]*3)
    tex=ImageChops.multiply(tex,sh)
    # the bevel: a soft height from the mask, embossed
    height=mask.filter(ImageFilter.GaussianBlur(22))
    em=height.filter(emb)
    lit=em.point(lambda v: max(0,min(255,(v-128)*5)))        # bright where it faces the light
    dark=em.point(lambda v: max(0,min(255,(128-v)*5)))
    tex=ImageChops.add(tex,Image.merge('RGB',[lit.point(lambda v:int(v*0.30))]*3))
    tex=ImageChops.subtract(tex,Image.merge('RGB',[dark.point(lambda v:int(v*0.55))]*3))
    # the outline: red segments get a bright red line, white ones a white one
    er=mask.filter(ImageFilter.MinFilter(15))
    ring=ImageChops.subtract(mask,er).filter(ImageFilter.GaussianBlur(1.2))
    ocol=Image.new('RGB',(N,N),(236,32,38) if red else (255,255,255))
    tex=Image.composite(ocol,tex,ring)
    # the gloss: a broad soft highlight on the upper half
    gl=Image.new('L',(N,N),0); d=ImageDraw.Draw(gl)
    th=math.radians(k*45-90); cx=N/2+math.cos(th)*N*0.30; cy=N/2+math.sin(th)*N*0.30
    d.ellipse((cx-N*0.07,cy-N*0.10,cx+N*0.07,cy+N*0.04),fill=34)
    gl=gl.filter(ImageFilter.GaussianBlur(60)); gl=ImageChops.multiply(gl,mask)
    tex=ImageChops.add(tex,Image.merge('RGB',[gl]*3))
    canvas.paste(tex,(0,0),mask)
# the black body (the spokes of the original) under everything
body=union.filter(ImageFilter.MaxFilter(21)).filter(ImageFilter.GaussianBlur(5))
final=Image.new('RGBA',(N,N),(0,0,0,0))
black=Image.new('RGBA',(N,N),(4,3,4,255)); final.paste(black,(0,0),body)
final.alpha_composite(canvas)
final=final.resize((1024,1024),Image.LANCZOS)
final.save('umb_hd.png',optimize=True)
bg=Image.new('RGB',(1024,1024),(8,7,9)); bg.paste(final,(0,0),final); bg.save('umb_hd_preview.png')
import os; print(os.path.getsize('umb_hd.png'))

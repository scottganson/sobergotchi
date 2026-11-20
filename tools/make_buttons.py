import os, sys
from PIL import Image, ImageDraw, ImageFont
ROOT=sys.argv[1]
S=4; W,H=122,96
im=Image.new('L',(W*S,H*S),255); d=ImageDraw.Draw(im)
def P(*a): return [v*S for v in a]
lw=int(1.3*S)
# one Sobergotchi standing up, like the enclosure CAD: black body, notches top and bottom, corner screws,
# the SOBERGOTCHI band at the top, big screen; enter is the one button on the left side, up and down the two on the right
x0,y0,w,h=44,10,34,76
x1,y1=x0+w,y0+h
d.rounded_rectangle(P(x0,y0,x1,y1),radius=5*S,fill=0)
for ny in (y0,y1-4):
    d.rounded_rectangle(P(x0+12,ny,x1-12,ny+4),radius=1*S,fill=255)
for sx in (x0+5,x1-5):
    for sy in (y0+5,y1-5):
        d.ellipse(P(sx-2,sy-2,sx+2,sy+2),fill=255); d.ellipse(P(sx-0.7,sy-0.7,sx+0.7,sy+0.7),fill=0)
# name band: dotted white line
for k in range(7): d.line(P(x0+8+k*3,y0+13,x0+9.2+k*3,y0+13),fill=255,width=int(1.0*S))
d.rounded_rectangle(P(x0+5,y0+18,x1-5,y1-10),radius=1.5*S,fill=255)
for i,ln in enumerate((0.7,0.5,0.8,0.4)):
    ly=y0+25+i*5; L=(w-16)*ln
    d.line(P(x0+8,ly,x0+8+L,ly),fill=0,width=int(1.0*S))
def frac(cad): return y0+h*(cad-38)/667
yl=frac(295); yr1=frac(235)-2.5; yr2=frac(290)+2.5  # a visible gap between up and down
nub=lambda x,y,side: d.rectangle(P(x-2.2 if side<0 else x, y-3, x if side<0 else x+2.2, y+3),fill=0)
nub(x0,yl,-1); nub(x1,yr1,1); nub(x1,yr2,1)
# arrows to the two buttons, with "hold" marks
def arrow(xa,ya,xb,yb):
    d.line(P(xa,ya,xb,yb),fill=0,width=lw)
    import math
    a=math.atan2(yb-ya,xb-xa)
    for da in (2.6,-2.6):
        d.line(P(xb,yb,xb+4*math.cos(a+da),yb+4*math.sin(a+da)),fill=0,width=lw)
arrow(6,yl,x0-4,yl)
arrow(W-8,yr1-14,x1+4,yr1-1)
arrow(W-8,yr2+14,x1+4,yr2+1)
im=im.resize((W,H),Image.LANCZOS).point(lambda v:0 if v<140 else 255)
d=ImageDraw.Draw(im); d.fontmode="1"
f=ImageFont.truetype(os.path.join(ROOT,'fonts','DejaVuSans-Bold.ttf'),9)
def label(t,cx,cy):
    bb=d.textbbox((0,0),t,font=f); tw=bb[2]-bb[0]
    d.text((cx-tw/2,cy-6),t,font=f,fill=0)
label('ENTER',20,yl-6); label('UP',W-16,yr1-20); label('DOWN',W-20,yr2+22)
im.save(os.path.join(ROOT,'art','icons','buttons.png'))
if len(sys.argv)>2: im.resize((W*4,H*4),Image.NEAREST).save(sys.argv[2])

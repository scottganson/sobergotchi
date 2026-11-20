import os, sys
from PIL import Image, ImageDraw
S=4; W,H=122,115
im=Image.new('L',(W*S,H*S),255); d=ImageDraw.Draw(im)
def P(*a): return [v*S for v in a]
lw=int(1.3*S)
# radio waves from a dot
cx,cy=61,32
d.ellipse(P(cx-4,cy-4,cx+4,cy+4),fill=0)
for r in (11,19,27):
    d.arc(P(cx-r,cy-r,cx+r,cy+r),start=-38,end=38,fill=0,width=lw)
    d.arc(P(cx-r,cy-r,cx+r,cy+r),start=142,end=218,fill=0,width=lw)
for x,y in ((20,12),(100,10),(14,48),(108,50),(61,3)):
    d.ellipse(P(x-1.2,y-1.2,x+1.2,y+1.2),fill=0)
# the Sobergotchi enclosure lying on its side: black body, notched ends, corner screws, big screen,
# a band at the head end (where SOBERGOTCHI is written); head = the end nearer the middle
def device(x0,y0,w,h,head_right):
    x1,y1=x0+w,y0+h
    d.rounded_rectangle(P(x0,y0,x1,y1),radius=4*S,fill=0)
    # notches in both short ends (the strap slots)
    for nx in (x0,x1-4):
        d.rounded_rectangle(P(nx,y0+8,nx+4,y1-8),radius=1*S,fill=255)
    # screw holes
    for sx in (x0+4.5,x1-4.5):
        for sy in (y0+4.5,y1-4.5):
            d.ellipse(P(sx-1.8,sy-1.8,sx+1.8,sy+1.8),fill=255)
            d.ellipse(P(sx-0.6,sy-0.6,sx+0.6,sy+0.6),fill=0)
    # screen (white), set back from the head band
    if head_right: sx0,sx1=x0+9,x1-15
    else: sx0,sx1=x0+15,x1-9
    d.rounded_rectangle(P(sx0,y0+4,sx1,y1-4),radius=1.5*S,fill=255)
    # a few lines of "text" on the screen
    for i,ln in enumerate((0.75,0.5,0.65)):
        ly=y0+9+i*4.5; L=(sx1-sx0-6)*ln
        d.line(P(sx0+3,ly,sx0+3+L,ly),fill=0,width=int(1.0*S))
    # the name band at the head: a dotted white stripe
    bx=x1-11 if head_right else x0+11
    for k in range(5):
        by=y0+8+k*2.6
        d.line(P(bx,by,bx,by+1.2),fill=255,width=int(1.0*S))
    # side buttons: one on one long side, two on the other
    tx=x0+w*0.45
    d.rectangle(P(tx-2,y0-1.6,tx+2,y0),fill=0)
    for tx in (x0+w*0.35,x0+w*0.6):
        d.rectangle(P(tx-2,y1,tx+2,y1+1.6),fill=0)
device(1,66,48,26,True)
device(73,66,48,26,False)
# small waves between the heads
for r in (4,8):
    d.arc(P(51-r,79-r,51+r,79+r),start=-45,end=45,fill=0,width=int(1.2*S))
    d.arc(P(71-r,79-r,71+r,79+r),start=135,end=225,fill=0,width=int(1.2*S))
for x in range(4,118,5):
    d.line(P(x,102,x+3,102),fill=0,width=int(1.0*S))
out=im.resize((W,H),Image.LANCZOS).point(lambda v:0 if v<140 else 255)
out.save(sys.argv[1])
if len(sys.argv)>2: out.resize((W*4,H*4),Image.NEAREST).save(sys.argv[2])

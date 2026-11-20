from PIL import Image, ImageDraw
S=4; W,H=122,115
im=Image.new('L',(W*S,H*S),255); d=ImageDraw.Draw(im)
def P(*a): return [v*S for v in a]
lw=int(1.3*S)
# signal: dot and arcs both ways
cx,cy=61,36
d.ellipse(P(cx-4,cy-4,cx+4,cy+4),fill=0)
for r in (11,19,27):
    d.arc(P(cx-r,cy-r,cx+r,cy+r),start=-38,end=38,fill=0,width=lw)
    d.arc(P(cx-r,cy-r,cx+r,cy+r),start=142,end=218,fill=0,width=lw)
# little ink flecks
for x,y in ((20,14),(100,12),(14,52),(108,56),(61,6)):
    d.line(P(x-2,y,x+2,y),fill=0,width=lw); d.line(P(x,y-2,x,y+2),fill=0,width=lw)
# two devices head to head (heads face the middle)
def device(x0,y0,w,h,head_right):
    d.rounded_rectangle(P(x0,y0,x0+w,y0+h),radius=4*S,outline=0,width=lw)
    # screen: towards the head end
    if head_right: sx0,sx1=x0+13,x0+w-4
    else: sx0,sx1=x0+4,x0+w-13
    d.rectangle(P(sx0,y0+4,sx1,y0+h-4),outline=0,width=lw)
    # text lines on screen
    for i,ln in enumerate((0.8,0.55,0.7)):
        ly=y0+9+i*5; L=(sx1-sx0-6)*ln
        d.line(P(sx0+3,ly,sx0+3+L,ly),fill=0,width=int(1.0*S))
    # three buttons at the foot end
    bx=x0+6 if head_right else x0+w-6
    for i in range(3):
        by=y0+7+i*7
        d.ellipse(P(bx-1.6,by-1.6,bx+1.6,by+1.6),fill=0)
device(2,72,44,30,True)
device(76,72,44,30,False)
# small waves between the heads
for r in (4,8,12):
    d.arc(P(48-r,87-r,48+r,87+r),start=-45,end=45,fill=0,width=int(1.2*S))
    d.arc(P(74-r,87-r,74+r,87+r),start=135,end=225,fill=0,width=int(1.2*S))
# ground hatching under devices
for x in range(4,118,5):
    d.line(P(x,107,x+3,107),fill=0,width=int(1.0*S))
out=im.resize((W,H),Image.LANCZOS).point(lambda v:0 if v<150 else 255)
out.save(__import__('os').path.join(__import__('os').path.dirname(__import__('os').path.dirname(__import__('os').path.abspath(__file__))),'art','icons','signal.png'))

"""Draw the Janjan game assets at native resolution using Pillow."""
from pathlib import Path

from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parent
INK = "#30292b"
WOOD = "#87543e"
GRAIN = "#a46a4a"
HONEY = "#ce9361"
CREAM = "#f9edcc"
PAPER = "#e5d1a5"
TAN = "#ba9872"
WINE = "#834951"
ROSE = "#d78676"
MOSS = "#607250"
LEAF = "#9ca668"
GOLD = "#f4c363"
LIGHT = "#fff0ab"


def canvas(w, h):
    img = Image.new("RGBA", (w, h))
    return img, ImageDraw.Draw(img)


def cut(d, box, fill, outline=INK, c=2):
    """A clipped rectangle with a single-pixel outline."""
    x, y, r, b = box
    d.polygon([(x+c,y),(r-c,y),(r,y+c),(r,b-c),
               (r-c,b),(x+c,b),(x,b-c),(x,y+c)], fill, outline)


def line(d, x, y, r, b, color):
    d.line((x, y, r, b), fill=color)


def stitches(d, y, start, stop, color, step=6):
    for x in range(start, stop, step):
        line(d, x, y, x+2, y, color)


def sprig(d, x, y, side=1):
    """A pressed flower, anchored at the lower end of its stem."""
    def p(dx, dy):
        return (x+side*dx, y+dy)
    d.line([p(0,0),p(4,-5),p(6,-12)], MOSS)
    d.polygon([p(2,-2),p(-2,-4),p(-3,-7),p(1,-6),p(4,-4)], LEAF)
    d.polygon([p(4,-5),p(6,-8),p(10,-9),p(9,-6)], MOSS)
    d.polygon([p(5,-15),p(7,-15),p(8,-13),p(10,-13),p(10,-11),
               p(8,-10),p(8,-8),p(6,-8),p(5,-10),p(3,-10),
               p(2,-12),p(3,-14),p(5,-13)], ROSE)
    d.rectangle([p(6,-12)[0], y-12, p(6,-12)[0]+1, y-11], GOLD)


def table():
    img, d = canvas(224, 224)
    cut(d, (0,0,223,223), WOOD, c=5)
    for y in (2, 221):
        line(d, 5,y,218,y,HONEY if y==2 else GRAIN)
    for x in (2, 221):
        line(d,x,5,x,218,HONEY if x==2 else GRAIN)
    for i in range(15):
        k = 11+i*14
        n = 5+i%7
        line(d,k,4+i%3,k+n,4+i%3,GRAIN)
        line(d,k,217+i%3,k+n,217+i%3,GRAIN)
        line(d,4+i%3,k,4+i%3,k+n,GRAIN)
        line(d,217+i%3,k,217+i%3,k+n,GRAIN)
    cut(d,(8,8,215,215),"#84916f", c=6)
    cut(d,(12,12,211,211),"#a5af8a","#647655",c=5)
    d.rectangle((20,20,203,203), "#bbc4a1")
    for y in range(21,204,4):
        for x in range(20+(y//4%3),203,6):
            line(d,x,y,x+1,y,"#b7c09d")
    for y in (15,208):
        stitches(d,y,24,199,CREAM)
    for y in range(24,200,6):
        for x in (15,208):
            line(d,x,y,x,y+2,CREAM)
    for x in (4,218):
        for y in (4,218):
            d.point((x,y), LIGHT)
    return img


def tile(size, back):
    img, d = canvas(size, size)
    r = size-1
    cut(d,(0,0,r,r),WOOD if back else TAN)
    cut(d,(0,0,r,r-2),WINE if back else PAPER)
    if not back:
        d.rectangle((2,2,r-2,r-4),CREAM)
        line(d,2,2,r-2,2,"#fff6dc")
        line(d,2,r-3,r-2,r-3,TAN)
        return img
    line(d,2,3,2,r-5,ROSE)
    for y in range(4,r-4,4 if size==32 else 3):
        line(d,1,y,3,y,PAPER)
    if size==32:
        cut(d,(6,5,27,25),PAPER,WOOD,c=3)
        d.polygon([(20,4),(27,4),(27,9),(21,9)], "#e6ae78")
        sprig(d,10,23)
        d.point((23,18),WINE)
        d.point((22,21),TAN)
        d.point((9,10),TAN)
    else:
        cut(d,(4,2,13,12),PAPER,WOOD,c=1)
        d.line([(6,10),(8,8),(9,5)], MOSS)
        d.line([(6,7),(8,9)], LEAF)
        d.line([(8,8),(11,7)], MOSS)
        d.line([(7,5),(11,5)], ROSE)
        d.line([(9,3),(9,7)], ROSE)
        d.point((9,5),GOLD)
    return img


def selection():
    img, d = canvas(36,36)
    for x, sx in ((0,1),(35,-1)):
        for y, sy in ((0,1),(35,-1)):
            def p(dx,dy):
                return (x+sx*dx,y+sy*dy)
            d.polygon([p(0,2),p(2,0),p(10,0),p(10,3),p(4,3),
                       p(3,4),p(3,10),p(0,10)], GOLD, INK)
            d.line([p(1,8),p(1,3),p(3,1),p(8,1)], LIGHT)
    return img


def panel():
    img, d = canvas(448,96)
    cut(d,(0,0,447,95),WINE,c=5)
    cut(d,(3,3,444,92),WOOD,TAN,c=4)
    cut(d,(6,6,441,89),(38,31,30,184),INK,c=3)
    # Rails are uniform between the 12 px corner slices.
    for x in (8,439):
        for y in (9,86):
            d.point((x,y),GOLD)
    return img


def buttons():
    strip = Image.new("RGBA",(192,24))
    states = [(WOOD,PAPER,CREAM,TAN), (WINE,ROSE,LIGHT,GOLD),
              ("#585351","#807771","#aaa096","#6e665f")]
    for i, (base,face,high,thread) in enumerate(states):
        img,d = canvas(64,24)
        cut(d,(0,0,63,23),base,c=4)
        cut(d,(1,1,62,20),face,base,c=3)
        line(d,5,2,58,2,high)
        line(d,6,19,57,19,thread)
        for x in (5,58):
            d.line([(x,8),(x,13)],base)
            d.point((x,7),high)
        strip.alpha_composite(img,(64*i,0))
    return strip


def nameplate():
    img,d = canvas(92,14)
    cut(d,(0,0,91,13),WINE,c=4)
    cut(d,(3,1,88,11),PAPER,WOOD,c=2)
    line(d,9,2,82,2,CREAM)
    for x in (7,84):
        d.polygon([(x,4),(x+2,6),(x,8),(x-2,6)],WINE)
        d.point((x,6),GOLD)
    return img


def riverbox():
    img,d = canvas(92,78)
    cut(d,(0,0,91,77),WOOD,c=4)
    cut(d,(2,2,89,75),TAN,c=3)
    cut(d,(5,5,86,72),"#6f7b62",c=3)
    for y in (3,74):
        stitches(d,y,9,82,CREAM)
    line(d,6,10,6,67,"#899274")
    line(d,85,10,85,67,"#58644f")
    return img


def seatmark():
    img,d = canvas(26,26)
    cut(d,(1,1,24,24),(120,120,120,125),"#555555",c=6)
    cut(d,(3,3,22,22),(179,179,179,156),"#dddddd",c=5)
    d.polygon([(12,6),(13,6),(19,12),(16,12),(16,17),(9,17),
               (9,12),(6,12)],"#eeeeee","#666666")
    line(d,10,19,15,19,"#eeeeee")
    return img


def banner():
    img,d = canvas(200,48)
    for x, s in ((0,1),(199,-1)):
        d.polygon([(x,12),(x+s*30,12),(x+s*30,45),
                   (x,45),(x+s*7,29)], WINE, INK)
        line(d,x+s*8,43,x+s*18,43,ROSE)
    cut(d,(13,2,186,40),WOOD,c=5)
    cut(d,(16,4,183,36),PAPER,c=4)
    cut(d,(20,7,179,33),CREAM,TAN,c=3)
    for y in (5,35):
        stitches(d,y,27,172,ROSE)
    sprig(d,23,30)
    sprig(d,176,30,-1)
    return img


def turn():
    img,d = canvas(16,16)
    d.polygon([(1,4),(5,4),(5,1),(7,1),(14,7),(14,8),
               (7,14),(5,14),(5,11),(1,11)],GOLD,INK)
    d.line([(2,5),(6,5),(6,2),(11,6)],LIGHT)
    d.line([(6,12),(12,7)],HONEY)
    return img


def main():
    assets = {
        "table": table(), "tile_front": tile(32,False),
        "tile_front16": tile(16,False), "tile_back": tile(32,True),
        "tile_back16": tile(16,True), "tile_sel32": selection(),
        "panel": panel(), "btn": buttons(), "nameplate": nameplate(),
        "riverbox": riverbox(), "seatmark": seatmark(), "banner": banner(),
        "turn": turn(),
    }
    # Native-size contact sheet on dark green for in-game contrast.
    sheet = Image.new("RGBA",(496,384),"#223c32")
    positions = {
        "table": (24,24), "banner": (272,26), "riverbox": (272,92),
        "nameplate": (380,94), "seatmark": (380,129), "turn": (423,134),
        "tile_front": (274,178), "tile_back": (319,178),
        "tile_sel32": (272,176), "tile_front16": (370,186),
        "tile_back16": (397,186), "btn": (276,224), "panel": (24,264),
    }
    for name, position in positions.items():
        sheet.alpha_composite(assets[name], position)
    assets["sheet"] = sheet
    for name, img in assets.items():
        img.save(OUT / f"jan_{name}.png")


if __name__ == "__main__":
    main()

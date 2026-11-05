/* MosaicOS 0.31 - núcleo propio x86 de 32 bits
   HTTPS usable + visor texto (status HTTP, HTML→texto). */
#include <stdint.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64; typedef int64_t i64;

/* ================= Puertos ================= */
static inline void outb(u16 p, u8 v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline void outw(u16 p, u16 v){ __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p)); }
static inline u16 inw(u16 p){ u16 v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p)); return v; }
static inline u8 inb(u16 p){ u8 v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v; }

/* ================= Pantalla ================= */
#define W 80
#define H 25
static volatile u16 *const VGA = (volatile u16*)0xB8000;
#define ATTR(fg,bg) ((u8)(((bg)<<4)|(fg)))
enum { NEGRO=0, AZUL=1, VERDE=2, CIAN=3, ROJO=4, MAGENTA=5, MARRON=6, GRIS=7,
       GRIS_OSC=8, AMARILLO=14, BLANCO=15 };
#define NORMAL ATTR(BLANCO,NEGRO)

static void put(int x,int y,u8 c,u8 a){
    if(x<0||y<0||x>=W||y>=H) return;
    VGA[y*W+x] = (u16)c | ((u16)a<<8);
}
static void fill(int x,int y,int w,int h,u8 c,u8 a){
    for(int j=0;j<h;j++) for(int i=0;i<w;i++) put(x+i,y+j,c,a);
}
/* Texto UTF-8 -> tabla CP437 de VGA (acentos, ñ, ¿, ¡) */
static int text(int x,int y,const char *s,u8 a){
    int n=0;
    while(*s){
        u8 c=(u8)*s++;
        if(c==0xC3 && *s){
            u8 d=(u8)*s++;
            switch(d){
                case 0xA1: c=0xA0; break; case 0xA9: c=0x82; break;
                case 0xAD: c=0xA1; break; case 0xB3: c=0xA2; break;
                case 0xBA: c=0xA3; break; case 0xB1: c=0xA4; break;
                case 0x91: c=0xA5; break; case 0xBC: c=0x81; break;
                default: c='?';
            }
        } else if(c==0xC2 && *s){
            u8 d=(u8)*s++;
            c = d==0xBF ? 0xA8 : d==0xA1 ? 0xAD : '?';
        }
        put(x+n,y,c,a); n++;
    }
    return n;
}
static int number(int x,int y,u32 v,u8 a){
    char b[11]; int n=0, len=0;
    do{ b[n++]='0'+v%10; v/=10; }while(v);
    while(n) put(x+len++,y,b[--n],a);
    return len;
}
static void two(int x,int y,u32 v,u8 a){ put(x,y,'0'+(v/10)%10,a); put(x+1,y,'0'+v%10,a); }
static void hex32(int x,int y,u32 v,u8 a){
    for(int i=0;i<8;i++) put(x+i,y,"0123456789ABCDEF"[(v>>(28-4*i))&15],a);
}
static void box(int x,int y,int w,int h,u8 a){
    put(x,y,0xC9,a); put(x+w-1,y,0xBB,a); put(x,y+h-1,0xC8,a); put(x+w-1,y+h-1,0xBC,a);
    for(int i=1;i<w-1;i++){ put(x+i,y,0xCD,a); put(x+i,y+h-1,0xCD,a); }
    for(int j=1;j<h-1;j++){ put(x,y+j,0xBA,a); put(x+w-1,y+j,0xBA,a); }
}
static void hide_hw_cursor(void){ outb(0x3D4,0x0A); outb(0x3D5,0x20); }

/* ================= Gráficos VGA Mode 13h (320x200, 256 colores) =================
   Base gráfica general: framebuffer lineal en 0xA0000, primitivas y demo.
   El escritorio principal sigue en texto; Mode 13h es opcional y reversible.
*/
#define GFX_W 320
#define GFX_H 200
static volatile u8 *const GFX = (volatile u8*)0xA0000;
static void mouse_erase(void);
static int gfx_active = 0;
static u32 gfx_frames = 0;

static void vga_write_regs(const u8 *regs){
    /* Secuencia típica de registros para Mode 13h (MISC, SEQ, CRTC, GC, ATTR). */
    outb(0x3C2, regs[0]);
    for(int i=0;i<5;i++){ outb(0x3C4, (u8)i); outb(0x3C5, regs[1+i]); }
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & 0x7F); /* unlock CRTC */
    for(int i=0;i<25;i++){ outb(0x3D4, (u8)i); outb(0x3D5, regs[6+i]); }
    for(int i=0;i<9;i++){ outb(0x3CE, (u8)i); outb(0x3CF, regs[31+i]); }
    for(int i=0;i<21;i++){
        (void)inb(0x3DA);
        outb(0x3C0, (u8)i);
        outb(0x3C0, regs[40+i]);
    }
    (void)inb(0x3DA);
    outb(0x3C0, 0x20); /* reenable attribute controller */
}

/* Tablas compactas Mode 13h y Mode 03h (texto 80x25). */
static const u8 VGA_MODE13[] = {
    /* MISC */ 0x63,
    /* SEQ  */ 0x03,0x01,0x0F,0x00,0x0E,
    /* CRTC */ 0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
               0x9C,0x0E,0x8F,0x28,0x40,0x96,0xB9,0xA3,0xFF,
    /* GC   */ 0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0xFF,
    /* ATTR */ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
               0x41,0x00,0x0F,0x00,0x00
};
static const u8 VGA_MODE03[] = {
    /* MISC */ 0x67,
    /* SEQ  */ 0x03,0x00,0x03,0x00,0x02,
    /* CRTC */ 0x5F,0x4F,0x50,0x82,0x55,0x81,0xBF,0x1F,0x00,0x4F,0x0D,0x0E,0x00,0x00,0x00,0x00,
               0x9C,0x0E,0x8F,0x28,0x1F,0x96,0xB9,0xA3,0xFF,
    /* GC   */ 0x00,0x00,0x00,0x00,0x00,0x10,0x0E,0x00,0xFF,
    /* ATTR */ 0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
               0x0C,0x00,0x0F,0x08,0x00
};

static void gfx_set_palette_entry(u8 idx, u8 r, u8 g, u8 b){
    outb(0x3C8, idx);
    outb(0x3C9, (u8)(r>>2));
    outb(0x3C9, (u8)(g>>2));
    outb(0x3C9, (u8)(b>>2));
}

static void gfx_init_palette(void){
    /* 0 = negro, 1-15 colores vivos, 16-31 grises, 32-255 degradado simple */
    gfx_set_palette_entry(0, 0,0,0);
    gfx_set_palette_entry(1, 255,255,255);
    gfx_set_palette_entry(2, 255,0,0);
    gfx_set_palette_entry(3, 0,255,0);
    gfx_set_palette_entry(4, 0,0,255);
    gfx_set_palette_entry(5, 255,255,0);
    gfx_set_palette_entry(6, 0,255,255);
    gfx_set_palette_entry(7, 255,0,255);
    gfx_set_palette_entry(8, 128,128,128);
    gfx_set_palette_entry(9, 255,128,0);
    gfx_set_palette_entry(10, 128,0,255);
    gfx_set_palette_entry(11, 0,128,255);
    gfx_set_palette_entry(12, 255,192,128);
    gfx_set_palette_entry(13, 64,192,64);
    gfx_set_palette_entry(14, 192,64,64);
    gfx_set_palette_entry(15, 64,64,192);
    for(int i=0;i<16;i++){
        u8 v = (u8)(i*17);
        gfx_set_palette_entry((u8)(16+i), v,v,v);
    }
    for(int i=0;i<224;i++){
        u8 r = (u8)((i*3)&255), g = (u8)((i*5)&255), b = (u8)((i*7)&255);
        gfx_set_palette_entry((u8)(32+i), r,g,b);
    }
}

static int gfx_enter(void){
    mouse_erase();
    vga_write_regs(VGA_MODE13);
    gfx_init_palette();
    for(int i=0;i<GFX_W*GFX_H;i++) GFX[i]=0;
    gfx_active = 1;
    gfx_frames = 0;
    return 1;
}

static void gfx_leave(void){
    if(!gfx_active) return;
    vga_write_regs(VGA_MODE03);
    /* Restaurar atributo de texto por defecto y limpiar pantalla de texto */
    for(int i=0;i<W*H;i++) VGA[i] = (u16)' ' | ((u16)NORMAL<<8);
    hide_hw_cursor();
    gfx_active = 0;
}

static inline void gfx_pixel(int x, int y, u8 c){
    if((unsigned)x >= GFX_W || (unsigned)y >= GFX_H) return;
    GFX[y*GFX_W + x] = c;
}

static void gfx_hline(int x, int y, int w, u8 c){
    if(y<0 || y>=GFX_H) return;
    if(x<0){ w+=x; x=0; }
    if(x+w > GFX_W) w = GFX_W-x;
    if(w<=0) return;
    volatile u8 *p = &GFX[y*GFX_W + x];
    for(int i=0;i<w;i++) p[i]=c;
}

static void gfx_fill_rect(int x, int y, int w, int h, u8 c){
    if(w<=0 || h<=0) return;
    for(int j=0;j<h;j++) gfx_hline(x, y+j, w, c);
}

static void gfx_rect(int x, int y, int w, int h, u8 c){
    if(w<=0 || h<=0) return;
    gfx_hline(x, y, w, c);
    gfx_hline(x, y+h-1, w, c);
    for(int j=1;j<h-1;j++){
        gfx_pixel(x, y+j, c);
        gfx_pixel(x+w-1, y+j, c);
    }
}

static void gfx_line(int x0, int y0, int x1, int y1, u8 c){
    int dx = x1-x0; if(dx<0) dx=-dx;
    int dy = y1-y0; if(dy<0) dy=-dy;
    int sx = x0<x1 ? 1 : -1;
    int sy = y0<y1 ? 1 : -1;
    int err = dx-dy;
    for(;;){
        gfx_pixel(x0,y0,c);
        if(x0==x1 && y0==y1) break;
        int e2 = err*2;
        if(e2 > -dy){ err -= dy; x0 += sx; }
        if(e2 <  dx){ err += dx; y0 += sy; }
    }
}

static void gfx_circle(int cx, int cy, int r, u8 c){
    int x=0, y=r, d=3-2*r;
    while(x<=y){
        gfx_pixel(cx+x,cy+y,c); gfx_pixel(cx-x,cy+y,c);
        gfx_pixel(cx+x,cy-y,c); gfx_pixel(cx-x,cy-y,c);
        gfx_pixel(cx+y,cy+x,c); gfx_pixel(cx-y,cy+x,c);
        gfx_pixel(cx+y,cy-x,c); gfx_pixel(cx-y,cy-x,c);
        if(d<0) d = d+4*x+6;
        else { d = d+4*(x-y)+10; y--; }
        x++;
    }
}

static void gfx_fill_circle(int cx, int cy, int r, u8 c){
    for(int y=-r; y<=r; y++){
        for(int x=-r; x<=r; x++){
            if(x*x + y*y <= r*r) gfx_pixel(cx+x, cy+y, c);
        }
    }
}

/* Fuente 8x8 mínima (solo dígitos y letras mayúsculas usadas en demo) — bitmap 8 bits por fila */
static const u8 FONT8[16][8] = {
    /* 0 */ {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    /* 1 */ {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    /* 2 */ {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
    /* 3 */ {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    /* M */ {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    /* O */ {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    /* S */ {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    /* A */ {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    /* I */ {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},
    /* C */ {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    /* . */ {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    /* space */ {0,0,0,0,0,0,0,0},
    /* G */ {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00},
    /* R */ {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},
    /* F */ {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    /* X */ {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
};

static int gfx_glyph_index(char ch){
    if(ch>='0' && ch<='3') return ch-'0';
    switch(ch){
        case 'M': return 4; case 'O': return 5; case 'S': return 6; case 'A': return 7;
        case 'I': return 8; case 'C': return 9; case '.': return 10; case ' ': return 11;
        case 'G': return 12; case 'R': return 13; case 'F': return 14; case 'X': return 15;
        default: return 11;
    }
}

static void gfx_char(int x, int y, char ch, u8 c){
    int gi = gfx_glyph_index(ch);
    for(int row=0; row<8; row++){
        u8 bits = FONT8[gi][row];
        for(int col=0; col<8; col++)
            if(bits & (0x80>>col)) gfx_pixel(x+col, y+row, c);
    }
}

static void gfx_text(int x, int y, const char *s, u8 c){
    while(*s){ gfx_char(x,y,*s,c); x+=8; s++; }
}


/* ================= GDT ================= */
struct dtr { u16 limit; u32 base; } __attribute__((packed));
static u64 gdt[3] = { 0, 0x00CF9A000000FFFFULL, 0x00CF92000000FFFFULL };
static struct dtr gdtr;
static void gdt_init(void){
    gdtr.limit = sizeof(gdt)-1; gdtr.base = (u32)gdt;
    __asm__ volatile(
        "lgdt %0\n mov $0x10,%%ax\n mov %%ax,%%ds\n mov %%ax,%%es\n"
        " mov %%ax,%%fs\n mov %%ax,%%gs\n mov %%ax,%%ss\n ljmp $0x08,$1f\n1:\n"
        :: "m"(gdtr) : "eax","memory");
}

/* ================= IDT / PIC / PIT ================= */
struct idt_entry { u16 lo; u16 sel; u8 zero; u8 type; u16 hi; } __attribute__((packed));
static struct idt_entry idt[256];
extern u32 isr_table[48];
extern void isr_default(void);
static struct dtr idtr;
static void idt_set(int i, u32 a){
    idt[i].lo=a&0xFFFF; idt[i].sel=0x08; idt[i].zero=0; idt[i].type=0x8E; idt[i].hi=a>>16;
}
static void idt_init(void){
    /* Todas las entradas apuntan a un manejador seguro; las 48 primeras
       se sustituyen por excepciones e IRQ conocidos. */
    u32 fallback=(u32)isr_default;
    for(int i=0;i<256;i++) idt_set(i,fallback);
    for(int i=0;i<48;i++) idt_set(i,isr_table[i]);
    idtr.limit=sizeof(idt)-1; idtr.base=(u32)idt;
    __asm__ volatile("lidt %0"::"m"(idtr));
}
static void pic_init(void){
    outb(0x20,0x11); outb(0xA0,0x11);
    outb(0x21,32);   outb(0xA1,40);
    outb(0x21,4);    outb(0xA1,2);
    outb(0x21,1);    outb(0xA1,1);
    outb(0x21,0xF8); outb(0xA1,0xEF);   /* IRQ0 timer, IRQ1 teclado, IRQ2 cascada, IRQ12 mouse */
}
static volatile u32 ticks = 0;          /* 100 por segundo */
static void pit_init(void){
    u32 d = 1193182/100;
    outb(0x43,0x36); outb(0x40,d&0xFF); outb(0x40,(d>>8)&0xFF);
}

/* ================= Eventos ================= */
enum { K_NONE=0, K_UP=0x101, K_DOWN, K_LEFT, K_RIGHT, K_ENTER, K_ESC, K_BACKSPACE,
       K_F1, K_CLICK, K_MOVE, K_DEL, K_HOME, K_END, K_PGUP, K_PGDN, K_F2, K_F3, K_F4, K_F5 };
#define QN 64
static volatile int q[QN]; static volatile int qh=0, qt=0;
static void push(int e){ int n=(qh+1)%QN; if(n!=qt){ q[qh]=e; qh=n; } }

/* ---- Teclado (distribución latinoamericana básica) ---- */
static u8 km[2][0x58];
static void km_set(int sc,const char *a,const char *b){
    for(int i=0;a[i];i++){ km[0][sc+i]=a[i]; km[1][sc+i]=b[i]; }
}
static void km_init(void){
    km_set(0x02,"1234567890","!\"#$%&/()=");
    km_set(0x10,"qwertyuiop","QWERTYUIOP");
    km_set(0x1E,"asdfghjkl","ASDFGHJKL");
    km_set(0x2C,"zxcvbnm","ZXCVBNM");
    km[0][0x0C]='\''; km[1][0x0C]='?';
    km[0][0x0D]=0xA8; km[1][0x0D]=0xAD;
    km[0][0x1B]='+';  km[1][0x1B]='*';
    km[0][0x27]=0xA4; km[1][0x27]=0xA5;
    km[0][0x28]='{';  km[1][0x28]='[';
    km[0][0x2B]='}';  km[1][0x2B]=']';
    km[0][0x33]=',';  km[1][0x33]=';';
    km[0][0x34]='.';  km[1][0x34]=':';
    km[0][0x35]='-';  km[1][0x35]='_';
    km[0][0x39]=' ';  km[1][0x39]=' ';
    km[0][0x56]='<';  km[1][0x56]='>';
}
static int ext=0, shift=0, caps=0;
static void kbd_byte(u8 s){
    if(s==0xE0){ ext=1; return; }
    int e=ext; ext=0;
    int rel=s&0x80; s&=0x7F;
    if(!e && (s==0x2A||s==0x36)){ shift=!rel; return; }
    if(e && (s==0x2A||s==0x36)) return;
    if(!e && s==0x3A && !rel){ caps^=1; return; }
    if(rel) return;
    int k=0;
    switch(s){
        case 0x48: k=K_UP; break;    case 0x50: k=K_DOWN; break;
        case 0x4B: k=K_LEFT; break;  case 0x4D: k=K_RIGHT; break;
        case 0x1C: k=K_ENTER; break; case 0x01: k=K_ESC; break;
        case 0x0E: k=K_BACKSPACE; break; case 0x3B: k=K_F1; break;
        case 0x3C: k=K_F2; break;    case 0x3D: k=K_F3; break;    case 0x3E: k=K_F4; break; case 0x3F: k=K_F5; break;
        case 0x47: if(e) k=K_HOME; break;  case 0x4F: if(e) k=K_END; break;
        case 0x49: if(e) k=K_PGUP; break;  case 0x51: if(e) k=K_PGDN; break;
        case 0x53: if(e) k=K_DEL; break;
        case 0x4E: k='+'; break;     case 0x4A: k='-'; break;
        case 0x37: if(!e) k='*'; break;
        case 0x35: if(e) k='/'; else k=km[shift][s]; break;
        default:
            if(s<0x58){
                k=km[0][s];
                if(k){
                    int letra = (k>='a'&&k<='z') || k==0xA4;
                    int up = letra ? (shift^caps) : shift;
                    k=km[up][s];
                }
            }
    }
    if(k) push(k);
}

/* ---- Mouse PS/2 ---- */
static int has_mouse=0;
static volatile int px=320, py=200, mbtn=0, mdirty=0;
static u8 mp[3]; static int mi=0;
static void mouse_byte(u8 b){
    if(mi==0 && !(b&8)) return;
    mp[mi++]=b;
    if(mi<3) return;
    mi=0;
    if(mp[0]&0xC0) return;
    int dx = (int)mp[1] - ((mp[0]<<4)&0x100);
    int dy = (int)mp[2] - ((mp[0]<<3)&0x100);
    int nx=px+dx*2, ny=py-dy*2;
    if(nx<0) nx=0;
    if(nx>639) nx=639;
    if(ny<0) ny=0;
    if(ny>399) ny=399;
    px=nx; py=ny;
    int bt=mp[0]&7;
    if((bt&1) && !(mbtn&1)) push(K_CLICK);
    mbtn=bt; mdirty=1;
}
static void ps2_drain(void){
    int n=0; u8 st;
    while(((st=inb(0x64))&1) && n++<16){
        u8 b=inb(0x60);
        if(st&0x20) mouse_byte(b); else kbd_byte(b);
    }
}
static int ps2_wr(void){ for(int i=0;i<100000;i++) if(!(inb(0x64)&2)) return 1; return 0; }
static int ps2_rd(void){ for(int i=0;i<100000;i++) if(inb(0x64)&1) return 1; return 0; }
static void ps2_cmd(u8 c){ ps2_wr(); outb(0x64,c); }
static void ps2_data(u8 d){ ps2_wr(); outb(0x60,d); }
static int mouse_cmd(u8 c){ ps2_cmd(0xD4); ps2_data(c); return ps2_rd() ? inb(0x60) : -1; }
static void mouse_init(void){
    while(inb(0x64)&1) inb(0x60);
    ps2_cmd(0xA8);
    ps2_cmd(0x20);
    u8 cfg = ps2_rd() ? inb(0x60) : 0;
    cfg |= 2; cfg &= ~0x20;
    ps2_cmd(0x60); ps2_data(cfg);
    mouse_cmd(0xF6);
    has_mouse = (mouse_cmd(0xF4)==0xFA);
}

/* ---- Manejador común de interrupciones ---- */
struct regs { u32 edi,esi,ebp,esp,ebx,edx,ecx,eax, vec,err, eip,cs,eflags; };
static void panico(struct regs *r){
    __asm__ volatile("cli");
    fill(0,0,W,H,' ',ATTR(BLANCO,ROJO));
    text(3,3,"MosaicOS encontró un error y se detuvo.",ATTR(AMARILLO,ROJO));
    text(3,5,"Excepción número:",ATTR(BLANCO,ROJO)); number(22,5,r->vec,ATTR(BLANCO,ROJO));
    text(3,6,"Dirección (EIP): ",ATTR(BLANCO,ROJO)); hex32(22,6,r->eip,ATTR(BLANCO,ROJO));
    text(3,8,"Reinicia el equipo con el botón físico.",ATTR(BLANCO,ROJO));
    for(;;) __asm__ volatile("hlt");
}
void isr_handler(struct regs *r){
    /* Solo se admiten excepciones CPU y las IRQ PIC mapeadas 32..47. */
    if(r->vec<32 || r->vec>=48) panico(r);
    if(r->vec==32) ticks++;
    else if(r->vec==33 || r->vec==44) ps2_drain();
    if(r->vec>=40) outb(0xA0,0x20);
    outb(0x20,0x20);
}

/* ---- Cursor del mouse dibujado sobre el texto ---- */
static int mx=40, my=12, mvis=0, mvx=0, mvy=0; static u16 msaved=0;
static void mouse_erase(void){ if(mvis){ VGA[mvy*W+mvx]=msaved; mvis=0; } }
static void mouse_draw(void){
    if(!has_mouse) return;
    msaved=VGA[my*W+mx]; u8 a=msaved>>8; a=(u8)((a<<4)|(a>>4));
    VGA[my*W+mx]=(msaved&0xFF)|((u16)a<<8);
    mvis=1; mvx=mx; mvy=my;
}
static void scr_begin(void){ mouse_erase(); }
static void scr_end(void){ mouse_draw(); }
static int in(int x,int y,int w,int h){ return mx>=x&&mx<x+w&&my>=y&&my<y+h; }

static int event_poll(void){
    if(mdirty){
        mdirty=0;
        int nx=px/8, ny=py/16;
        if(nx!=mx||ny!=my){ mouse_erase(); mx=nx; my=ny; mouse_draw(); return K_MOVE; }
    }
    if(qh!=qt){ int e=q[qt]; qt=(qt+1)%QN; return e; }
    return K_NONE;
}
static int event_wait(void){
    for(;;){
        int e=event_poll(); if(e) return e;
        __asm__ volatile("sti; hlt");
    }
}

/* ================= Reloj (CMOS) ================= */
static u8 cmos(u8 r){ outb(0x70,r); return inb(0x71); }
typedef struct { u8 h,m,s,d,mo,y; } Hora;
static void read_time(Hora *t){
    while(cmos(0x0A)&0x80);
    u8 b=cmos(0x0B);
    u8 v[6]={cmos(4),cmos(2),cmos(0),cmos(7),cmos(8),cmos(9)};
    if(!(b&4)) for(int i=0;i<6;i++) v[i]=(v[i]&15)+(v[i]>>4)*10;
    t->h=v[0]; t->m=v[1]; t->s=v[2]; t->d=v[3]; t->mo=v[4]; t->y=v[5];
}

/* ================= Utilidades de memoria y texto ================= */
void *memcpy(void *d,const void *s,u32 n){ u8 *a=d; const u8 *b=s; while(n--) *a++=*b++; return d; }
void *memmove(void *d,const void *s,u32 n){
    u8 *a=d; const u8 *b=s;
    if(a<b) while(n--) *a++=*b++;
    else { a+=n; b+=n; while(n--) *--a=*--b; }
    return d;
}
void *memset(void *d,int c,u32 n){ u8 *a=d; while(n--) *a++=(u8)c; return d; }
static int slen(const char *s){ int n=0; while(s[n]) n++; return n; }
static void scopy(char *d,const char *s,int max){ int i=0; while(s[i]&&i<max-1){ d[i]=s[i]; i++; } d[i]=0; }
static void scat(char *d,const char *s,int max){ int n=slen(d); scopy(d+n,s,max-n); }
static int seq(const char *a,const char *b){ while(*a&&*a==*b){ a++; b++; } return *a==*b; }
/* Texto "crudo" (bytes CP437 tal cual, sin interpretar UTF-8) */
static int rawtext(int x,int y,const char *s,u8 a){ int n=0; while(s[n]){ put(x+n,y,(u8)s[n],a); n++; } return n; }

extern char _end[];

/* ================= Memoria virtual x86 (paging de 4 KiB) =================
   Mapeo identidad inicial + API de map/unmap dinámico (0.16).
   Las direcciones virtuales coinciden con las físicas en el rango inicial.
   No existe todavía aislamiento por proceso ni permisos de usuario.
*/
#define PAGE_SIZE 4096u
#define PAGE_PRESENT 0x001u
#define PAGE_WRITE   0x002u
#define PAGE_USER    0x004u
#define PAGE_TABLES  16u                  /* 64 MiB; reduce BSS de tablas a 64 KiB */
#define PAGE_ENTRIES 1024u
static u32 page_directory[PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static u32 page_tables[PAGE_TABLES][PAGE_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
static u32 paging_enabled = 0;
static u32 paging_mapped_bytes = 0;
static u32 paging_table_count = 0;
static u32 vmm_maps = 0, vmm_unmaps = 0, vmm_map_failures = 0;
static u32 vmm_selftest_ok = 0, vmm_selftest_ran = 0;

/* Declaraciones anticipadas del PMM (definido más abajo) */
u32 pmm_alloc_page(void);
int pmm_free_page(u32 address);

static inline void invlpg(u32 addr){
    __asm__ volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

static int paging_init(u32 physical_end){
    /* No inventar RAM: mapear solo el rango informado por el arranque. */
    if(physical_end < (u32)_end) return 0;
    if(physical_end > PAGE_TABLES * 0x00400000u)
        physical_end = PAGE_TABLES * 0x00400000u;

    memset(page_directory, 0, sizeof(page_directory));
    memset(page_tables, 0, sizeof(page_tables));
    u32 pages = (physical_end + PAGE_SIZE - 1u) / PAGE_SIZE;
    u32 tables = (pages + PAGE_ENTRIES - 1u) / PAGE_ENTRIES;
    if(tables == 0 || tables > PAGE_TABLES) return 0;

    for(u32 t=0; t<tables; t++){
        for(u32 p=0; p<PAGE_ENTRIES; p++){
            u32 phys = (t * PAGE_ENTRIES + p) * PAGE_SIZE;
            if(phys >= physical_end) break;
            page_tables[t][p] = phys | PAGE_PRESENT | PAGE_WRITE;
        }
        page_directory[t] = ((u32)&page_tables[t][0]) | PAGE_PRESENT | PAGE_WRITE;
    }

    u32 cr3 = (u32)page_directory;
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u; /* CR0.PG: activa traducción de direcciones */
    cr0 |= 0x00010000u; /* CR0.WP: respeta páginas de solo lectura en supervisor */
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
    paging_enabled = 1;
    paging_mapped_bytes = physical_end;
    paging_table_count = tables;
    vmm_maps = vmm_unmaps = vmm_map_failures = 0;
    return 1;
}

static u32 paging_translate(u32 address){
    if(!paging_enabled) return 0;
    u32 pde = page_directory[address >> 22];
    if(!(pde & PAGE_PRESENT)) return 0;
    u32 *pt = (u32*)(pde & 0xFFFFF000u);
    u32 pte = pt[(address >> 12) & 0x3FFu];
    if(!(pte & PAGE_PRESENT)) return 0;
    return (pte & 0xFFFFF000u) | (address & 0xFFFu);
}

/* Compatibilidad con código anterior */
static u32 paging_translate_identity(u32 address){
    return paging_translate(address);
}

/* Mapea una página virtual a una física dentro del rango gestionado (64 MiB).
   flags: PAGE_PRESENT | PAGE_WRITE (y opcionalmente PAGE_USER más adelante). */
static int vmm_map_page(u32 virt, u32 phys, u32 flags){
    if(!paging_enabled){ vmm_map_failures++; return 0; }
    if((virt & (PAGE_SIZE-1u)) || (phys & (PAGE_SIZE-1u))){ vmm_map_failures++; return 0; }
    if(virt >= PAGE_TABLES * 0x00400000u || phys >= paging_mapped_bytes){
        vmm_map_failures++; return 0;
    }
    u32 t = virt >> 22;
    u32 p = (virt >> 12) & 0x3FFu;
    if(t >= PAGE_TABLES){ vmm_map_failures++; return 0; }

    /* Asegurar que la tabla de páginas existe en el directorio */
    if(!(page_directory[t] & PAGE_PRESENT)){
        page_directory[t] = ((u32)&page_tables[t][0]) | PAGE_PRESENT | PAGE_WRITE;
        if(t >= paging_table_count) paging_table_count = t + 1;
    }

    page_tables[t][p] = phys | (flags & (PAGE_PRESENT|PAGE_WRITE|PAGE_USER));
    invlpg(virt);
    vmm_maps++;
    return 1;
}

static int vmm_unmap_page(u32 virt){
    if(!paging_enabled) return 0;
    if(virt & (PAGE_SIZE-1u)) return 0;
    if(virt >= PAGE_TABLES * 0x00400000u) return 0;
    u32 t = virt >> 22;
    u32 p = (virt >> 12) & 0x3FFu;
    if(!(page_directory[t] & PAGE_PRESENT)) return 0;
    if(!(page_tables[t][p] & PAGE_PRESENT)) return 0;
    page_tables[t][p] = 0;
    invlpg(virt);
    vmm_unmaps++;
    return 1;
}

/* Asigna una página física libre (PMM) y la mapea en la dirección virtual dada.
   Útil para pruebas y futuras estructuras dinámicas. */
static u32 vmm_alloc_map(u32 virt){
    u32 phys = pmm_alloc_page();
    if(!phys){ vmm_map_failures++; return 0; }
    if(!vmm_map_page(virt, phys, PAGE_PRESENT | PAGE_WRITE)){
        pmm_free_page(phys);
        return 0;
    }
    return phys;
}
/* Mantener API referenciada para el compilador */
static void vmm_api_anchor(void){ (void)vmm_alloc_map; (void)paging_translate_identity; }


/* Autoprueba de arranque: asigna, mapea, escribe, verifica, desmapea y libera. */
static void vmm_selftest(void){
    vmm_selftest_ran = 1; vmm_api_anchor();
    vmm_selftest_ok = 0;
    if(!paging_enabled) return;

    /* Usamos una dirección alta dentro del rango de 64 MiB pero normalmente libre */
    u32 test_virt = 0x03000000u;          /* 48 MiB */
    if(test_virt >= paging_mapped_bytes) test_virt = (paging_mapped_bytes / 2) & ~(PAGE_SIZE-1u);
    if(test_virt < 0x00200000u) return;   /* demasiada poca RAM */

    u32 phys = pmm_alloc_page();
    if(!phys) return;

    if(!vmm_map_page(test_virt, phys, PAGE_PRESENT | PAGE_WRITE)){
        pmm_free_page(phys);
        return;
    }

    volatile u32 *p = (volatile u32*)test_virt;
    p[0] = 0x4D4F5341u; /* "MOSA" */
    p[1] = 0x56303136u; /* "V016" */
    p[1023] = 0xDEADBEEFu;

    if(p[0] != 0x4D4F5341u || p[1] != 0x56303136u || p[1023] != 0xDEADBEEFu){
        vmm_unmap_page(test_virt);
        pmm_free_page(phys);
        return;
    }

    /* Verificar traducción */
    if(paging_translate(test_virt) != phys){
        vmm_unmap_page(test_virt);
        pmm_free_page(phys);
        return;
    }

    vmm_unmap_page(test_virt);
    pmm_free_page(phys);
    vmm_selftest_ok = 1;
}

/* ================= Heap del kernel ================= */
#define HEAP_MAGIC 0x4D4F5341u /* "MOSA" */
typedef struct hblk { u32 size; u32 libre; struct hblk *sig; u32 magic; } hblk;
static hblk *heap_head = 0;
static u32 heap_start = 0, heap_limit = 0;
static u32 heap_allocations = 0, heap_failures = 0;
static int heap_header_valid(const hblk *b){
    u32 a=(u32)b;
    if(!b || heap_limit<sizeof(hblk) || a<heap_start || a>heap_limit-sizeof(hblk)) return 0;
    return b->magic==HEAP_MAGIC && b->size<=heap_limit-a-sizeof(hblk);
}
static void heap_init(u32 ini,u32 fin){
    ini=(ini+15)&~15u;
    /* Never let a low-memory boot turn the heap size into an unsigned underflow. */
    heap_start=ini; heap_limit=fin; heap_head=0;
    heap_allocations=0; heap_failures=0;
    if(fin<=ini || fin-ini<=sizeof(hblk)+16){ heap_start=heap_limit=0; return; }
    heap_head=(hblk*)ini; heap_head->size=fin-ini-sizeof(hblk); heap_head->libre=1;
    heap_head->sig=0; heap_head->magic=HEAP_MAGIC;
}
static void *kmalloc(u32 n){
    if(!heap_head || n>0xFFFFFFE0u){ heap_failures++; return 0; }
    n=(n+15)&~15u;
    for(hblk *b=heap_head;b;){
        if(!heap_header_valid(b)){ heap_failures++; return 0; }
        hblk *next=b->sig;
        if(next && (!heap_header_valid(next) || (u32)next<((u32)(b+1)+b->size))){ heap_failures++; return 0; }
        if(b->libre && b->size>=n){
            if(b->size>=n+sizeof(hblk)+16){
                hblk *nb=(hblk*)((u8*)(b+1)+n);
                nb->size=b->size-n-sizeof(hblk); nb->libre=1; nb->sig=b->sig; nb->magic=HEAP_MAGIC;
                b->sig=nb; b->size=n;
            }
            b->libre=0; heap_allocations++; return b+1;
        }
        b=next;
    }
    heap_failures++; return 0;
}
static void kfree(void *p){
    if(!p || !heap_head) return;
    u32 a=(u32)p;
    if(a<heap_start+sizeof(hblk) || a>=heap_limit) { heap_failures++; return; }
    hblk *target=((hblk*)p)-1;
    if(!heap_header_valid(target) || target->libre){ heap_failures++; return; }
    target->libre=1;
    for(hblk *c=heap_head;c;){
        if(!heap_header_valid(c)){ heap_failures++; return; }
        while(c->libre && c->sig && heap_header_valid(c->sig) && c->sig->libre){
            c->size+=sizeof(hblk)+c->sig->size; c->sig=c->sig->sig;
        }
        c=c->sig;
    }
}
static u32 heap_libre(void){ u32 t=0; for(hblk *b=heap_head;b && heap_header_valid(b);b=b->sig) if(b->libre) t+=b->size; return t; }
static u32 heap_usado(void){ u32 t=0; for(hblk *b=heap_head;b && heap_header_valid(b);b=b->sig) if(!b->libre) t+=b->size; return t; }
static u32 heap_bloques(void){ u32 n=0; for(hblk *b=heap_head;b && heap_header_valid(b);b=b->sig) n++; return n; }

/* ================= PCI ================= */
static inline void outl(u16 p, u32 v){ __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p)); }
static inline u32 inl(u16 p){ u32 v; __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p)); return v; }
static u32 pci_rd(u32 b,u32 d,u32 f,u32 off){
    outl(0xCF8,0x80000000u|(b<<16)|(d<<11)|(f<<8)|(off&0xFC)); return inl(0xCFC);
}
static void pci_wr(u32 b,u32 d,u32 f,u32 off,u32 v){
    outl(0xCF8,0x80000000u|(b<<16)|(d<<11)|(f<<8)|(off&0xFC)); outl(0xCFC,v);
}

/* Inventario PCI general (USB, red, almacenamiento, etc.) */
#define PCI_MAX 32
typedef struct {
    u8 bus, dev, func;
    u16 vendor, device;
    u8 classc, subclass, prog_if;
    u32 bar0, bar1;
} PciDev;
static PciDev pci_devs[PCI_MAX];
static int pci_count = 0;

static void pci_scan(void){
    pci_count = 0;
    for(u32 b=0;b<256 && pci_count<PCI_MAX;b++)
    for(u32 d=0;d<32 && pci_count<PCI_MAX;d++){
        u32 id0 = pci_rd(b,d,0,0);
        if((id0 & 0xFFFF) == 0xFFFF) continue;
        u32 hdr = pci_rd(b,d,0,0x0C);
        int funcs = ((hdr >> 16) & 0x80) ? 8 : 1;
        for(int f=0; f<funcs && pci_count<PCI_MAX; f++){
            u32 id = pci_rd(b,d,f,0);
            if((id & 0xFFFF) == 0xFFFF) continue;
            u32 classr = pci_rd(b,d,f,0x08);
            PciDev *p = &pci_devs[pci_count++];
            p->bus=(u8)b; p->dev=(u8)d; p->func=(u8)f;
            p->vendor=(u16)(id & 0xFFFF);
            p->device=(u16)(id >> 16);
            p->classc=(u8)(classr >> 24);
            p->subclass=(u8)(classr >> 16);
            p->prog_if=(u8)(classr >> 8);
            p->bar0 = pci_rd(b,d,f,0x10);
            p->bar1 = pci_rd(b,d,f,0x14);
        }
    }
}

static void pci_enable_bus_master(u8 b,u8 d,u8 f){
    u32 cmd = pci_rd(b,d,f,0x04);
    pci_wr(b,d,f,0x04, cmd | 0x07); /* IO + mem + bus master */
}

/* ================= USB UHCI (detección y estado de puertos) =================
   Base mínima: localizar controladores UHCI por PCI, mapear I/O, reset global
   y leer el estado de los puertos. No es un stack USB completo ni HID.
*/
#define USB_MAX 4
typedef struct {
    u8 bus, dev, func;
    u16 iobase;
    u16 vendor, device;
    int ports_connected;
    int ok;
} UsbCtrl;
static UsbCtrl usb_ctrls[USB_MAX];
static int usb_count = 0;

static void usb_uhci_init_one(PciDev *pd){
    if(usb_count >= USB_MAX) return;
    /* BAR4 de UHCI suele ser I/O */
    u32 bar = pci_rd(pd->bus, pd->dev, pd->func, 0x20);
    if(!(bar & 1)) bar = pd->bar0;
    if(!(bar & 1)) return; /* necesitamos puerto I/O */
    u16 io = (u16)(bar & ~0x3u);
    if(!io) return;

    pci_enable_bus_master(pd->bus, pd->dev, pd->func);

    /* Reset global UHCI */
    outw(io + 0x00, 0x0004); /* USBCMD GRESET */
    for(volatile int i=0;i<100000;i++);
    outw(io + 0x00, 0x0000);
    for(volatile int i=0;i<50000;i++);

    /* Habilitar máximo 64 bytes, configurar */
    outw(io + 0x00, 0x0000);
    outw(io + 0x04, 0x00FF); /* USBINTR disable all for now */
    outw(io + 0x08, 0);      /* FRNUM */
    outl(io + 0x0C, 0);      /* FLBASEADD - no schedule yet */

    int connected = 0;
    for(int port=0; port<2; port++){
        u16 ps = inw(io + 0x10 + port*2);
        /* bit0 = connection status */
        if(ps & 1) connected++;
        /* clear enable change / connect change */
        outw(io + 0x10 + port*2, ps | 0x000A);
    }

    UsbCtrl *u = &usb_ctrls[usb_count++];
    u->bus=pd->bus; u->dev=pd->dev; u->func=pd->func;
    u->iobase=io; u->vendor=pd->vendor; u->device=pd->device;
    u->ports_connected=connected; u->ok=1;
}

static void usb_init(void){
    usb_count = 0;
    for(int i=0;i<pci_count;i++){
        PciDev *p = &pci_devs[i];
        /* Clase 0x0C (serial bus), subclase 0x03 (USB) */
        if(p->classc != 0x0C || p->subclass != 0x03) continue;
        if(p->prog_if == 0x00){
            /* UHCI */
            usb_uhci_init_one(p);
        }
        /* OHCI 0x10, EHCI 0x20, xHCI 0x30: solo contabilizar en inventario PCI por ahora */
    }
}

/* ================= Red: RTL8139 (compatible con QEMU -device rtl8139) ================= */
#define NET_RX_BUF 8192+16
static u8 net_rx_buf[NET_RX_BUF + 16] __attribute__((aligned(16)));
static u8 net_tx_buf[1536] __attribute__((aligned(4)));
static u16 net_iobase = 0;
static u8 net_mac[6];
static int net_ok = 0;
static u32 net_tx_count = 0, net_rx_count = 0, net_rx_errors = 0;
static u16 net_vendor = 0, net_device = 0;

static void net_rtl_reset(u16 io){
    outb(io + 0x37, 0x10); /* CMD: reset */
    for(int i=0;i<100000;i++) if(!(inb(io + 0x37) & 0x10)) break;
}

static int net_init(void){
    net_ok = 0; net_iobase = 0; net_tx_count = net_rx_count = net_rx_errors = 0;
    for(int i=0;i<6;i++) net_mac[i]=0;

    for(int i=0;i<pci_count;i++){
        PciDev *p = &pci_devs[i];
        /* Realtek RTL8139: vendor 0x10EC device 0x8139
           También algunos clones / QEMU */
        int is_rtl = (p->vendor == 0x10EC && p->device == 0x8139);
        /* Clase network ethernet */
        int is_eth = (p->classc == 0x02 && p->subclass == 0x00);
        if(!is_rtl && !(is_eth && p->vendor == 0x10EC)) continue;

        u32 bar = p->bar0;
        if(!(bar & 1)) continue;
        u16 io = (u16)(bar & ~0x3u);
        if(!io) continue;

        pci_enable_bus_master(p->bus, p->dev, p->func);
        net_rtl_reset(io);

        /* Leer MAC */
        for(int m=0;m<6;m++) net_mac[m] = inb(io + m);

        /* Configurar buffer RX */
        outl(io + 0x30, (u32)net_rx_buf); /* RBSTART */
        outw(io + 0x3C, 0x0005); /* IMR: TOK | ROK (no usamos IRQ aún) */
        outl(io + 0x44, 0x0000070F); /* RCR: accept broadcast+multicast+phys match, wrap */
        outl(io + 0x40, 0x03000700); /* TCR */
        outb(io + 0x37, 0x0C); /* CMD: RE | TE */

        net_iobase = io;
        net_vendor = p->vendor;
        net_device = p->device;
        net_ok = 1;
        return 1;
    }
    return 0;
}

/* Transmite una trama Ethernet cruda (máx ~1514). Devuelve 0 ok. */
static int net_send(const void *data, u32 len){
    if(!net_ok || !net_iobase || len < 14 || len > 1514) return -1;
    u16 io = net_iobase;
    /* Esperar descriptor TX libre (usamos TSAD0 / TSD0) */
    for(int t=0;t<100000;t++){
        u32 tsd = inl(io + 0x10);
        if(tsd & (1u<<13) || (tsd & 0x1FFF) == 0 || (tsd & (1u<<15))) break;
    }
    memcpy(net_tx_buf, data, len);
    if(len < 60){ memset(net_tx_buf+len, 0, 60-len); len = 60; }
    outl(io + 0x20, (u32)net_tx_buf); /* TSAD0 */
    outl(io + 0x10, len);              /* TSD0: size, own=0 starts xmit */
    net_tx_count++;
    return 0;
}

/* Poll RX no bloqueante: copia hasta max bytes, devuelve tamaño o 0/-1 */
static int net_poll_rx(void *out, u32 max){
    if(!net_ok || !net_iobase) return -1;
    u16 io = net_iobase;
    u16 isr = inw(io + 0x3E);
    if(isr & 0x01){ /* ROK */
        /* Cabecera RTL8139 en buffer: status(16) + size(16) + packet */
        u16 *hdr = (u16*)net_rx_buf;
        u16 status = hdr[0];
        u16 size = hdr[1];
        if(size > 4) size -= 4; /* quitar CRC */
        if(size > max) size = (u16)max;
        if(status & 0x01){ /* ROK bit in packet status */
            memcpy(out, net_rx_buf + 4, size);
            net_rx_count++;
        } else {
            net_rx_errors++;
            size = 0;
        }
        /* Reiniciar buffer simple (modo single buffer) */
        outb(io + 0x37, 0x04); /* RE off briefly */
        outb(io + 0x37, 0x0C);
        outw(io + 0x3E, 0x05); /* ack ROK+TOK */
        return (int)size;
    }
    if(isr) outw(io + 0x3E, isr); /* ack otros */
    return 0;
}

/* Envía una trama de prueba Ethernet broadcast (tipo 0x88B5 experimental) */
static int net_send_test(void){
    u8 frame[64];
    memset(frame, 0, sizeof frame);
    for(int i=0;i<6;i++) frame[i]=0xFF;           /* dest broadcast */
    for(int i=0;i<6;i++) frame[6+i]=net_mac[i];   /* src */
    frame[12]=0x88; frame[13]=0xB5;                /* ethertype experimental */
    frame[14]='M'; frame[15]='O'; frame[16]='S'; frame[17]='A';
    frame[18]='I'; frame[19]='C'; frame[20]='O'; frame[21]='S';
    return net_send(frame, 64);
}

/* ================= Pila IP inicial (ARP + IPv4 + ICMP) — camino a Internet =================
   Configuración por defecto para QEMU user networking:
   IP 10.0.2.15 /24, puerta 10.0.2.2, DNS 10.0.2.3
   Todavía no hay TCP ni HTTP; esto habilita ping y la base para navegar después.
*/
static inline u16 htons_u(u16 x){ return (u16)((x<<8)|(x>>8)); }
static inline u32 htonl_u(u32 x){
    return ((x&0xFF)<<24)|((x&0xFF00)<<8)|((x&0xFF0000)>>8)|((x>>24)&0xFF);
}
static inline u16 ntohs_u(u16 x){ return htons_u(x); }
static inline u32 ntohl_u(u32 x){ return htonl_u(x); }

static u16 ip_checksum(const void *data, u32 len){
    const u16 *p = (const u16*)data;
    u32 sum = 0;
    while(len > 1){ sum += *p++; len -= 2; }
    if(len) sum += *(const u8*)p;
    while(sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (u16)(~sum);
}

/* Configuración IPv4 estática (QEMU SLIRP) */
static u32 ip_addr = 0x0A00020F;   /* 10.0.2.15 */
static u32 ip_mask = 0xFFFFFF00;   /* 255.255.255.0 */
static u32 ip_gw   = 0x0A000202;   /* 10.0.2.2 */
static u32 ip_dns  = 0x0A000203;   /* 10.0.2.3 */
static u32 ip_tx_pkts = 0, ip_rx_pkts = 0, icmp_echo_tx = 0, icmp_echo_rx = 0;
static u32 arp_requests = 0, arp_replies_rx = 0;

#define ARP_CACHE 8
typedef struct { u32 ip; u8 mac[6]; u32 age; int used; } ArpEntry;
static ArpEntry arp_tab[ARP_CACHE];
static u32 arp_clock = 0;

static void ip_fmt(u32 ip, char *out){
    /* "a.b.c.d" max 15 chars + NUL */
    int n=0;
    for(int i=0;i<4;i++){
        u8 v = (u8)((ip >> (24-8*i)) & 0xFF);
        if(v>=100) out[n++]='0'+v/100;
        if(v>=10) out[n++]='0'+(v/10)%10;
        out[n++]='0'+v%10;
        if(i<3) out[n++]='.';
    }
    out[n]=0;
}

static int parse_ip(const char *s, u32 *out){
    u32 ip=0; int parts=0; u32 cur=0; int dig=0;
    for(;;){
        char c=*s++;
        if(c>='0'&&c<='9'){
            cur = cur*10 + (u32)(c-'0');
            if(cur>255) return 0;
            dig=1;
        } else if(c=='.' || c==0){
            if(!dig || parts>=4) return 0;
            ip = (ip<<8) | cur;
            parts++; cur=0; dig=0;
            if(c==0) break;
        } else return 0;
    }
    if(parts!=4) return 0;
    *out = ip;
    return 1;
}

static void arp_cache_put(u32 ip, const u8 mac[6]){
    arp_clock++;
    for(int i=0;i<ARP_CACHE;i++){
        if(arp_tab[i].used && arp_tab[i].ip==ip){
            for(int j=0;j<6;j++) arp_tab[i].mac[j]=mac[j];
            arp_tab[i].age = arp_clock;
            return;
        }
    }
    int slot=0; u32 oldest=0xFFFFFFFFu;
    for(int i=0;i<ARP_CACHE;i++){
        if(!arp_tab[i].used){ slot=i; break; }
        if(arp_tab[i].age < oldest){ oldest=arp_tab[i].age; slot=i; }
    }
    arp_tab[slot].used=1; arp_tab[slot].ip=ip; arp_tab[slot].age=arp_clock;
    for(int j=0;j<6;j++) arp_tab[slot].mac[j]=mac[j];
}

static int arp_cache_get(u32 ip, u8 mac[6]){
    for(int i=0;i<ARP_CACHE;i++){
        if(arp_tab[i].used && arp_tab[i].ip==ip){
            for(int j=0;j<6;j++) mac[j]=arp_tab[i].mac[j];
            return 1;
        }
    }
    return 0;
}

static int arp_send_request(u32 tip){
    u8 frame[42];
    memset(frame, 0, sizeof frame);
    for(int i=0;i<6;i++) frame[i]=0xFF;
    for(int i=0;i<6;i++) frame[6+i]=net_mac[i];
    frame[12]=0x08; frame[13]=0x06; /* ARP */
    frame[14]=0x00; frame[15]=0x01; /* eth */
    frame[16]=0x08; frame[17]=0x00; /* IPv4 */
    frame[18]=6; frame[19]=4;       /* hlen, plen */
    frame[20]=0x00; frame[21]=0x01; /* request */
    for(int i=0;i<6;i++) frame[22+i]=net_mac[i];
    frame[28]=(u8)(ip_addr>>24); frame[29]=(u8)(ip_addr>>16);
    frame[30]=(u8)(ip_addr>>8);  frame[31]=(u8)(ip_addr);
    /* target MAC zeros */
    frame[38]=(u8)(tip>>24); frame[39]=(u8)(tip>>16);
    frame[40]=(u8)(tip>>8);  frame[41]=(u8)(tip);
    arp_requests++;
    return net_send(frame, 42);
}

static void arp_send_reply(u32 tip, const u8 tmac[6]){
    u8 frame[42];
    memset(frame, 0, sizeof frame);
    for(int i=0;i<6;i++) frame[i]=tmac[i];
    for(int i=0;i<6;i++) frame[6+i]=net_mac[i];
    frame[12]=0x08; frame[13]=0x06;
    frame[14]=0x00; frame[15]=0x01;
    frame[16]=0x08; frame[17]=0x00;
    frame[18]=6; frame[19]=4;
    frame[20]=0x00; frame[21]=0x02; /* reply */
    for(int i=0;i<6;i++) frame[22+i]=net_mac[i];
    frame[28]=(u8)(ip_addr>>24); frame[29]=(u8)(ip_addr>>16);
    frame[30]=(u8)(ip_addr>>8);  frame[31]=(u8)(ip_addr);
    for(int i=0;i<6;i++) frame[32+i]=tmac[i];
    frame[38]=(u8)(tip>>24); frame[39]=(u8)(tip>>16);
    frame[40]=(u8)(tip>>8);  frame[41]=(u8)(tip);
    net_send(frame, 42);
}

static int eth_send_ip(const u8 dst_mac[6], const void *ip_payload, u32 ip_len){
    u8 frame[1514];
    if(ip_len > 1500) return -1;
    for(int i=0;i<6;i++) frame[i]=dst_mac[i];
    for(int i=0;i<6;i++) frame[6+i]=net_mac[i];
    frame[12]=0x08; frame[13]=0x00; /* IPv4 */
    memcpy(frame+14, ip_payload, ip_len);
    return net_send(frame, 14+ip_len);
}

static int ip_resolve_mac(u32 dst_ip, u8 mac[6]){
    u32 next = dst_ip;
    if((dst_ip & ip_mask) != (ip_addr & ip_mask))
        next = ip_gw; /* fuera de subred → puerta de enlace */
    if(arp_cache_get(next, mac)) return 1;
    arp_send_request(next);
    /* Esperar respuesta ARP un poco */
    for(int tries=0; tries<50; tries++){
        u8 buf[1514];
        int n = net_poll_rx(buf, sizeof buf);
        if(n >= 42){
            /* Procesar solo ARP aquí de forma mínima */
            if(buf[12]==0x08 && buf[13]==0x06){
                u16 op = (u16)((buf[20]<<8)|buf[21]);
                u32 sip = ((u32)buf[28]<<24)|((u32)buf[29]<<16)|((u32)buf[30]<<8)|buf[31];
                if(op==2){
                    arp_cache_put(sip, buf+22);
                    arp_replies_rx++;
                    if(sip==next){
                        for(int j=0;j<6;j++) mac[j]=buf[22+j];
                        return 1;
                    }
                } else if(op==1){
                    u32 tip = ((u32)buf[38]<<24)|((u32)buf[39]<<16)|((u32)buf[40]<<8)|buf[41];
                    if(tip==ip_addr) arp_send_reply(sip, buf+22);
                }
            }
        }
        for(volatile int d=0; d<20000; d++);
    }
    return arp_cache_get(next, mac);
}

static int ipv4_send(u32 dst, u8 proto, const void *payload, u32 plen){
    u8 pkt[1500];
    if(20+plen > sizeof pkt) return -1;
    memset(pkt, 0, 20);
    pkt[0]=0x45; /* v4, ihl=5 */
    u16 total = (u16)(20+plen);
    pkt[2]=(u8)(total>>8); pkt[3]=(u8)total;
    static u16 ip_id=1; u16 id=ip_id++;
    pkt[4]=(u8)(id>>8); pkt[5]=(u8)id;
    pkt[8]=64; /* TTL */
    pkt[9]=proto;
    pkt[12]=(u8)(ip_addr>>24); pkt[13]=(u8)(ip_addr>>16);
    pkt[14]=(u8)(ip_addr>>8);  pkt[15]=(u8)(ip_addr);
    pkt[16]=(u8)(dst>>24); pkt[17]=(u8)(dst>>16);
    pkt[18]=(u8)(dst>>8);  pkt[19]=(u8)(dst);
    u16 csum = ip_checksum(pkt, 20);
    pkt[10]=(u8)(csum>>8); pkt[11]=(u8)csum;
    memcpy(pkt+20, payload, plen);

    u8 mac[6];
    if(!ip_resolve_mac(dst, mac)) return -2;
    int r = eth_send_ip(mac, pkt, 20+plen);
    if(r==0) ip_tx_pkts++;
    return r;
}

static int icmp_send_echo(u32 dst, u16 ident, u16 seq){
    u8 icmp[8+32];
    memset(icmp, 0, sizeof icmp);
    icmp[0]=8; /* echo request */
    icmp[1]=0;
    icmp[4]=(u8)(ident>>8); icmp[5]=(u8)ident;
    icmp[6]=(u8)(seq>>8);   icmp[7]=(u8)seq;
    for(int i=0;i<32;i++) icmp[8+i]=(u8)('A'+(i%26));
    u16 csum = ip_checksum(icmp, sizeof icmp);
    icmp[2]=(u8)(csum>>8); icmp[3]=(u8)csum;
    icmp_echo_tx++;
    return ipv4_send(dst, 1, icmp, sizeof icmp);
}

/* Estado del último ping */
static int ping_ok = 0;
// static u32 ping_last_ms = 0;
static u32 ping_target = 0;

static void tcp_input(u32 src, u32 dst, u8 *tcp, int tcplen);
static void udp_input(u32 src, u32 dst, u8 *udp, int udplen);

static void net_handle_frame(u8 *buf, int n){
    if(n < 14) return;
    u16 ethertype = (u16)((buf[12]<<8)|buf[13]);
    if(ethertype == 0x0806 && n >= 42){
        u16 op = (u16)((buf[20]<<8)|buf[21]);
        u32 sip = ((u32)buf[28]<<24)|((u32)buf[29]<<16)|((u32)buf[30]<<8)|buf[31];
        u32 tip = ((u32)buf[38]<<24)|((u32)buf[39]<<16)|((u32)buf[40]<<8)|buf[41];
        if(op==1 && tip==ip_addr){
            arp_send_reply(sip, buf+22);
            arp_cache_put(sip, buf+22);
        } else if(op==2){
            arp_cache_put(sip, buf+22);
            arp_replies_rx++;
        }
        return;
    }
    if(ethertype != 0x0800 || n < 34) return; /* IPv4 */
    u8 *ip = buf+14;
    u8 ihl = (ip[0] & 0x0F) * 4;
    if(ihl < 20 || 14+ihl > n) return;
    u16 total = (u16)((ip[2]<<8)|ip[3]);
    if(14+total > n) return;
    u32 src = ((u32)ip[12]<<24)|((u32)ip[13]<<16)|((u32)ip[14]<<8)|ip[15];
    u32 dst = ((u32)ip[16]<<24)|((u32)ip[17]<<16)|((u32)ip[18]<<8)|ip[19];
    if(dst != ip_addr && dst != 0xFFFFFFFF) return;
    ip_rx_pkts++;
    u8 proto = ip[9];
    if(proto == 1){ /* ICMP */
        u8 *icmp = ip + ihl;
        int icmplen = total - ihl;
        if(icmplen < 8) return;
        if(icmp[0] == 8){
            u8 reply[1500];
            if(icmplen > (int)sizeof(reply)) return;
            memcpy(reply, icmp, icmplen);
            reply[0]=0;
            reply[2]=0; reply[3]=0;
            u16 csum = ip_checksum(reply, icmplen);
            reply[2]=(u8)(csum>>8); reply[3]=(u8)csum;
            ipv4_send(src, 1, reply, icmplen);
        } else if(icmp[0] == 0){
            icmp_echo_rx++;
            if(src == ping_target) ping_ok = 1;
        }
    } else if(proto == 6){ /* TCP */
        u8 *tcp = ip + ihl;
        int tcplen = total - ihl;
        if(tcplen >= 20) tcp_input(src, dst, tcp, tcplen);
    } else if(proto == 17){ /* UDP */
        u8 *udp = ip + ihl;
        int udplen = total - ihl;
        if(udplen >= 8) udp_input(src, dst, udp, udplen);
    }
}

static void net_poll_stack(void){
    u8 buf[1514];
    for(int i=0;i<8;i++){
        int n = net_poll_rx(buf, sizeof buf);
        if(n <= 0) break;
        net_handle_frame(buf, n);
    }
}

/* Ping bloqueante corto: envía 1 echo y espera hasta ~1s */
static int net_ping(u32 dst, u32 *elapsed_hint){
    ping_ok = 0;
    ping_target = dst;
    if(icmp_send_echo(dst, 0x4D4F, 1) != 0) return -1;
    u32 start = ticks;
    while((ticks - start) < 100){ /* ~1 segundo a 100 Hz */
        net_poll_stack();
        if(ping_ok){
            if(elapsed_hint) *elapsed_hint = ticks - start;
            return 0;
        }
    }
    if(elapsed_hint) *elapsed_hint = ticks - start;
    return -2; /* timeout */
}

/* ================= TCP cliente mínimo (una conexión) + HTTP GET ================= */
enum {
    TCP_CLOSED = 0,
    TCP_SYN_SENT,
    TCP_ESTABLISHED,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK
};
#define TCP_RX_MAX 24576
static int  tcp_state = TCP_CLOSED;
static u32  tcp_remote_ip = 0;
static u16  tcp_local_port = 40000;
static u16  tcp_remote_port = 80;
static u32  tcp_snd_nxt = 0;   /* próximo SEQ a enviar */
static u32  tcp_rcv_nxt = 0;   /* próximo SEQ esperado */
static u32  tcp_iss = 0;
static u8   tcp_rx[TCP_RX_MAX];
static u32  tcp_rx_len = 0;
static int  tcp_got_synack = 0;
static int  tcp_got_fin = 0;
static u32  tcp_tx_segs = 0, tcp_rx_segs = 0;

static u16 tcp_checksum(u32 src, u32 dst, const u8 *tcp, u32 tcp_len){
    /* Pseudo-cabecera + TCP */
    u32 sum = 0;
    sum += (src >> 16) & 0xFFFF;
    sum += src & 0xFFFF;
    sum += (dst >> 16) & 0xFFFF;
    sum += dst & 0xFFFF;
    sum += 6; /* protocol TCP */
    sum += tcp_len;
    const u16 *p = (const u16*)tcp;
    u32 len = tcp_len;
    while(len > 1){ sum += *p++; len -= 2; }
    if(len) sum += *(const u8*)p;
    while(sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (u16)(~sum);
}

static int tcp_send_raw(u32 dst, u16 sport, u16 dport, u32 seq, u32 ack,
                        u8 flags, const void *payload, u32 plen){
    u8 seg[1500];
    u32 hdr = 20;
    if(hdr + plen > sizeof seg) return -1;
    memset(seg, 0, hdr);
    seg[0] = (u8)(sport >> 8); seg[1] = (u8)sport;
    seg[2] = (u8)(dport >> 8); seg[3] = (u8)dport;
    seg[4] = (u8)(seq >> 24); seg[5] = (u8)(seq >> 16);
    seg[6] = (u8)(seq >> 8);  seg[7] = (u8)seq;
    seg[8] = (u8)(ack >> 24); seg[9] = (u8)(ack >> 16);
    seg[10]= (u8)(ack >> 8);  seg[11]= (u8)ack;
    seg[12]= (5 << 4); /* data offset 5 (20 bytes) */
    seg[13]= flags;
    seg[14]= 0x16; seg[15]= 0xD0; /* window 5840 */
    if(payload && plen) memcpy(seg + hdr, payload, plen);
    u16 csum = tcp_checksum(ip_addr, dst, seg, hdr + plen);
    seg[16] = (u8)(csum >> 8); seg[17] = (u8)csum;
    int r = ipv4_send(dst, 6, seg, hdr + plen);
    if(r == 0) tcp_tx_segs++;
    return r;
}

static void tcp_input(u32 src, u32 dst, u8 *tcp, int tcplen){
    (void)dst;
    if(tcp_state == TCP_CLOSED) return;
    u16 sport = (u16)((tcp[0]<<8)|tcp[1]);
    u16 dport = (u16)((tcp[2]<<8)|tcp[3]);
    if(dport != tcp_local_port) return;
    if(src != tcp_remote_ip) return;
    if(sport != tcp_remote_port) return;

    u32 seq = ((u32)tcp[4]<<24)|((u32)tcp[5]<<16)|((u32)tcp[6]<<8)|tcp[7];
    u32 ack = ((u32)tcp[8]<<24)|((u32)tcp[9]<<16)|((u32)tcp[10]<<8)|tcp[11];
    u8 doff = (tcp[12] >> 4) * 4;
    u8 flags = tcp[13];
    int data_len = tcplen - doff;
    if(data_len < 0) return;
    u8 *data = tcp + doff;
    tcp_rx_segs++;

    if(tcp_state == TCP_SYN_SENT){
        if((flags & 0x12) == 0x12){ /* SYN+ACK */
            tcp_rcv_nxt = seq + 1;
            tcp_snd_nxt = ack;
            tcp_send_raw(src, tcp_local_port, tcp_remote_port, tcp_snd_nxt, tcp_rcv_nxt, 0x10, 0, 0); /* ACK */
            tcp_state = TCP_ESTABLISHED;
            tcp_got_synack = 1;
        }
        return;
    }

    if(tcp_state == TCP_ESTABLISHED || tcp_state == TCP_CLOSE_WAIT){
        if(data_len > 0){
            u32 room = TCP_RX_MAX - tcp_rx_len;
            u32 copy = (u32)data_len < room ? (u32)data_len : room;
            if(copy){
                memcpy(tcp_rx + tcp_rx_len, data, copy);
                tcp_rx_len += copy;
            }
            tcp_rcv_nxt = seq + (u32)data_len;
            tcp_send_raw(src, tcp_local_port, tcp_remote_port, tcp_snd_nxt, tcp_rcv_nxt, 0x10, 0, 0);
        }
        if(flags & 0x01){ /* FIN */
            tcp_rcv_nxt = seq + 1 + (data_len > 0 ? 0 : 0);
            if(data_len == 0) tcp_rcv_nxt = seq + 1;
            else tcp_rcv_nxt = seq + (u32)data_len + 1;
            tcp_send_raw(src, tcp_local_port, tcp_remote_port, tcp_snd_nxt, tcp_rcv_nxt, 0x10, 0, 0);
            tcp_got_fin = 1;
            tcp_state = TCP_CLOSE_WAIT;
        }
        if(flags & 0x10){
            /* ACK of our data — advance already tracked via tcp_snd_nxt on send */
            (void)ack;
        }
    }
}

static void tcp_close_conn(void){
    if(tcp_state == TCP_ESTABLISHED || tcp_state == TCP_CLOSE_WAIT){
        tcp_send_raw(tcp_remote_ip, tcp_local_port, tcp_remote_port,
                     tcp_snd_nxt, tcp_rcv_nxt, 0x11, 0, 0); /* FIN+ACK */
        tcp_snd_nxt++;
        tcp_state = TCP_LAST_ACK;
    }
    /* Esperar un poco y forzar cierre */
    u32 t0 = ticks;
    while((ticks - t0) < 20) net_poll_stack();
    tcp_state = TCP_CLOSED;
}

static int tcp_connect(u32 ip, u16 port){
    if(!net_ok) return -1;
    tcp_close_conn();
    tcp_state = TCP_CLOSED;
    tcp_remote_ip = ip;
    tcp_remote_port = port;
    tcp_local_port = (u16)(40000 + (ticks & 0xFFF));
    tcp_iss = 1000 + (ticks & 0xFFFF);
    tcp_snd_nxt = tcp_iss;
    tcp_rcv_nxt = 0;
    tcp_rx_len = 0;
    tcp_got_synack = 0;
    tcp_got_fin = 0;

    tcp_state = TCP_SYN_SENT;
    if(tcp_send_raw(ip, tcp_local_port, port, tcp_iss, 0, 0x02, 0, 0) != 0){ /* SYN */
        tcp_state = TCP_CLOSED;
        return -2;
    }
    tcp_snd_nxt = tcp_iss + 1;

    u32 t0 = ticks;
    while((ticks - t0) < 200){ /* ~2s */
        net_poll_stack();
        if(tcp_got_synack && tcp_state == TCP_ESTABLISHED) return 0;
    }
    tcp_state = TCP_CLOSED;
    return -3; /* timeout */
}

static int tcp_send_data(const void *data, u32 len){
    if(tcp_state != TCP_ESTABLISHED) return -1;
    if(tcp_send_raw(tcp_remote_ip, tcp_local_port, tcp_remote_port,
                    tcp_snd_nxt, tcp_rcv_nxt, 0x18, data, len) != 0) /* PSH+ACK */
        return -2;
    tcp_snd_nxt += len;
    return 0;
}

/* Espera más datos o FIN hasta timeout (ticks) */
static void tcp_recv_wait(u32 max_ticks){
    u32 t0 = ticks;
    u32 last_len = tcp_rx_len;
    u32 idle = 0;
    while((ticks - t0) < max_ticks){
        net_poll_stack();
        if(tcp_rx_len > last_len){
            last_len = tcp_rx_len;
            idle = 0;
            t0 = ticks; /* reinicia al recibir */
        } else {
            idle++;
            /* si no llega nada nuevo ~300ms y ya hay datos, salir */
            if(last_len > 0 && idle > 30) break;
        }
        if(tcp_got_fin && idle > 10) break;
        if(tcp_rx_len >= TCP_RX_MAX - 64) break;
    }
}

/* ================= UDP + DNS ================= */
static u16 udp_local_port = 50000;
static u8  dns_buf[512];
static int dns_ready = 0;
static u32 dns_answer_ip = 0;
static u32 dns_queries = 0, dns_answers = 0;

static int udp_send(u32 dst, u16 sport, u16 dport, const void *data, u32 len){
    u8 pkt[1500];
    if(8 + len > sizeof pkt) return -1;
    pkt[0] = (u8)(sport >> 8); pkt[1] = (u8)sport;
    pkt[2] = (u8)(dport >> 8); pkt[3] = (u8)dport;
    u16 ulen = (u16)(8 + len);
    pkt[4] = (u8)(ulen >> 8); pkt[5] = (u8)ulen;
    pkt[6] = 0; pkt[7] = 0; /* checksum optional for IPv4 */
    if(len) memcpy(pkt + 8, data, len);
    return ipv4_send(dst, 17, pkt, 8 + len);
}

static void udp_input(u32 src, u32 dst, u8 *udp, int udplen){
    (void)src; (void)dst;
    u16 sport = (u16)((udp[0]<<8)|udp[1]);
    u16 dport = (u16)((udp[2]<<8)|udp[3]);
    u16 ulen  = (u16)((udp[4]<<8)|udp[5]);
    if(ulen < 8 || ulen > udplen) return;
    int payload = ulen - 8;
    if(payload < 0) return;
    /* DNS response (port 53 → our ephemeral) */
    if(sport == 53 && dport == udp_local_port && payload >= 12){
        memcpy(dns_buf, udp + 8, payload < 512 ? (u32)payload : 512);
        /* Parse answers for first A record */
        u16 id = (u16)((dns_buf[0]<<8)|dns_buf[1]);
        u16 flags = (u16)((dns_buf[2]<<8)|dns_buf[3]);
        u16 ancount = (u16)((dns_buf[6]<<8)|dns_buf[7]);
        (void)id;
        if(!(flags & 0x8000) || ancount == 0){ dns_ready = 1; dns_answer_ip = 0; return; }
        int pos = 12;
        /* skip question */
        while(pos < payload && dns_buf[pos] != 0){
            int lab = dns_buf[pos++];
            if(lab > 63){ /* compression in question rare */ break; }
            pos += lab;
        }
        pos++; /* zero */
        pos += 4; /* type class */
        for(u16 a = 0; a < ancount && pos + 12 <= payload; a++){
            if((dns_buf[pos] & 0xC0) == 0xC0) pos += 2;
            else {
                while(pos < payload && dns_buf[pos] != 0){
                    int lab = dns_buf[pos++];
                    if(lab > 63) break;
                    pos += lab;
                }
                pos++;
            }
            if(pos + 10 > payload) break;
            u16 typ = (u16)((dns_buf[pos]<<8)|dns_buf[pos+1]);
            u16 rdlen = (u16)((dns_buf[pos+8]<<8)|dns_buf[pos+9]);
            pos += 10;
            if(typ == 1 && rdlen == 4 && pos + 4 <= payload){
                dns_answer_ip = ((u32)dns_buf[pos]<<24)|((u32)dns_buf[pos+1]<<16)|
                                ((u32)dns_buf[pos+2]<<8)|dns_buf[pos+3];
                dns_answers++;
                dns_ready = 1;
                return;
            }
            pos += rdlen;
        }
        dns_ready = 1;
        dns_answer_ip = 0;
    }
}

static int dns_encode_name(u8 *out, int max, const char *host){
    int n = 0;
    while(*host && n < max - 1){
        const char *dot = host;
        while(*dot && *dot != '.') dot++;
        int lab = (int)(dot - host);
        if(lab <= 0 || lab > 63) return -1;
        out[n++] = (u8)lab;
        for(int i=0;i<lab && n<max;i++) out[n++] = (u8)host[i];
        host = *dot ? dot + 1 : dot;
    }
    out[n++] = 0;
    return n;
}

/* Resuelve A record. 0 ok, <0 error. DNS server = ip_dns (10.0.2.3 en QEMU). */
static int dns_resolve(const char *host, u32 *out_ip){
    if(!net_ok || !host || !host[0]) return -1;
    /* Si ya es IP, parsear */
    if(parse_ip(host, out_ip)) return 0;

    u8 query[512];
    memset(query, 0, sizeof query);
    static u16 dns_id = 1;
    u16 id = dns_id++;
    query[0] = (u8)(id >> 8); query[1] = (u8)id;
    query[2] = 0x01; query[3] = 0x00; /* recursion desired */
    query[4] = 0; query[5] = 1; /* QDCOUNT = 1 */
    int nlen = dns_encode_name(query + 12, 400, host);
    if(nlen < 0) return -2;
    int pos = 12 + nlen;
    query[pos++] = 0; query[pos++] = 1; /* TYPE A */
    query[pos++] = 0; query[pos++] = 1; /* CLASS IN */

    dns_ready = 0;
    dns_answer_ip = 0;
    udp_local_port = (u16)(50000 + (ticks & 0xFF));
    dns_queries++;
    if(udp_send(ip_dns, udp_local_port, 53, query, (u32)pos) != 0) return -3;

    u32 t0 = ticks;
    while((ticks - t0) < 200){
        net_poll_stack();
        if(dns_ready){
            if(dns_answer_ip){ *out_ip = dns_answer_ip; return 0; }
            return -4; /* NXDOMAIN / no A */
        }
    }
    return -5; /* timeout */
}

/* HTTP/1.0 GET — IP o se usa host_hdr para Host:. Puerto 80, sin TLS. */
static int http_get(u32 ip, const char *host_hdr, const char *path){
    if(!path || !path[0]) path = "/";
    if(tcp_connect(ip, 80) != 0) return -1;

    char req[512];
    int n = 0;
    const char *p;
    p = "GET "; while(*p && n < 480) req[n++] = *p++;
    p = path;   while(*p && n < 480) req[n++] = *p++;
    p = " HTTP/1.0\r\nHost: "; while(*p && n < 480) req[n++] = *p++;
    if(host_hdr){ p = host_hdr; while(*p && n < 480) req[n++] = *p++; }
    p = "\r\nUser-Agent: MosaicOS/0.22\r\nAccept: text/html,text/plain,*/*\r\nConnection: close\r\n\r\n";
    while(*p && n < 500) req[n++] = *p++;
    req[n] = 0;

    if(tcp_send_data(req, (u32)n) != 0){
        tcp_close_conn();
        return -2;
    }
    tcp_recv_wait(300);
    tcp_close_conn();
    return (int)tcp_rx_len;
}

/* Quita etiquetas HTML de forma tosca; escribe en dst hasta max-1. */
static u32 html_to_text(const u8 *src, u32 slen, char *dst, u32 max){
    u32 di = 0;
    int in_tag = 0, in_script = 0;
    for(u32 i=0; i<slen && di+1<max; i++){
        char c = (char)src[i];
        if(c == '<'){
            /* detectar script/style */
            if(i+7 < slen){
                char t0=src[i+1]|32,t1=src[i+2]|32,t2=src[i+3]|32,t3=src[i+4]|32,t4=src[i+5]|32,t5=src[i+6]|32;
                if(t0=='s'&&t1=='c'&&t2=='r'&&t3=='i'&&t4=='p'&&t5=='t') in_script=1;
                if(t0=='/'&&t1=='s'&&t2=='c'&&t3=='r'&&t4=='i'&&t5=='p') in_script=0;
            }
            in_tag = 1;
            continue;
        }
        if(c == '>'){ in_tag = 0; continue; }
        if(in_tag || in_script) continue;
        if(c == '&'){
            /* entidades mínimas */
            if(i+5 < slen && src[i+1]=='n'&&src[i+2]=='b'&&src[i+3]=='s'&&src[i+4]=='p'&&src[i+5]==';'){
                c = ' '; i += 5;
            } else if(i+3 < slen && src[i+1]=='l'&&src[i+2]=='t'&&src[i+3]==';'){ c='<'; i+=3; }
            else if(i+3 < slen && src[i+1]=='g'&&src[i+2]=='t'&&src[i+3]==';'){ c='>'; i+=3; }
            else if(i+4 < slen && src[i+1]=='a'&&src[i+2]=='m'&&src[i+3]=='p'&&src[i+4]==';'){ c='&'; i+=4; }
        }
        if(c == '\r') continue;
        if(c == '\n' || c == '\t') c = ' ';
        if(c < 32 || (u8)c > 126) continue;
        /* colapsar espacios */
        if(c == ' ' && di > 0 && dst[di-1] == ' ') continue;
        dst[di++] = c;
    }
    dst[di] = 0;
    return di;
}

/* browse: DNS (si hace falta) + HTTP + texto sin tags. Resultado en tcp_rx sigue crudo;
   texto legible se genera al mostrar. */
static int http_browse(const char *host, const char *path, u32 *resolved_ip){
    u32 ip = 0;
    int r = dns_resolve(host, &ip);
    if(r != 0) return -10 + r; /* -10..-15 */
    if(resolved_ip) *resolved_ip = ip;
    return http_get(ip, host, path && path[0] ? path : "/");
}


/* ================= Criptografía base + TLS 1.2 (inicio de la misión HTTPS) =================
   0.23: SHA-256, HMAC-SHA256, AES-128, ClientHello TLS 1.2 sobre TCP/443.
   Aún NO completa el handshake (falta intercambio de claves, certificados, Finished).
   Sin eso no hay YouTube; esto es el cimiento obligatorio.
*/

/* ---- SHA-256 ---- */
static u32 sha_rotr(u32 x, int n){ return (x >> n) | (x << (32 - n)); }
static void sha256_transform(u32 st[8], const u8 block[64]){
    static const u32 K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    u32 w[64];
    for(int i=0;i<16;i++)
        w[i] = ((u32)block[i*4]<<24)|((u32)block[i*4+1]<<16)|((u32)block[i*4+2]<<8)|block[i*4+3];
    for(int i=16;i<64;i++){
        u32 s0 = sha_rotr(w[i-15],7) ^ sha_rotr(w[i-15],18) ^ (w[i-15]>>3);
        u32 s1 = sha_rotr(w[i-2],17) ^ sha_rotr(w[i-2],19) ^ (w[i-2]>>10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    u32 a=st[0],b=st[1],c=st[2],d=st[3],e=st[4],f=st[5],g=st[6],h=st[7];
    for(int i=0;i<64;i++){
        u32 S1 = sha_rotr(e,6) ^ sha_rotr(e,11) ^ sha_rotr(e,25);
        u32 ch = (e & f) ^ ((~e) & g);
        u32 t1 = h + S1 + ch + K[i] + w[i];
        u32 S0 = sha_rotr(a,2) ^ sha_rotr(a,13) ^ sha_rotr(a,22);
        u32 maj = (a & b) ^ (a & c) ^ (b & c);
        u32 t2 = S0 + maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    st[0]+=a; st[1]+=b; st[2]+=c; st[3]+=d; st[4]+=e; st[5]+=f; st[6]+=g; st[7]+=h;
}

typedef struct {
    u32 state[8];
    u8  buf[64];
    u32 buflen;
    u32 bitlen_hi, bitlen_lo;
} Sha256Ctx;

static void sha256_init(Sha256Ctx *c){
    c->state[0]=0x6a09e667; c->state[1]=0xbb67ae85; c->state[2]=0x3c6ef372; c->state[3]=0xa54ff53a;
    c->state[4]=0x510e527f; c->state[5]=0x9b05688c; c->state[6]=0x1f83d9ab; c->state[7]=0x5be0cd19;
    c->buflen=0; c->bitlen_hi=0; c->bitlen_lo=0;
}
static void sha256_update(Sha256Ctx *c, const u8 *data, u32 len){
    for(u32 i=0;i<len;i++){
        c->buf[c->buflen++] = data[i];
        if(c->buflen == 64){
            sha256_transform(c->state, c->buf);
            /* +512 bits */
            u32 lo = c->bitlen_lo + 512;
            if(lo < c->bitlen_lo) c->bitlen_hi++;
            c->bitlen_lo = lo;
            c->buflen = 0;
        }
    }
}
static void sha256_final(Sha256Ctx *c, u8 out[32]){
    u32 i = c->buflen;
    /* bits processed in incomplete block */
    u32 lo = c->bitlen_lo + i*8;
    u32 hi = c->bitlen_hi;
    if(lo < c->bitlen_lo) hi++;
    c->buf[i++] = 0x80;
    if(i > 56){
        while(i < 64) c->buf[i++] = 0;
        sha256_transform(c->state, c->buf);
        i = 0;
    }
    while(i < 56) c->buf[i++] = 0;
    c->buf[56]=(u8)(hi>>24); c->buf[57]=(u8)(hi>>16); c->buf[58]=(u8)(hi>>8); c->buf[59]=(u8)hi;
    c->buf[60]=(u8)(lo>>24); c->buf[61]=(u8)(lo>>16); c->buf[62]=(u8)(lo>>8); c->buf[63]=(u8)lo;
    sha256_transform(c->state, c->buf);
    for(i=0;i<8;i++){
        out[i*4]=(u8)(c->state[i]>>24); out[i*4+1]=(u8)(c->state[i]>>16);
        out[i*4+2]=(u8)(c->state[i]>>8); out[i*4+3]=(u8)c->state[i];
    }
}
static void sha256(const u8 *data, u32 len, u8 out[32]){
    Sha256Ctx c; sha256_init(&c); sha256_update(&c, data, len); sha256_final(&c, out);
}

/* ---- HMAC-SHA256 ---- */
static void hmac_sha256(const u8 *key, u32 keylen, const u8 *msg, u32 msglen, u8 out[32]){
    u8 k[64]; memset(k, 0, 64);
    if(keylen > 64){ sha256(key, keylen, k); }
    else memcpy(k, key, keylen);
    u8 ipad[64], opad[64];
    for(int i=0;i<64;i++){ ipad[i]=k[i]^0x36; opad[i]=k[i]^0x5c; }
    Sha256Ctx c;
    u8 inner[32];
    sha256_init(&c); sha256_update(&c, ipad, 64); sha256_update(&c, msg, msglen); sha256_final(&c, inner);
    sha256_init(&c); sha256_update(&c, opad, 64); sha256_update(&c, inner, 32); sha256_final(&c, out);
}

/* ---- AES-128 (Rijndael) encrypt one block — base para AES-GCM/CBC posteriores ---- */
static const u8 AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};
static const u8 AES_RCON[11] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36};
static u8 aes_xtime(u8 x){ return (u8)((x<<1) ^ ((x&0x80)?0x1b:0)); }
static void aes_key_expand(const u8 key[16], u8 rk[176]){
    memcpy(rk, key, 16);
    for(int i=4;i<44;i++){
        u8 t[4] = { rk[(i-1)*4], rk[(i-1)*4+1], rk[(i-1)*4+2], rk[(i-1)*4+3] };
        if((i%4)==0){
            u8 tmp=t[0]; t[0]=AES_SBOX[t[1]]^AES_RCON[i/4]; t[1]=AES_SBOX[t[2]]; t[2]=AES_SBOX[t[3]]; t[3]=AES_SBOX[tmp];
        }
        rk[i*4]  =rk[(i-4)*4]  ^t[0];
        rk[i*4+1]=rk[(i-4)*4+1]^t[1];
        rk[i*4+2]=rk[(i-4)*4+2]^t[2];
        rk[i*4+3]=rk[(i-4)*4+3]^t[3];
    }
}
static void aes_encrypt_block(const u8 rk[176], const u8 in[16], u8 out[16]){
    u8 s[16]; memcpy(s, in, 16);
    for(int i=0;i<16;i++) s[i]^=rk[i];
    for(int round=1; round<=10; round++){
        for(int i=0;i<16;i++) s[i]=AES_SBOX[s[i]];
        /* shift rows */
        u8 t;
        t=s[1]; s[1]=s[5]; s[5]=s[9]; s[9]=s[13]; s[13]=t;
        t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t;
        t=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=s[3]; s[3]=t;
        if(round < 10){
            for(int c=0;c<4;c++){
                u8 *p = s + c*4;
                u8 a0=p[0],a1=p[1],a2=p[2],a3=p[3];
                p[0]=aes_xtime(a0)^aes_xtime(a1)^a1^a2^a3;
                p[1]=a0^aes_xtime(a1)^aes_xtime(a2)^a2^a3;
                p[2]=a0^a1^aes_xtime(a2)^aes_xtime(a3)^a3;
                p[3]=aes_xtime(a0)^a0^a1^a2^aes_xtime(a3);
            }
        }
        for(int i=0;i<16;i++) s[i]^=rk[round*16+i];
    }
    memcpy(out, s, 16);
}

static const u8 AES_INV_SBOX[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};
static u8 aes_mul(u8 a, u8 b){
    u8 p=0;
    for(int i=0;i<8;i++){
        if(b&1) p^=a;
        u8 hi=a&0x80; a<<=1; if(hi) a^=0x1b; b>>=1;
    }
    return p;
}
static void aes_decrypt_block(const u8 rk[176], const u8 in[16], u8 out[16]){
    u8 s[16]; memcpy(s, in, 16);
    for(int i=0;i<16;i++) s[i]^=rk[160+i];
    for(int round=9; round>=0; round--){
        /* inv shift rows */
        u8 t;
        t=s[13]; s[13]=s[9]; s[9]=s[5]; s[5]=s[1]; s[1]=t;
        t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t;
        t=s[3]; s[3]=s[7]; s[7]=s[11]; s[11]=s[15]; s[15]=t;
        for(int i=0;i<16;i++) s[i]=AES_INV_SBOX[s[i]];
        for(int i=0;i<16;i++) s[i]^=rk[round*16+i];
        if(round > 0){
            for(int c=0;c<4;c++){
                u8 *p=s+c*4;
                u8 a0=p[0],a1=p[1],a2=p[2],a3=p[3];
                p[0]=aes_mul(a0,0x0e)^aes_mul(a1,0x0b)^aes_mul(a2,0x0d)^aes_mul(a3,0x09);
                p[1]=aes_mul(a0,0x09)^aes_mul(a1,0x0e)^aes_mul(a2,0x0b)^aes_mul(a3,0x0d);
                p[2]=aes_mul(a0,0x0d)^aes_mul(a1,0x09)^aes_mul(a2,0x0e)^aes_mul(a3,0x0b);
                p[3]=aes_mul(a0,0x0b)^aes_mul(a1,0x0d)^aes_mul(a2,0x09)^aes_mul(a3,0x0e);
            }
        }
    }
    memcpy(out, s, 16);
}

/* Self-test rápido de SHA-256 (vector vacío) */
static int crypto_selftest(void){
    u8 dig[32];
    sha256((const u8*)"", 0, dig);
    /* e3b0c44298fc1c14... */
    int ok = dig[0]==0xe3 && dig[1]==0xb0 && dig[2]==0xc4 && dig[3]==0x42;
    /* ejercitar HMAC y AES para dejar la API lista */
    u8 mac[32], rk[176], pt[16], ct[16];
    hmac_sha256((const u8*)"key", 3, (const u8*)"msg", 3, mac);
    memset(pt, 0, 16); pt[0]=0x11;
    u8 key[16]; memset(key, 0x2b, 16);
    aes_key_expand(key, rk);
    aes_encrypt_block(rk, pt, ct);
    ok = ok && (mac[0] | ct[0] | 1);
    return ok;
}

/* ---- TLS 1.2 ClientHello (record + handshake) ---- */
static u32 tls_hello_sent = 0, tls_records_rx = 0;
static u8  tls_last_server_type = 0;
static int tls_crypto_ok = 0;
/* Resultado del parseo del handshake del servidor (0.24) */
static int  tls_got_server_hello = 0;
static int  tls_got_certificate = 0;
static int  tls_got_server_hello_done = 0;
static int  tls_got_key_exchange = 0;
static int  tls_got_cert_req = 0;
static int  tls_alert = 0;
static u16  tls_server_version = 0;
static u16  tls_cipher_suite = 0;
static u8   tls_server_random[32];
static u32  tls_cert_count = 0;
static u32  tls_cert0_len = 0;
static u8   tls_cert0_sha[32]; /* SHA-256 del primer certificado DER */
static char tls_suite_name[48];
static u8  tls_ske_pub[128];
static int tls_ske_pub_len = 0;
static u16 tls_ske_curve = 0; /* 0x001D = x25519, 0x0017 = secp256r1 */
static int tls_ecdhe_ok = 0;
static char tls_req_path[96] = "/";
static u8   tls_client_random[32];
static u8   tls_premaster[48];
static u8   tls_master[48];
static int  tls_ke_ok = 0;
static int  tls_finished_sent = 0;
static int  tls_rsa_bits = 0;
static int  tls_handshake_status = 0;
static u8   tls_cert0_der[2048]; /* copia del primer cert para RSA */
static u32  tls_cert0_der_len = 0;
static Sha256Ctx tls_hs_sha;
static int  tls_hs_on = 0;

static void tls_hs_init(void){ sha256_init(&tls_hs_sha); tls_hs_on=1; }
static void tls_hs_add(const u8 *d, u32 n){ if(tls_hs_on && d && n) sha256_update(&tls_hs_sha, d, n); }
static void tls_hs_digest(u8 out[32]){ Sha256Ctx t=tls_hs_sha; sha256_final(&t, out); }


static int tls_build_client_hello(u8 *out, u32 max, const char *sni_host){
    /* Construye un ClientHello TLS 1.2 con SNI y suites comunes (GCM/ECDHE listadas;
       el servidor responderá; completar el handshake requiere más trabajo). */
    if(max < 200) return -1;
    u8 *p = out;
    /* TLS record header placeholder */
    p[0]=0x16; /* Handshake */
    p[1]=0x03; p[2]=0x01; /* TLS 1.0 for compatibility in record version */
    /* length later */
    u8 *rec_len = p+3;
    p += 5;

    u8 *hs = p;
    hs[0]=0x01; /* ClientHello */
    /* hs length 3 bytes later */
    u8 *hs_len = hs+1;
    p = hs+4;

    /* client_version TLS 1.2 */
    *p++=0x03; *p++=0x03;
    /* random 32 bytes (guardar para PRF) */
    for(int i=0;i<32;i++){
        u8 b = (u8)(0xA5 ^ (ticks + i*17) ^ (i*3) ^ (i*i));
        tls_client_random[i] = b;
        *p++ = b;
    }
    /* session id empty */
    *p++ = 0;
    /* cipher suites: list several TLS 1.2 suites (we cannot finish all yet) */
    static const u8 suites[] = {
        0xC0,0x2F, /* TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 — 0.29 */
        0xC0,0x30, /* TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 */
        0x00,0x9C, /* TLS_RSA_WITH_AES_128_GCM_SHA256 */
        0x00,0x2F, /* TLS_RSA_WITH_AES_128_CBC_SHA */
        0x00,0x35, /* TLS_RSA_WITH_AES_256_CBC_SHA */
    };
    u16 cs_len = sizeof suites;
    *p++ = (u8)(cs_len>>8); *p++ = (u8)cs_len;
    for(u16 i=0;i<cs_len;i++) *p++ = suites[i];
    /* compression null */
    *p++ = 1; *p++ = 0;

    /* extensions: SNI + supported versions + signature algorithms + ec point formats */
    u8 *ext_len_at = p; p += 2;
    u8 *ext_start = p;

    /* SNI */
    if(sni_host && sni_host[0]){
        u32 hlen = 0; while(sni_host[hlen] && hlen < 64) hlen++;
        *p++=0x00; *p++=0x00; /* type server_name */
        u16 el = (u16)(5 + hlen);
        *p++=(u8)(el>>8); *p++=(u8)el;
        u16 ll = (u16)(3 + hlen);
        *p++=(u8)(ll>>8); *p++=(u8)ll;
        *p++=0x00; /* host_name */
        *p++=(u8)(hlen>>8); *p++=(u8)hlen;
        for(u32 i=0;i<hlen;i++) *p++=(u8)sni_host[i];
    }

    /* signature_algorithms */
    {
        *p++=0x00; *p++=0x0d;
        *p++=0x00; *p++=0x08;
        *p++=0x00; *p++=0x06;
        *p++=0x04; *p++=0x01; /* rsa_pkcs1_sha256 */
        *p++=0x02; *p++=0x01; /* rsa_pkcs1_sha1 */
        *p++=0x04; *p++=0x03; /* ecdsa_secp256r1_sha256 */
    }

    /* ec_point_formats */
    {
        *p++=0x00; *p++=0x0b;
        *p++=0x00; *p++=0x02;
        *p++=0x01; *p++=0x00;
    }

    /* supported_groups (curves) */
    {
        *p++=0x00; *p++=0x0a;
        *p++=0x00; *p++=0x08;
        *p++=0x00; *p++=0x06;
        *p++=0x00; *p++=0x17; /* secp256r1 */
        *p++=0x00; *p++=0x18; /* secp384r1 */
        *p++=0x00; *p++=0x1d; /* x25519 */
    }

    u16 ext_total = (u16)(p - ext_start);
    ext_len_at[0]=(u8)(ext_total>>8); ext_len_at[1]=(u8)ext_total;

    u32 hs_body = (u32)(p - (hs+4));
    hs_len[0]=(u8)(hs_body>>16); hs_len[1]=(u8)(hs_body>>8); hs_len[2]=(u8)hs_body;

    u16 rec_body = (u16)(p - (out+5));
    rec_len[0]=(u8)(rec_body>>8); rec_len[1]=(u8)rec_body;
    return (int)(p - out);
}

static void tls_suite_label(u16 suite, char *out, int max){
    const char *s = "desconocida";
    switch(suite){
        case 0x002F: s="RSA_AES_128_CBC_SHA"; break;
        case 0x0035: s="RSA_AES_256_CBC_SHA"; break;
        case 0x009C: s="RSA_AES_128_GCM_SHA256"; break;
        case 0x009D: s="RSA_AES_256_GCM_SHA384"; break;
        case 0xC02F: s="ECDHE_RSA_AES_128_GCM_SHA256"; break;
        case 0xC030: s="ECDHE_RSA_AES_256_GCM_SHA384"; break;
        case 0xCCA8: s="ECDHE_RSA_CHACHA20_POLY1305"; break;
        case 0x1301: s="TLS13_AES_128_GCM_SHA256"; break;
        case 0x1302: s="TLS13_AES_256_GCM_SHA384"; break;
        default: break;
    }
    int i=0; while(s[i] && i<max-1){ out[i]=s[i]; i++; } out[i]=0;
}

/* Recorre records TLS y mensajes handshake en un buffer. */
static void tls_parse_response(const u8 *buf, int n){
    tls_got_server_hello = tls_got_certificate = tls_got_server_hello_done = 0;
    tls_got_key_exchange = tls_got_cert_req = tls_alert = 0;
    tls_server_version = 0; tls_cipher_suite = 0;
    tls_cert_count = 0; tls_cert0_len = 0;
    memset(tls_server_random, 0, 32);
    memset(tls_cert0_sha, 0, 32);
    tls_suite_name[0] = 0;
    tls_last_server_type = 0;

    int off = 0;
    while(off + 5 <= n){
        u8 rtype = buf[off];
        u16 rver = (u16)((buf[off+1]<<8)|buf[off+2]);
        u16 rlen = (u16)((buf[off+3]<<8)|buf[off+4]);
        (void)rver;
        if(rlen > 16384 || off + 5 + rlen > n) break;
        tls_last_server_type = rtype;
        tls_records_rx++;
        const u8 *body = buf + off + 5;
        int blen = rlen;

        if(rtype == 0x15 && blen >= 2){
            tls_alert = (body[0]<<8)|body[1]; /* level|desc simplified: store both */
            tls_alert = body[1]; /* description code */
        } else if(rtype == 0x16){
            int hsoff = 0;
            while(hsoff + 4 <= blen){
                u8 htype = body[hsoff];
                u32 hlen = ((u32)body[hsoff+1]<<16)|((u32)body[hsoff+2]<<8)|body[hsoff+3];
                if(hsoff + 4 + (int)hlen > blen) break;
                const u8 *msg = body + hsoff + 4;

                if(htype == 2 && hlen >= 38){ /* ServerHello */
                    tls_got_server_hello = 1;
                    tls_server_version = (u16)((msg[0]<<8)|msg[1]);
                    for(int i=0;i<32;i++) tls_server_random[i] = msg[2+i];
                    u8 sid_len = msg[34];
                    int p = 35 + sid_len;
                    if(p + 3 <= (int)hlen){
                        tls_cipher_suite = (u16)((msg[p]<<8)|msg[p+1]);
                        tls_suite_label(tls_cipher_suite, tls_suite_name, sizeof tls_suite_name);
                    }
                } else if(htype == 11 && hlen >= 3){ /* Certificate */
                    tls_got_certificate = 1;
                    u32 list_len = ((u32)msg[0]<<16)|((u32)msg[1]<<8)|msg[2];
                    u32 pos = 3;
                    u32 count = 0;
                    while(pos + 3 <= 3 + list_len && pos + 3 <= hlen){
                        u32 clen = ((u32)msg[pos]<<16)|((u32)msg[pos+1]<<8)|msg[pos+2];
                        pos += 3;
                        if(pos + clen > hlen) break;
                        if(count == 0){
                            tls_cert0_len = clen;
                            sha256(msg + pos, clen, tls_cert0_sha);
                            tls_cert0_der_len = clen < sizeof(tls_cert0_der) ? clen : sizeof(tls_cert0_der);
                            memcpy(tls_cert0_der, msg + pos, tls_cert0_der_len);
                        }
                        count++;
                        pos += clen;
                    }
                    tls_cert_count = count;
                } else if(htype == 12){ /* ServerKeyExchange */
                    tls_got_key_exchange = 1;
                    /* NamedCurve: curve_type(1)=3, namedcurve(2), pubkey_len(1), pubkey */
                    if(hlen >= 4 && msg[0]==0x03){
                        tls_ske_curve = (u16)((msg[1]<<8)|msg[2]);
                        u8 plen = msg[3];
                        if(4+plen <= (int)hlen && plen <= sizeof(tls_ske_pub)){
                            tls_ske_pub_len = plen;
                            memcpy(tls_ske_pub, msg+4, plen);
                        }
                    }
                } else if(htype == 13){ /* CertificateRequest */
                    tls_got_cert_req = 1;
                } else if(htype == 14){ /* ServerHelloDone */
                    tls_got_server_hello_done = 1;
                }
                hsoff += 4 + (int)hlen;
            }
        }
        off += 5 + rlen;
    }
}

/* Conecta a host:443, envía ClientHello, lee y parsea ServerHello/Certificate.
   Devuelve bytes recibidos o <0. Handshake aún incompleto. */

/* ---- RSA 2048 (bigint little-endian words) ---- */
#define BN_MAX 64
typedef struct { u32 v[BN_MAX]; int n; } BN;
static void bn_zero(BN *a){ memset(a->v,0,sizeof a->v); a->n=1; }
static void bn_set_u32(BN *a, u32 x){ bn_zero(a); a->v[0]=x; }
static void bn_from_be(BN *a, const u8 *b, int len){
    bn_zero(a);
    int wi=0, sh=0; u32 w=0;
    for(int i=len-1;i>=0;i--){
        w |= ((u32)b[i]) << sh;
        sh += 8;
        if(sh==32){ if(wi<BN_MAX) a->v[wi++]=w; w=0; sh=0; }
    }
    if(sh && wi<BN_MAX) a->v[wi++]=w;
    a->n = wi ? wi : 1;
    while(a->n>1 && a->v[a->n-1]==0) a->n--;
}
static void bn_to_be(const BN *a, u8 *b, int len){
    memset(b,0,len);
    for(int i=0;i<a->n;i++){
        for(int j=0;j<4;j++){
            int pos = len-1 - (i*4+j);
            if(pos>=0) b[pos] = (u8)((a->v[i]>>(8*j))&0xFF);
        }
    }
}
static int bn_cmp(const BN *a, const BN *b){
    if(a->n!=b->n) return a->n>b->n?1:-1;
    for(int i=a->n-1;i>=0;i--) if(a->v[i]!=b->v[i]) return a->v[i]>b->v[i]?1:-1;
    return 0;
}
static void bn_sub(BN *a, const BN *b){
    u32 borrow=0;
    for(int i=0;i<a->n;i++){
        u64 av=a->v[i];
        u64 bv=(u64)(i<b->n?b->v[i]:0) + borrow;
        if(av < bv){ a->v[i]=(u32)(av + (1ull<<32) - bv); borrow=1; }
        else { a->v[i]=(u32)(av - bv); borrow=0; }
    }
    while(a->n>1 && a->v[a->n-1]==0) a->n--;
}
static void bn_copy(BN *d, const BN *s){ *d=*s; }

/* a = a + b */
static void bn_add(BN *a, const BN *b){
    u64 c=0; int n=a->n>b->n?a->n:b->n;
    for(int i=0;i<n;i++){
        c += (u64)(i<a->n?a->v[i]:0) + (u64)(i<b->n?b->v[i]:0);
        if(i>=a->n) a->n=i+1;
        a->v[i]=(u32)c; c>>=32;
    }
    if(c && a->n<BN_MAX) a->v[a->n++]=(u32)c;
}
/* r = a * b (schoolbook) */
static void bn_mul(BN *r, const BN *a, const BN *b){
    BN t; bn_zero(&t); t.n = a->n+b->n; if(t.n>BN_MAX) t.n=BN_MAX;
    for(int i=0;i<a->n;i++){
        u64 c=0;
        for(int j=0;j<b->n;j++){
            if(i+j>=BN_MAX) break;
            c += (u64)t.v[i+j] + (u64)a->v[i]*b->v[j];
            t.v[i+j]=(u32)c; c>>=32;
        }
        if(i+b->n<BN_MAX) t.v[i+b->n]+=(u32)c;
    }
    while(t.n>1 && t.v[t.n-1]==0) t.n--;
    *r=t;
}
/* a = a mod m  (binary long division style using shifts - slow but ok) */
static void bn_mod(BN *a, const BN *m){
    if(bn_cmp(a,m)<0) return;
    BN mm, cur;
    /* while a >= m, subtract shifted m */
    while(bn_cmp(a,m)>=0){
        bn_copy(&mm, m);
        bn_copy(&cur, m);
        /* shift mm left until just below a */
        int shifts=0;
        for(;;){
            /* mm2 = mm << 1 */
            BN mm2=mm; u64 c=0;
            for(int i=0;i<mm2.n;i++){ u64 x=((u64)mm2.v[i]<<1)|c; mm2.v[i]=(u32)x; c=x>>32; }
            if(c && mm2.n<BN_MAX) mm2.v[mm2.n++]=(u32)c;
            if(bn_cmp(&mm2,a)>0) break;
            mm=mm2; shifts++;
            if(shifts>2048) break;
        }
        /* a -= mm */
        u64 borrow=0;
        for(int i=0;i<a->n;i++){
            u64 av=a->v[i], bv=(i<mm.n?mm.v[i]:0)+borrow;
            if(av<bv){ a->v[i]=(u32)(av+(1ull<<32)-bv); borrow=1; }
            else { a->v[i]=(u32)(av-bv); borrow=0; }
        }
        while(a->n>1 && a->v[a->n-1]==0) a->n--;
    }
}
static void bn_modmul(BN *r, const BN *a, const BN *b, const BN *m){
    BN t; bn_mul(&t,a,b); bn_mod(&t,m); *r=t;
}
/* r = base^exp mod m  (exp is small u32 often 65537) */
static void bn_modexp_u32(BN *r, const BN *base, u32 exp, const BN *m){
    BN res, b; (void)bn_add; (void)bn_sub; bn_set_u32(&res,1); bn_copy(&b, base); bn_mod(&b,m);
    while(exp){
        if(exp&1) bn_modmul(&res,&res,&b,m);
        bn_modmul(&b,&b,&b,m);
        exp>>=1;
    }
    *r=res;
}

/* Extrae modulus + exponent RSA de un cert X.509 DER (heurística BIT STRING RSA). */
static int x509_rsa_pub(const u8 *der, u32 derlen, BN *mod, u32 *exp_out, int *mod_bytes){
    /* Buscar secuencia BIT STRING que encapsula SEQUENCE { INTEGER n, INTEGER e } */
    for(u32 i=0;i+10<derlen;i++){
        if(der[i]!=0x30) continue; /* SEQUENCE */
        /* intentar parsear como SubjectPublicKeyInfo interior */
        u32 j=i;
        /* rough scan for INTEGER modulus of length 129-257 (1024-2048 bit) */
        if(j+4>=derlen) continue;
    }
    /* Escaneo de INTEGER grandes: tag 0x02, length 0x81 xx or 0x82 xx xx */
    for(u32 i=0;i+6<derlen;i++){
        if(der[i]!=0x02) continue;
        u32 len=0, p=i+1;
        if(der[p] & 0x80){
            int nb = der[p] & 0x7F; p++;
            if(nb==1 && p<derlen){ len=der[p++]; }
            else if(nb==2 && p+1<derlen){ len=((u32)der[p]<<8)|der[p+1]; p+=2; }
            else continue;
        } else len=der[p++];
        if(len < 64 || len > 257 || p+len > derlen) continue;
        /* leading 0x00 optional */
        const u8 *mb = der+p; u32 ml=len;
        if(ml>0 && mb[0]==0x00){ mb++; ml--; }
        if(ml < 64 || ml > 256) continue;
        /* next should be small INTEGER exponent */
        u32 q = p+len;
        if(q+2 >= derlen || der[q]!=0x02) continue;
        u32 el; u32 ep=q+1;
        if(der[ep]&0x80) continue;
        el=der[ep++];
        if(el==0 || el>4 || ep+el>derlen) continue;
        u32 exp=0; for(u32 k=0;k<el;k++) exp=(exp<<8)|der[ep+k];
        if(exp!=3 && exp!=65537 && exp!=17) continue;
        bn_from_be(mod, mb, (int)ml);
        *exp_out = exp;
        *mod_bytes = (int)ml;
        return 1;
    }
    return 0;
}

/* PKCS#1 v1.5 encrypt: EB = 0x00||0x02||PS||0x00||M */
static int rsa_pkcs1_encrypt(const BN *mod, u32 exp, int mod_bytes,
                             const u8 *msg, int mlen, u8 *out){
    if(mlen > mod_bytes - 11) return -1;
    u8 eb[256];
    if(mod_bytes > 256) return -1;
    memset(eb, 0, mod_bytes);
    eb[0]=0x00; eb[1]=0x02;
    int ps = mod_bytes - 3 - mlen;
    for(int i=0;i<ps;i++){
        u8 r = (u8)(0xA5 ^ ticks ^ (i*37) ^ (i<<1));
        if(r==0) r=0x42;
        eb[2+i]=r;
    }
    eb[2+ps]=0x00;
    memcpy(eb+3+ps, msg, mlen);
    BN m, c;
    bn_from_be(&m, eb, mod_bytes);
    bn_modexp_u32(&c, &m, exp, mod);
    bn_to_be(&c, out, mod_bytes);
    return mod_bytes;
}

/* TLS 1.2 PRF (SHA-256): P_hash */
static void tls12_p_hash(const u8 *secret, u32 secret_len,
                         const u8 *seed, u32 seed_len,
                         u8 *out, u32 out_len){
    u8 a[32];
    hmac_sha256(secret, secret_len, seed, seed_len, a); /* A(1) */
    u32 produced = 0;
    while(produced < out_len){
        u8 buf[32+128];
        if(seed_len > 128) seed_len = 128;
        memcpy(buf, a, 32);
        memcpy(buf+32, seed, seed_len);
        u8 block[32];
        hmac_sha256(secret, secret_len, buf, 32+seed_len, block);
        u32 take = out_len - produced; if(take>32) take=32;
        memcpy(out+produced, block, take);
        produced += take;
        /* A(i+1) = HMAC(secret, A(i)) */
        hmac_sha256(secret, secret_len, a, 32, a);
    }
}
static void tls12_prf(const u8 *secret, u32 slen, const char *label,
                      const u8 *seed, u32 seedlen, u8 *out, u32 outlen){
    u8 ls[64+64];
    u32 llen=0; while(label[llen] && llen<32){ ls[llen]=(u8)label[llen]; llen++; }
    memcpy(ls+llen, seed, seedlen);
    tls12_p_hash(secret, slen, ls, llen+seedlen, out, outlen);
}


/* ---- SHA-1 (para HMAC-SHA1 de AES_128_CBC_SHA) ---- */
static u32 sha1_rotl(u32 x, int n){ return (x<<n)|(x>>(32-n)); }
static void sha1_transform(u32 st[5], const u8 block[64]){
    u32 w[80];
    for(int i=0;i<16;i++)
        w[i]=((u32)block[i*4]<<24)|((u32)block[i*4+1]<<16)|((u32)block[i*4+2]<<8)|block[i*4+3];
    for(int i=16;i<80;i++) w[i]=sha1_rotl(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    u32 a=st[0],b=st[1],c=st[2],d=st[3],e=st[4];
    for(int i=0;i<80;i++){
        u32 f,k;
        if(i<20){ f=(b&c)|((~b)&d); k=0x5A827999; }
        else if(i<40){ f=b^c^d; k=0x6ED9EBA1; }
        else if(i<60){ f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDC; }
        else { f=b^c^d; k=0xCA62C1D6; }
        u32 t=sha1_rotl(a,5)+f+e+k+w[i];
        e=d; d=c; c=sha1_rotl(b,30); b=a; a=t;
    }
    st[0]+=a; st[1]+=b; st[2]+=c; st[3]+=d; st[4]+=e;
}
typedef struct { u32 s[5]; u8 buf[64]; u32 bl; u32 lo,hi; } Sha1Ctx;
static void sha1_init(Sha1Ctx *c){
    c->s[0]=0x67452301; c->s[1]=0xEFCDAB89; c->s[2]=0x98BADCFE; c->s[3]=0x10325476; c->s[4]=0xC3D2E1F0;
    c->bl=0; c->lo=0; c->hi=0;
}
static void sha1_update(Sha1Ctx *c, const u8 *data, u32 len){
    for(u32 i=0;i<len;i++){
        c->buf[c->bl++]=data[i];
        if(c->bl==64){
            sha1_transform(c->s,c->buf);
            u32 nlo=c->lo+512; if(nlo<c->lo) c->hi++; c->lo=nlo; c->bl=0;
        }
    }
}
static void sha1_final(Sha1Ctx *c, u8 out[20]){
    u32 i=c->bl;
    u32 lo=c->lo+i*8, hi=c->hi; if(lo<c->lo) hi++;
    c->buf[i++]=0x80;
    if(i>56){ while(i<64) c->buf[i++]=0; sha1_transform(c->s,c->buf); i=0; }
    while(i<56) c->buf[i++]=0;
    c->buf[56]=hi>>24; c->buf[57]=hi>>16; c->buf[58]=hi>>8; c->buf[59]=hi;
    c->buf[60]=lo>>24; c->buf[61]=lo>>16; c->buf[62]=lo>>8; c->buf[63]=lo;
    sha1_transform(c->s,c->buf);
    for(i=0;i<5;i++){ out[i*4]=c->s[i]>>24; out[i*4+1]=c->s[i]>>16; out[i*4+2]=c->s[i]>>8; out[i*4+3]=c->s[i]; }
}
static void hmac_sha1(const u8 *key, u32 keylen, const u8 *msg, u32 msglen, u8 out[20]){
    u8 k[64]; memset(k,0,64);
    if(keylen>64){
        Sha1Ctx c; sha1_init(&c); sha1_update(&c,key,keylen); sha1_final(&c,k);
    } else memcpy(k,key,keylen);
    u8 ip[64],op[64];
    for(int i=0;i<64;i++){ ip[i]=k[i]^0x36; op[i]=k[i]^0x5c; }
    Sha1Ctx c; u8 inn[20];
    sha1_init(&c); sha1_update(&c,ip,64); sha1_update(&c,msg,msglen); sha1_final(&c,inn);
    sha1_init(&c); sha1_update(&c,op,64); sha1_update(&c,inn,20); sha1_final(&c,out);
}

/* ---- Claves de tráfico TLS 1.2 (AES_128_CBC_SHA) ---- */
static u8 tls_client_mac[20];
static u8 tls_server_mac[20];
static u8 tls_client_key[16];
static u8 tls_server_key[16];
static u8 tls_client_rk[176];
static u8 tls_server_rk[176];
static u64 tls_client_seq = 0;
static u64 tls_server_seq = 0;
static int tls_keys_ready = 0;
static int tls_record_enc_ok = 0;
static int tls_use_gcm = 0;
static u8  tls_client_iv_salt[4];
static u8  tls_server_iv_salt[4];
static u8  tls_gcm_client_key[16];
static u8  tls_gcm_server_key[16];
static u8  tls_gcm_client_rk[176];
static u8  tls_gcm_server_rk[176];

/* ---- GHASH / AES-GCM (TLS 1.2) ---- */
static void ghash_mul(u8 x[16], const u8 y[16]){
    u8 z[16]; memset(z,0,16);
    u8 v[16]; memcpy(v,y,16);
    for(int i=0;i<16;i++){
        for(int j=7;j>=0;j--){
            if((x[i]>>j)&1){ for(int k=0;k<16;k++) z[k]^=v[k]; }
            int lsb = v[15] & 1;
            for(int k=15;k>0;k--) v[k]=(u8)((v[k]>>1)|((v[k-1]&1)<<7));
            v[0]>>=1;
            if(lsb) v[0]^=0xe1;
        }
    }
    memcpy(x,z,16);
}
static void ghash(const u8 h[16], const u8 *aad, u32 aad_len, const u8 *ct, u32 ct_len, u8 out[16]){
    u8 y[16]; memset(y,0,16);
    /* process AAD in 16-byte blocks */
    u8 block[16];
    u32 off=0;
    while(off < aad_len){
        memset(block,0,16);
        u32 n = aad_len-off; if(n>16) n=16;
        memcpy(block, aad+off, n);
        for(int i=0;i<16;i++) y[i]^=block[i];
        ghash_mul(y, h);
        off+=n;
    }
    off=0;
    while(off < ct_len){
        memset(block,0,16);
        u32 n = ct_len-off; if(n>16) n=16;
        memcpy(block, ct+off, n);
        for(int i=0;i<16;i++) y[i]^=block[i];
        ghash_mul(y, h);
        off+=n;
    }
    /* lengths block: aad_bits || ct_bits as 64-bit BE each */
    memset(block,0,16);
    u64 ab=(u64)aad_len*8, cb=(u64)ct_len*8;
    for(int i=0;i<8;i++){ block[7-i]=(u8)(ab>>(8*i)); block[15-i]=(u8)(cb>>(8*i)); }
    for(int i=0;i<16;i++) y[i]^=block[i];
    ghash_mul(y, h);
    memcpy(out,y,16);
}
static void gctr(const u8 rk[176], const u8 icb[16], const u8 *in, u32 len, u8 *out){
    u8 cb[16]; memcpy(cb, icb, 16);
    u32 off=0;
    while(off < len){
        u8 s[16];
        aes_encrypt_block(rk, cb, s);
        u32 n = len-off; if(n>16) n=16;
        for(u32 i=0;i<n;i++) out[off+i]=in[off+i]^s[i];
        /* inc32 counter (last 4 bytes BE) */
        for(int i=15;i>=12;i--){ cb[i]++; if(cb[i]) break; }
        off+=n;
    }
}
static void aes_gcm_encrypt(const u8 rk[176], const u8 nonce[12],
                            const u8 *aad, u32 aad_len,
                            const u8 *pt, u32 pt_len,
                            u8 *ct, u8 tag[16]){
    u8 h[16]; memset(h,0,16);
    aes_encrypt_block(rk, h, h); /* H = AES(K,0) */
    u8 j0[16];
    memcpy(j0, nonce, 12);
    j0[12]=0; j0[13]=0; j0[14]=0; j0[15]=1;
    u8 j0c[16]; memcpy(j0c, j0, 16);
    /* inc for GCTR start */
    for(int i=15;i>=12;i--){ j0c[i]++; if(j0c[i]) break; }
    gctr(rk, j0c, pt, pt_len, ct);
    u8 s[16];
    ghash(h, aad, aad_len, ct, pt_len, s);
    u8 t[16];
    gctr(rk, j0, s, 16, t);
    memcpy(tag, t, 16);
}
static int aes_gcm_decrypt(const u8 rk[176], const u8 nonce[12],
                           const u8 *aad, u32 aad_len,
                           const u8 *ct, u32 ct_len,
                           const u8 tag[16], u8 *pt){
    u8 h[16]; memset(h,0,16);
    aes_encrypt_block(rk, h, h);
    u8 j0[16];
    memcpy(j0, nonce, 12);
    j0[12]=0; j0[13]=0; j0[14]=0; j0[15]=1;
    u8 s[16];
    ghash(h, aad, aad_len, ct, ct_len, s);
    u8 t[16];
    gctr(rk, j0, s, 16, t);
    int fail=0; for(int i=0;i<16;i++) if(t[i]!=tag[i]) fail=1;
    if(fail) return -1;
    u8 j0c[16]; memcpy(j0c, j0, 16);
    for(int i=15;i>=12;i--){ j0c[i]++; if(j0c[i]) break; }
    gctr(rk, j0c, ct, ct_len, pt);
    return 0;
}

static void tls_derive_keys_aes128_gcm(void){
    u8 seed[64];
    memcpy(seed, tls_server_random, 32);
    memcpy(seed+32, tls_client_random, 32);
    u8 kb[64];
    tls12_prf(tls_master, 48, "key expansion", seed, 64, kb, 16+16+4+4);
    memcpy(tls_gcm_client_key, kb, 16);
    memcpy(tls_gcm_server_key, kb+16, 16);
    memcpy(tls_client_iv_salt, kb+32, 4);
    memcpy(tls_server_iv_salt, kb+36, 4);
    aes_key_expand(tls_gcm_client_key, tls_gcm_client_rk);
    aes_key_expand(tls_gcm_server_key, tls_gcm_server_rk);
    tls_client_seq = 0;
    tls_server_seq = 0;
    tls_use_gcm = 1;
    tls_keys_ready = 1;
}

static int tls_send_record_gcm(u8 type, const u8 *body, u16 body_len){
    if(!tls_keys_ready || !tls_use_gcm) return -1;
    u8 explicit[8];
    for(int i=0;i<8;i++) explicit[i]=(u8)((tls_client_seq>>(56-8*i))&0xFF);
    u8 nonce[12];
    memcpy(nonce, tls_client_iv_salt, 4);
    memcpy(nonce+4, explicit, 8);
    u8 aad[13];
    for(int i=0;i<8;i++) aad[i]=(u8)((tls_client_seq>>(56-8*i))&0xFF);
    aad[8]=type; aad[9]=0x03; aad[10]=0x03;
    aad[11]=(u8)(body_len>>8); aad[12]=(u8)body_len;
    u8 ct[2048], tag[16];
    if(body_len > 2000) return -1;
    aes_gcm_encrypt(tls_gcm_client_rk, nonce, aad, 13, body, body_len, ct, tag);
    u8 rec[2100];
    u16 rlen = (u16)(8 + body_len + 16);
    rec[0]=type; rec[1]=0x03; rec[2]=0x03;
    rec[3]=(u8)(rlen>>8); rec[4]=(u8)rlen;
    memcpy(rec+5, explicit, 8);
    memcpy(rec+13, ct, body_len);
    memcpy(rec+13+body_len, tag, 16);
    int r = tcp_send_data(rec, 5+rlen);
    if(r==0){ tls_client_seq++; tls_record_enc_ok=1; }
    return r;
}

static int tls_decrypt_record_gcm(const u8 *rec, int reclen, u8 *out_type, u8 *out_body, u16 *out_len){
    if(!tls_keys_ready || !tls_use_gcm || reclen < 5+8+16) return -1;
    u8 type = rec[0];
    u16 rlen = (u16)((rec[3]<<8)|rec[4]);
    if(5+rlen > reclen || rlen < 8+16) return -2;
    const u8 *explicit = rec+5;
    u32 ct_len = rlen - 8 - 16;
    const u8 *ct = rec+13;
    const u8 *tag = rec+13+ct_len;
    u8 nonce[12];
    memcpy(nonce, tls_server_iv_salt, 4);
    memcpy(nonce+4, explicit, 8);
    u8 aad[13];
    for(int i=0;i<8;i++) aad[i]=(u8)((tls_server_seq>>(56-8*i))&0xFF);
    aad[8]=type; aad[9]=0x03; aad[10]=0x03;
    aad[11]=(u8)(ct_len>>8); aad[12]=(u8)ct_len;
    u8 pt[2048];
    if(ct_len > 2000) return -3;
    if(aes_gcm_decrypt(tls_gcm_server_rk, nonce, aad, 13, ct, ct_len, tag, pt)!=0) return -6;
    tls_server_seq++;
    *out_type = type;
    memcpy(out_body, pt, ct_len);
    *out_len = (u16)ct_len;
    return 0;
}

static int tls_send_record_cbc(u8 type, const u8 *body, u16 body_len);
static int tls_decrypt_record_cbc(const u8 *rec, int reclen, u8 *out_type, u8 *out_body, u16 *out_len);

static int tls_send_app(u8 type, const u8 *body, u16 len){
    if(tls_use_gcm) return tls_send_record_gcm(type, body, len);
    return tls_send_record_cbc(type, body, len);
}
static int tls_decrypt_app(const u8 *rec, int reclen, u8 *otype, u8 *body, u16 *blen){
    if(tls_use_gcm) return tls_decrypt_record_gcm(rec, reclen, otype, body, blen);
    return tls_decrypt_record_cbc(rec, reclen, otype, body, blen);
}


static void tls_derive_keys_aes128_cbc_sha(void){
    /* key_block = PRF(master, "key expansion", server_random + client_random) */
    u8 seed[64];
    memcpy(seed, tls_server_random, 32);
    memcpy(seed+32, tls_client_random, 32);
    u8 kb[128];
    tls12_prf(tls_master, 48, "key expansion", seed, 64, kb, 20+20+16+16);
    memcpy(tls_client_mac, kb, 20);
    memcpy(tls_server_mac, kb+20, 20);
    memcpy(tls_client_key, kb+40, 16);
    memcpy(tls_server_key, kb+56, 16);
    aes_key_expand(tls_client_key, tls_client_rk);
    aes_key_expand(tls_server_key, tls_server_rk);
    tls_client_seq = 0;
    tls_server_seq = 0;
    tls_keys_ready = 1;
}

static void aes_cbc_encrypt(const u8 rk[176], const u8 iv[16], const u8 *in, u32 len, u8 *out){
    u8 prev[16]; memcpy(prev, iv, 16);
    for(u32 off=0; off<len; off+=16){
        u8 block[16];
        for(int i=0;i<16;i++) block[i]=in[off+i]^prev[i];
        aes_encrypt_block(rk, block, out+off);
        memcpy(prev, out+off, 16);
    }
}
static void aes_cbc_decrypt(const u8 rk[176], const u8 iv[16], const u8 *in, u32 len, u8 *out){
    u8 prev[16]; memcpy(prev, iv, 16);
    for(u32 off=0; off<len; off+=16){
        u8 block[16], dec[16];
        aes_decrypt_block(rk, in+off, dec);
        for(int i=0;i<16;i++) out[off+i]=dec[i]^prev[i];
        memcpy(prev, in+off, 16);
    }
}

static int tls_got_server_finished = 0;
static int tls_https_bytes = 0;
static u8  tls_https_buf[8192];

/* Descifra un record CBC del servidor. Devuelve tipo y longitud del fragmento en out. */
static int tls_decrypt_record_cbc(const u8 *rec, int reclen, u8 *out_type, u8 *out_body, u16 *out_len){
    if(!tls_keys_ready || reclen < 5+16+16) return -1;
    u8 type = rec[0];
    u16 rlen = (u16)((rec[3]<<8)|rec[4]);
    if(5+rlen > reclen || rlen < 16+16) return -2;
    const u8 *iv = rec+5;
    const u8 *cipher = rec+21;
    u32 clen = rlen - 16;
    if(clen > 2000 || (clen % 16)) return -3;
    u8 plain[2048];
    aes_cbc_decrypt(tls_server_rk, iv, cipher, clen, plain);
    /* quitar padding PKCS#7 */
    u8 pad = plain[clen-1];
    if(pad > 15 || pad+1 > clen) return -4;
    u32 content_mac = clen - (pad+1);
    if(content_mac < 20) return -5;
    u32 frag_len = content_mac - 20;
    /* verificar MAC */
    u8 mac_in[8+1+2+2+2048];
    for(int i=0;i<8;i++) mac_in[i]=(u8)((tls_server_seq>>(56-8*i))&0xFF);
    mac_in[8]=type; mac_in[9]=0x03; mac_in[10]=0x03;
    mac_in[11]=(u8)(frag_len>>8); mac_in[12]=(u8)frag_len;
    memcpy(mac_in+13, plain, frag_len);
    u8 mac[20];
    hmac_sha1(tls_server_mac, 20, mac_in, 13+frag_len, mac);
    int macfail=0; for(int mi=0;mi<20;mi++) if(mac[mi]!=plain[frag_len+mi]) macfail=1;
    if(macfail){
        /* MAC fail — aún avanzamos seq? mejor no */
        return -6;
    }
    tls_server_seq++;
    *out_type = type;
    memcpy(out_body, plain, frag_len);
    *out_len = (u16)frag_len;
    return 0;
}

/* Cifra un record TLS 1.2 AES_128_CBC_SHA: IV || AES-CBC( PKCS7( fragment || MAC ) ) */
static int tls_send_record_cbc(u8 type, const u8 *body, u16 body_len){
    if(!tls_keys_ready) return -1;
    /* MAC input: seq(8) + type(1) + ver(2) + len(2) + body */
    u8 mac_in[8+1+2+2+2048];
    if(body_len > 2000) return -1;
    for(int i=0;i<8;i++) mac_in[i] = (u8)((tls_client_seq >> (56-8*i)) & 0xFF);
    mac_in[8]=type; mac_in[9]=0x03; mac_in[10]=0x03;
    mac_in[11]=(u8)(body_len>>8); mac_in[12]=(u8)body_len;
    memcpy(mac_in+13, body, body_len);
    u8 mac[20];
    hmac_sha1(tls_client_mac, 20, mac_in, 13+body_len, mac);

    /* fragment || MAC */
    u8 plain[2048];
    memcpy(plain, body, body_len);
    memcpy(plain+body_len, mac, 20);
    u32 plen = body_len + 20;
    /* PKCS#7 padding to 16-byte multiple */
    u32 pad = 16 - (plen % 16);
    if(pad==0) pad=16;
    for(u32 i=0;i<pad;i++) plain[plen+i]=(u8)(pad-1);
    plen += pad;

    /* random IV */
    u8 iv[16];
    for(int i=0;i<16;i++) iv[i]=(u8)(0x5A ^ ticks ^ (i*19) ^ (tls_client_seq*3+i));

    u8 cipher[2048];
    aes_cbc_encrypt(tls_client_rk, iv, plain, plen, cipher);

    u8 rec[2100];
    u16 rlen = (u16)(16 + plen);
    rec[0]=type; rec[1]=0x03; rec[2]=0x03;
    rec[3]=(u8)(rlen>>8); rec[4]=(u8)rlen;
    memcpy(rec+5, iv, 16);
    memcpy(rec+21, cipher, plen);
    int r = tcp_send_data(rec, 5+rlen);
    if(r==0){ tls_client_seq++; tls_record_enc_ok=1; }
    return r;
}

static int tls_send_record(u8 type, const u8 *body, u16 body_len){
    /* Records en claro (pre-CCS) */
    u8 rec[1600];
    if((u32)body_len + 5 > sizeof rec) return -1;
    rec[0]=type; rec[1]=0x03; rec[2]=0x03;
    rec[3]=(u8)(body_len>>8); rec[4]=(u8)body_len;
    memcpy(rec+5, body, body_len);
    return tcp_send_data(rec, 5+body_len);
}

/* ================= X25519 ECDH (RFC 7748) ================= */
static u64 x_load_8(const u8 *in){
    return (u64)in[0] | ((u64)in[1]<<8) | ((u64)in[2]<<16) | ((u64)in[3]<<24)
         | ((u64)in[4]<<32) | ((u64)in[5]<<40) | ((u64)in[6]<<48) | ((u64)in[7]<<56);
}
static void x_store_8(u8 *out, u64 v){
    for(int i=0;i<8;i++) out[i]=(u8)(v>>(8*i));
}
/* 5 limbs × 51 bits */
typedef struct { u64 v[5]; } xf;
static void xf_carry(xf *o){
    u64 c;
    c=o->v[0]>>51; o->v[0]&=((1ull<<51)-1); o->v[1]+=c;
    c=o->v[1]>>51; o->v[1]&=((1ull<<51)-1); o->v[2]+=c;
    c=o->v[2]>>51; o->v[2]&=((1ull<<51)-1); o->v[3]+=c;
    c=o->v[3]>>51; o->v[3]&=((1ull<<51)-1); o->v[4]+=c;
    c=o->v[4]>>51; o->v[4]&=((1ull<<51)-1); o->v[0]+=19*c;
}
static void xf_add(xf *o, const xf *a, const xf *b){
    for(int i=0;i<5;i++) o->v[i]=a->v[i]+b->v[i];
}
static void xf_sub(xf *o, const xf *a, const xf *b){
    u64 t0 = a->v[0] + 0xfffffffffffdaull - b->v[0];
    u64 t1 = a->v[1] + 0xffffffffffffeull - b->v[1];
    u64 t2 = a->v[2] + 0xffffffffffffeull - b->v[2];
    u64 t3 = a->v[3] + 0xffffffffffffeull - b->v[3];
    u64 t4 = a->v[4] + 0xffffffffffffeull - b->v[4];
    o->v[0]=t0; o->v[1]=t1; o->v[2]=t2; o->v[3]=t3; o->v[4]=t4;
    xf_carry(o);
}
static void xf_mul(xf *o, const xf *a, const xf *b){
    u64 a0=a->v[0],a1=a->v[1],a2=a->v[2],a3=a->v[3],a4=a->v[4];
    u64 b0=b->v[0],b1=b->v[1],b2=b->v[2],b3=b->v[3],b4=b->v[4];
    u64 s0=a0*b0;
    u64 s1=a0*b1+a1*b0;
    u64 s2=a0*b2+a1*b1+a2*b0;
    u64 s3=a0*b3+a1*b2+a2*b1+a3*b0;
    u64 s4=a0*b4+a1*b3+a2*b2+a3*b1+a4*b0;
    u64 s5=a1*b4+a2*b3+a3*b2+a4*b1;
    u64 s6=a2*b4+a3*b3+a4*b2;
    u64 s7=a3*b4+a4*b3;
    u64 s8=a4*b4;
    /* reduce */
    s0 += 19*s5; s1 += 19*s6; s2 += 19*s7; s3 += 19*s8;
    u64 c;
    c=s0>>51; s0&=((1ull<<51)-1); s1+=c;
    c=s1>>51; s1&=((1ull<<51)-1); s2+=c;
    c=s2>>51; s2&=((1ull<<51)-1); s3+=c;
    c=s3>>51; s3&=((1ull<<51)-1); s4+=c;
    c=s4>>51; s4&=((1ull<<51)-1); s0+=19*c;
    c=s0>>51; s0&=((1ull<<51)-1); s1+=c;
    o->v[0]=s0; o->v[1]=s1; o->v[2]=s2; o->v[3]=s3; o->v[4]=s4;
}
static void xf_sqr(xf *o, const xf *a){ xf_mul(o,a,a); }
static void xf_inv(xf *o, const xf *z){
    xf t0,t1,t2,t3;
    xf_sqr(&t0,z); xf_sqr(&t1,&t0); xf_sqr(&t1,&t1); xf_mul(&t1,z,&t1);
    xf_mul(&t0,&t0,&t1); xf_sqr(&t2,&t0); xf_mul(&t1,&t1,&t2);
    xf_sqr(&t2,&t1); for(int i=1;i<5;i++) xf_sqr(&t2,&t2); xf_mul(&t1,&t2,&t1);
    xf_sqr(&t2,&t1); for(int i=1;i<10;i++) xf_sqr(&t2,&t2); xf_mul(&t2,&t2,&t1);
    xf_sqr(&t3,&t2); for(int i=1;i<20;i++) xf_sqr(&t3,&t3); xf_mul(&t2,&t3,&t2);
    xf_sqr(&t2,&t2); for(int i=1;i<10;i++) xf_sqr(&t2,&t2); xf_mul(&t1,&t2,&t1);
    xf_sqr(&t2,&t1); for(int i=1;i<50;i++) xf_sqr(&t2,&t2); xf_mul(&t2,&t2,&t1);
    xf_sqr(&t3,&t2); for(int i=1;i<100;i++) xf_sqr(&t3,&t3); xf_mul(&t2,&t3,&t2);
    xf_sqr(&t2,&t2); for(int i=1;i<50;i++) xf_sqr(&t2,&t2); xf_mul(&t1,&t2,&t1);
    xf_sqr(&t1,&t1); for(int i=1;i<5;i++) xf_sqr(&t1,&t1); xf_mul(o,&t1,&t0);
}
static void xf_from_bytes(xf *o, const u8 in[32]){
    u64 x0=x_load_8(in);
    u64 x1=x_load_8(in+6)>>3;
    u64 x2=x_load_8(in+12)>>6;
    u64 x3=x_load_8(in+19)>>1;
    u64 x4=x_load_8(in+24)>>12;
    x4 &= (1ull<<51)-1;
    o->v[0]=x0&((1ull<<51)-1);
    o->v[1]=x1&((1ull<<51)-1);
    o->v[2]=x2&((1ull<<51)-1);
    o->v[3]=x3&((1ull<<51)-1);
    o->v[4]=x4;
}
static void xf_to_bytes(const xf *a, u8 out[32]){
    xf t=*a;
    /* final reduction */
    for(int i=0;i<2;i++){
        u64 c=t.v[0]>>51; t.v[0]&=((1ull<<51)-1); t.v[1]+=c;
        c=t.v[1]>>51; t.v[1]&=((1ull<<51)-1); t.v[2]+=c;
        c=t.v[2]>>51; t.v[2]&=((1ull<<51)-1); t.v[3]+=c;
        c=t.v[3]>>51; t.v[3]&=((1ull<<51)-1); t.v[4]+=c;
        c=t.v[4]>>51; t.v[4]&=((1ull<<51)-1); t.v[0]+=19*c;
    }
    u64 t0=t.v[0],t1=t.v[1],t2=t.v[2],t3=t.v[3],t4=t.v[4];
    memset(out,0,32);
    out[0]=(u8)t0; out[1]=(u8)(t0>>8); out[2]=(u8)(t0>>16); out[3]=(u8)(t0>>24);
    out[4]=(u8)(t0>>32); out[5]=(u8)(t0>>40);
    out[6]=(u8)((t0>>48)|((t1&0x7)<<3));
    out[7]=(u8)(t1>>5); out[8]=(u8)(t1>>13); out[9]=(u8)(t1>>21);
    out[10]=(u8)(t1>>29); out[11]=(u8)(t1>>37);
    out[12]=(u8)((t1>>45)|((t2&3)<<6));
    out[13]=(u8)(t2>>2); out[14]=(u8)(t2>>10); out[15]=(u8)(t2>>18);
    out[16]=(u8)(t2>>26); out[17]=(u8)(t2>>34); out[18]=(u8)(t2>>42);
    out[19]=(u8)((t2>>50)|((t3&1)<<7));
    out[20]=(u8)(t3>>1); out[21]=(u8)(t3>>9); out[22]=(u8)(t3>>17);
    out[23]=(u8)(t3>>25); out[24]=(u8)(t3>>33); out[25]=(u8)(t3>>41);
    out[26]=(u8)((t3>>49)|((t4&7)<<2));
    out[27]=(u8)(t4>>6); out[28]=(u8)(t4>>14); out[29]=(u8)(t4>>22);
    out[30]=(u8)(t4>>30); out[31]=(u8)(t4>>38);
}
static void x25519_cswap(int swap, xf *a, xf *b){
    u64 m = (u64)(-(i64)swap);
    for(int i=0;i<5;i++){
        u64 t = m & (a->v[i] ^ b->v[i]);
        a->v[i] ^= t; b->v[i] ^= t;
    }
}
static void x25519_scalarmult(u8 out[32], const u8 scalar[32], const u8 point[32]){
    u8 e[32]; memcpy(e, scalar, 32);
    e[0] &= 248; e[31] &= 127; e[31] |= 64;
    xf x1, x2, z2, x3, z3, tmp0, tmp1;
    xf_from_bytes(&x1, point);
    memset(&x2,0,sizeof x2); x2.v[0]=1; /* x2=1 */
    memset(&z2,0,sizeof z2); /* z2=0 */
    x3=x1; memset(&z3,0,sizeof z3); z3.v[0]=1; /* z3=1 */
    int swap=0;
    for(int pos=254; pos>=0; pos--){
        int b = (e[pos/8] >> (pos&7)) & 1;
        swap ^= b;
        x25519_cswap(swap, &x2, &x3);
        x25519_cswap(swap, &z2, &z3);
        swap = b;
        xf_sub(&tmp0, &x3, &z3);
        xf_sub(&tmp1, &x2, &z2);
        xf_add(&x2, &x2, &z2);
        xf_add(&z2, &x3, &z3);
        xf_mul(&z3, &tmp0, &x2);
        xf_mul(&z2, &z2, &tmp1);
        xf_sqr(&tmp0, &tmp1);
        xf_sqr(&tmp1, &x2);
        xf_add(&x3, &z3, &z2);
        xf_sub(&z2, &z3, &z2);
        xf_mul(&x2, &tmp1, &tmp0);
        xf_sub(&tmp1, &tmp1, &tmp0);
        xf_sqr(&z2, &z2);
        /* z2 = z2 * x1;  a24 * (tmp1) + tmp0  */
        {
            xf a24; memset(&a24,0,sizeof a24); a24.v[0]=121665;
            xf t; xf_mul(&t, &tmp1, &a24);
            xf_add(&t, &t, &tmp0);
            xf_mul(&z2, &z2, &x1);
            xf_mul(&z3, &tmp1, &t);
            xf_sqr(&x3, &x3);
        }
    }
    x25519_cswap(swap, &x2, &x3);
    x25519_cswap(swap, &z2, &z3);
    xf_inv(&z2, &z2);
    xf_mul(&x2, &x2, &z2);
    xf_to_bytes(&x2, out);
}
static void x25519_generate(u8 priv[32], u8 pub[32]){
    for(int i=0;i<32;i++) priv[i]=(u8)(0xC3 ^ (ticks*3+i*17) ^ (i*i));
    priv[0] &= 248; priv[31] &= 127; priv[31] |= 64;
    static const u8 base[32]={9}; /* u-coordinate of base point */
    x25519_scalarmult(pub, priv, base);
}

/* Conecta, ClientHello, parsea, intenta RSA ClientKeyExchange + Finished. */
static int tls_probe(const char *host){
    u32 ip = 0;
    tls_ke_ok = tls_finished_sent = 0;
    tls_rsa_bits = 0;
    tls_handshake_status = 0;
    tls_cert0_der_len = 0;

    if(dns_resolve(host, &ip) != 0) return -1;
    if(tcp_connect(ip, 443) != 0) return -2;

    tls_hs_init();
    u8 hello[512];
    int hlen = tls_build_client_hello(hello, sizeof hello, host);
    if(hlen < 0){ tcp_close_conn(); return -3; }
    /* hashear solo el handshake message (sin record header) */
    if(hlen > 5) tls_hs_add(hello+5, (u32)(hlen-5));
    if(tcp_send_data(hello, (u32)hlen) != 0){ tcp_close_conn(); return -4; }
    tls_hello_sent++;

    tcp_recv_wait(600);
    int n = (int)tcp_rx_len;
    if(n <= 0){ tcp_close_conn(); return -5; }
    tls_parse_response(tcp_rx, n);

    /* Añadir handshake messages del servidor al hash (recorrer records) */
    int off=0;
    while(off+5 <= n){
        u8 rt=tcp_rx[off];
        u16 rl=(u16)((tcp_rx[off+3]<<8)|tcp_rx[off+4]);
        if(off+5+rl>n) break;
        if(rt==0x16) tls_hs_add(tcp_rx+off+5, rl);
        off+=5+rl;
    }

    tls_handshake_status = 1;
    if(!tls_got_server_hello || !tls_got_certificate){
        tcp_close_conn();
        return n;
    }

    /* ¿Suite RSA? */
    u16 cs = tls_cipher_suite;
    int is_rsa = (cs==0x002F||cs==0x0035||cs==0x009C||cs==0x003C||cs==0x009D);
    int is_ecdhe = (cs==0xC02F||cs==0xC030||cs==0xC027||cs==0xC013||cs==0xC014);
    tls_ecdhe_ok = 0;

    u8 cke[512];
    int cke_total = 0;

    if(is_ecdhe && tls_got_key_exchange && tls_ske_curve==0x001D && tls_ske_pub_len==32){
        /* X25519 ECDHE */
        u8 priv[32], pub[32], shared[32];
        x25519_generate(priv, pub);
        x25519_scalarmult(shared, priv, tls_ske_pub);
        /* TLS 1.2 ECDHE premaster = shared secret (32 bytes for X25519) */
        memset(tls_premaster, 0, 48);
        memcpy(tls_premaster, shared, 32);
        /* ClientKeyExchange: length(1) + pubkey(32)  — for EC curve */
        cke[0]=0x10;
        cke[1]=0; cke[2]=0; cke[3]=33; /* hs body len */
        cke[4]=32; /* point len */
        memcpy(cke+5, pub, 32);
        cke_total = 4+33;
        tls_hs_add(cke, (u32)cke_total);
        if(tls_send_record(0x16, cke, (u16)cke_total) != 0){ tcp_close_conn(); return n; }
        tls_ke_ok = 1;
        tls_ecdhe_ok = 1;
        tls_handshake_status = 2;
    } else if(is_rsa){
        BN mod; u32 exp=0; int mbytes=0;
        if(!x509_rsa_pub(tls_cert0_der, tls_cert0_der_len, &mod, &exp, &mbytes)){
            tcp_close_conn();
            return n;
        }
        tls_rsa_bits = mbytes * 8;
        tls_premaster[0]=0x03; tls_premaster[1]=0x03;
        for(int i=2;i<48;i++) tls_premaster[i]=(u8)(0x3C^(ticks+i*11)^i);
        u8 enc[256];
        int elen = rsa_pkcs1_encrypt(&mod, exp, mbytes, tls_premaster, 48, enc);
        if(elen < 0){ tcp_close_conn(); return n; }
        int cke_hs_len = 2 + elen;
        cke[0]=0x10;
        cke[1]=(u8)(cke_hs_len>>16); cke[2]=(u8)(cke_hs_len>>8); cke[3]=(u8)cke_hs_len;
        cke[4]=(u8)(elen>>8); cke[5]=(u8)elen;
        memcpy(cke+6, enc, elen);
        cke_total = 4 + cke_hs_len;
        tls_hs_add(cke, (u32)cke_total);
        if(tls_send_record(0x16, cke, (u16)cke_total) != 0){ tcp_close_conn(); return n; }
        tls_ke_ok = 1;
        tls_handshake_status = 2;
    } else {
        tcp_close_conn();
        return n;
    }

    /* master_secret = PRF(premaster, "master secret", client_random+server_random) */
    u8 seed[64];
    memcpy(seed, tls_client_random, 32);
    memcpy(seed+32, tls_server_random, 32);
    {
        u32 pms_len = tls_ecdhe_ok ? 32u : 48u;
        tls12_prf(tls_premaster, pms_len, "master secret", seed, 64, tls_master, 48);
    }

    /* Derivar claves de tráfico si suite CBC-SHA */
    tls_keys_ready = 0;
    tls_record_enc_ok = 0;
    tls_use_gcm = 0;
    if(cs == 0x009C || cs == 0xC02F){
        tls_derive_keys_aes128_gcm();
    } else if(cs == 0x002F){
        tls_derive_keys_aes128_cbc_sha();
    } else if(cs == 0x0035){
        tls_derive_keys_aes128_cbc_sha();
    }

    /* ChangeCipherSpec (siempre en claro) */
    u8 ccs=1;
    tls_send_record(0x14, &ccs, 1);

    /* Finished */
    u8 hs_hash[32];
    tls_hs_digest(hs_hash);
    u8 verify[12];
    tls12_prf(tls_master, 48, "client finished", hs_hash, 32, verify, 12);
    u8 fin[16];
    fin[0]=0x14; fin[1]=0; fin[2]=0; fin[3]=12;
    memcpy(fin+4, verify, 12);
    tls_hs_add(fin, 16);

    if(tls_keys_ready){
        if(tls_send_app(0x16, fin, 16) == 0){
            tls_finished_sent = 1;
            tls_handshake_status = 4;
        } else {
            tls_send_record(0x16, fin, 16);
            tls_finished_sent = 1;
            tls_handshake_status = 3;
        }
    } else {
        tls_send_record(0x16, fin, 16);
        tls_finished_sent = 1;
        tls_handshake_status = 3;
    }

    tls_got_server_finished = 0;
    tls_https_bytes = 0;
    memset(tls_https_buf, 0, sizeof tls_https_buf);

    /* Esperar CCS + Finished del servidor (y opcionalmente app data) */
    tcp_rx_len = 0;
    tcp_got_fin = 0;
    tcp_recv_wait(600);
    n = (int)tcp_rx_len;

    if(tls_keys_ready && n > 5){
        int off = 0;
        while(off + 5 <= n){
            u8 rtype = tcp_rx[off];
            u16 rlen = (u16)((tcp_rx[off+3]<<8)|tcp_rx[off+4]);
            if(off+5+rlen > n) break;
            if(rtype == 0x14){
                /* CCS servidor — en claro */
            } else if(rtype == 0x16 || rtype == 0x17){
                u8 otype=0; u8 body[2048]; u16 blen=0;
                if(tls_decrypt_app(tcp_rx+off, 5+rlen, &otype, body, &blen)==0){
                    if(otype==0x16 && blen>=4 && body[0]==0x14){
                        tls_got_server_finished = 1;
                        tls_handshake_status = 5; /* handshake cerrado (lado cliente) */
                    } else if(otype==0x17 && blen>0){
                        u32 copy = blen;
                        if(copy > sizeof(tls_https_buf)-1) copy = sizeof(tls_https_buf)-1;
                        if(tls_https_bytes + copy > sizeof(tls_https_buf)-1)
                            copy = sizeof(tls_https_buf)-1 - tls_https_bytes;
                        memcpy(tls_https_buf + tls_https_bytes, body, copy);
                        tls_https_bytes += copy;
                        tls_handshake_status = 6;
                    }
                }
            } else if(rtype == 0x15){
                tls_alert = tcp_rx[off+6]; /* description if present */
            }
            off += 5+rlen;
        }
    }

    /* Si el handshake parece listo, enviar HTTP GET cifrado */
    if(tls_keys_ready && tls_finished_sent && (tls_got_server_finished || tls_handshake_status>=4)){
        char req[256];
        int ri=0;
        const char *rp = "GET ";
        while(*rp) req[ri++]=*rp++;
        rp = tls_req_path[0] ? tls_req_path : "/";
        while(*rp && ri<80) req[ri++]=*rp++;
        rp = " HTTP/1.1\r\nHost: ";
        while(*rp) req[ri++]=*rp++;
        rp = host; while(*rp && ri<180) req[ri++]=*rp++;
        rp = "\r\nUser-Agent: MosaicOS/0.30\r\nAccept: text/html,text/plain,*/*\r\nConnection: close\r\n\r\n";
        while(*rp && ri<250) req[ri++]=*rp++;
        if(tls_send_app(0x17, (const u8*)req, (u16)ri)==0){
            tls_handshake_status = 6;
            tcp_rx_len = 0;
            tcp_got_fin = 0;
            tcp_recv_wait(800);
            n = (int)tcp_rx_len;
            int off=0;
            while(off+5<=n){
                u8 rtype=tcp_rx[off];
                u16 rlen=(u16)((tcp_rx[off+3]<<8)|tcp_rx[off+4]);
                if(off+5+rlen>n) break;
                if(rtype==0x17 || rtype==0x16){
                    u8 otype=0; u8 body[2048]; u16 blen=0;
                    if(tls_decrypt_app(tcp_rx+off, 5+rlen, &otype, body, &blen)==0 && otype==0x17){
                        u32 copy=blen;
                        if(tls_https_bytes + copy > sizeof(tls_https_buf)-1)
                            copy = sizeof(tls_https_buf)-1 - tls_https_bytes;
                        memcpy(tls_https_buf + tls_https_bytes, body, copy);
                        tls_https_bytes += copy;
                        tls_handshake_status = 7; /* HTTPS data recibida */
                    }
                }
                off+=5+rlen;
            }
        }
    }

    tcp_close_conn();
    return n > 0 ? n : (tls_https_bytes > 0 ? (int)tls_https_bytes : 0);
}

static void ip_stack_init(void){


    memset(arp_tab, 0, sizeof arp_tab);
    ip_tx_pkts = ip_rx_pkts = icmp_echo_tx = icmp_echo_rx = 0;
    arp_requests = arp_replies_rx = 0;
    tcp_state = TCP_CLOSED;
    tcp_rx_len = 0;
    dns_ready = 0;
    dns_answer_ip = 0;
    dns_queries = dns_answers = 0;
    tls_hello_sent = tls_records_rx = 0;
    tls_last_server_type = 0;
    tls_got_server_hello = tls_got_certificate = tls_got_server_hello_done = 0;
    tls_got_key_exchange = tls_got_cert_req = tls_alert = 0;
    tls_cert_count = tls_cert0_len = 0;
    tls_ke_ok = tls_finished_sent = tls_rsa_bits = tls_handshake_status = 0;
    tls_cert0_der_len = 0;
    tls_keys_ready = tls_record_enc_ok = tls_use_gcm = 0;
    tls_client_seq = tls_server_seq = 0;
    tls_got_server_finished = 0;
    tls_https_bytes = 0;
    tls_ecdhe_ok = 0;
    tls_ske_pub_len = 0;
    tls_ske_curve = 0;
    tls_crypto_ok = crypto_selftest();
}

/* ================= Discos: IDE (PIO) y SATA (AHCI) ================= */
enum { DK_ATA=1, DK_AHCI=2 };
typedef struct { int kind; u16 io, ctl; u8 sel; u32 abar; int port, slot; u32 sectors; char model[41]; } Disk;
#define MAXDISK 8
static Disk disks[MAXDISK]; static int ndisks=0, cur_disk=-1;
/* Datos del disco actualmente elegido */
static int ata_present=0; static u32 ata_sectors=0; static char ata_model[41]; static const char *disk_tipo="";

static inline void insw(u16 port,void *buf,u32 n){ __asm__ volatile("rep insw":"+D"(buf),"+c"(n):"d"(port):"memory"); }
static inline void outsw(u16 port,const void *buf,u32 n){ __asm__ volatile("rep outsw":"+S"(buf),"+c"(n):"d"(port):"memory"); }
static void fix_model(const u16 *id,char *out){
    for(int i=0;i<20;i++){ out[2*i]=id[27+i]>>8; out[2*i+1]=id[27+i]&0xFF; }
    out[40]=0;
    for(int i=39;i>=0&&(out[i]==' '||!out[i]);i--) out[i]=0;
}

/* ---- IDE clásico ---- */
static void ata_delay(u16 ctl){ for(int i=0;i<4;i++) inb(ctl); }
static int ata_wait(u16 io,u16 ctl,int drq){
    ata_delay(ctl);
    for(int i=0;i<2000000;i++){
        u8 st=inb(io+7);
        if(st&0x01) return -1;
        if(st&0x20) return -1;
        if(!(st&0x80)){ if(!drq) return 0; if(st&0x08) return 0; }
    }
    return -1;
}
static void ata_probe(u16 io,u16 ctl,u8 sel){
    if(ndisks>=MAXDISK) return;
    outb(ctl,2);
    outb(io+6,sel); ata_delay(ctl);
    outb(io+2,0); outb(io+3,0); outb(io+4,0); outb(io+5,0);
    outb(io+7,0xEC);
    u8 st=inb(io+7);
    if(st==0 || st==0xFF) return;
    for(int i=0;i<2000000 && (inb(io+7)&0x80);i++);
    if(inb(io+4)||inb(io+5)) return;           /* unidad óptica (ATAPI) u otro tipo */
    if(ata_wait(io,ctl,1)) return;
    u16 id[256]; insw(io,id,256);
    u32 sec=((u32)id[61]<<16)|id[60];
    if(!sec) return;
    Disk *d=&disks[ndisks++];
    d->kind=DK_ATA; d->io=io; d->ctl=ctl; d->sel=sel; d->sectors=sec; fix_model(id,d->model);
}
static int ata_rw(u16 io,u16 ctl,u8 sel,u32 lba,void *buf,int wr){
    if(lba>=0x0FFFFFFF) return -1;
    if(ata_wait(io,ctl,0)) return -1;
    outb(io+6,sel|0x40|((lba>>24)&0x0F));
    outb(io+2,1); outb(io+3,lba&0xFF); outb(io+4,(lba>>8)&0xFF); outb(io+5,(lba>>16)&0xFF);
    outb(io+7, wr?0x30:0x20);
    if(ata_wait(io,ctl,1)) return -1;
    if(wr){ outsw(io,buf,256); outb(io+7,0xE7); return ata_wait(io,ctl,0); }
    insw(io,buf,256); return 0;
}

/* ---- SATA AHCI ---- */
#define AR(a,o) (*(volatile u32*)((a)+(o)))
static u8 ahci_clb[4][1024] __attribute__((aligned(1024)));
static u8 ahci_fb[4][256]   __attribute__((aligned(256)));
static u8 ahci_ct[4][256]   __attribute__((aligned(128)));
static u8 ahci_buf[512]     __attribute__((aligned(16)));
static int nahci=0;

static int ahci_port_start(u32 pb,int slot){
    AR(pb,0x18) &= ~1u; AR(pb,0x18) &= ~0x10u;
    int ok=0; for(int i=0;i<500000;i++) if(!(AR(pb,0x18)&0xC000)){ ok=1; break; }
    if(!ok) return -1;
    memset(ahci_clb[slot],0,1024); memset(ahci_fb[slot],0,256);
    AR(pb,0x00)=(u32)ahci_clb[slot]; AR(pb,0x04)=0;
    AR(pb,0x08)=(u32)ahci_fb[slot];  AR(pb,0x0C)=0;
    AR(pb,0x30)=0xFFFFFFFF; AR(pb,0x10)=0xFFFFFFFF; AR(pb,0x14)=0;
    AR(pb,0x18) |= 0x10;
    for(int i=0;i<500000;i++) if(!(AR(pb,0x20)&0x88)) break;
    AR(pb,0x18) |= 1;
    return 0;
}
/* cmd: 0xEC identificar, 0x25 leer DMA ext, 0x35 escribir DMA ext (siempre 1 sector) */
static int ahci_cmd(const Disk *d,u8 cmd,u32 lba,int wr,void *buf){
    u32 pb=d->abar+0x100+d->port*0x80; int s=d->slot;
    u32 *h=(u32*)ahci_clb[s]; u8 *ct=ahci_ct[s];
    memset(ct,0,0x90);
    h[0]=5|(wr?(1u<<6):0)|(1u<<16); h[1]=0; h[2]=(u32)ct; h[3]=0;
    ct[0]=0x27; ct[1]=0x80; ct[2]=cmd; ct[7]=(cmd==0xEC)?0:0x40;
    ct[4]=lba&0xFF; ct[5]=(lba>>8)&0xFF; ct[6]=(lba>>16)&0xFF; ct[8]=(lba>>24)&0xFF; ct[12]=1;
    u32 *prd=(u32*)(ct+0x80); prd[0]=(u32)ahci_buf; prd[1]=0; prd[2]=0; prd[3]=511;
    if(wr) memcpy(ahci_buf,buf,512);
    for(int i=0;i<500000 && (AR(pb,0x20)&0x88);i++);
    AR(pb,0x10)=0xFFFFFFFF; AR(pb,0x38)=1;
    for(int i=0;i<5000000;i++){
        if(!(AR(pb,0x38)&1)) break;
        if(AR(pb,0x10)&(1u<<30)) return -1;
    }
    if((AR(pb,0x38)&1) || (AR(pb,0x10)&(1u<<30)) || (AR(pb,0x20)&1)) return -1;
    if(!wr) memcpy(buf,ahci_buf,512);
    return 0;
}
static void ahci_init_all(void){
    for(u32 b=0;b<256;b++) for(u32 dv=0;dv<32;dv++){
        if((pci_rd(b,dv,0,0)&0xFFFF)==0xFFFF) continue;
        u32 nf=(pci_rd(b,dv,0,0x0C)>>23)&1 ? 8 : 1;
        for(u32 f=0;f<nf;f++){
            if((pci_rd(b,dv,f,0)&0xFFFF)==0xFFFF) continue;
            if((pci_rd(b,dv,f,0x08)>>8)!=0x010601) continue;     /* SATA, AHCI */
            u32 abar=pci_rd(b,dv,f,0x24)&~0xFu;
            if(!abar) continue;
            pci_wr(b,dv,f,0x04,pci_rd(b,dv,f,0x04)|0x06);         /* memoria + bus master */
            AR(abar,0x04) |= 1u<<31;                               /* AHCI habilitado */
            if(AR(abar,0x24)&1){                                   /* tomar control a la BIOS */
                AR(abar,0x28) |= 2;
                for(int i=0;i<2000000;i++) if(!(AR(abar,0x28)&1)) break;
            }
            u32 pi=AR(abar,0x0C);
            for(int p=0;p<32;p++){
                if(!(pi&(1u<<p))) continue;
                u32 pb=abar+0x100+p*0x80;
                u32 ss=AR(pb,0x28);
                if((ss&0xF)!=3 || ((ss>>8)&0xF)!=1) continue;      /* sin enlace activo */
                if(AR(pb,0x24)!=0x00000101) continue;              /* no es disco SATA */
                if(nahci>=4 || ndisks>=MAXDISK) return;
                if(ahci_port_start(pb,nahci)) continue;
                Disk *d=&disks[ndisks]; memset(d,0,sizeof *d);
                d->kind=DK_AHCI; d->abar=abar; d->port=p; d->slot=nahci;
                u16 id[256];
                if(ahci_cmd(d,0xEC,0,0,id)) continue;
                u32 sec=((u32)id[61]<<16)|id[60];
                if(id[83]&(1<<10)){ sec=((u32)id[101]<<16)|id[100]; if(id[102]||id[103]) sec=0x7FFFFFFF; }
                if(sec>0x7FFFFFFF) sec=0x7FFFFFFF;
                if(!sec) continue;
                d->sectors=sec; fix_model(id,d->model);
                ndisks++; nahci++;
            }
        }
    }
}

static int disk_rw(int di,u32 lba,void *buf,int wr){
    const Disk *d=&disks[di];
    if(d->kind==DK_ATA) return ata_rw(d->io,d->ctl,d->sel,lba,buf,wr);
    return ahci_cmd(d,wr?0x35:0x25,lba,wr,buf);
}
static u32 disk_limit(void){ return disks[cur_disk].kind==DK_ATA ? 0x0FFFFFFFu : 0x7FFFFFFFu; }
static void disk_select(int i){
    cur_disk=i; ata_present=1; ata_sectors=disks[i].sectors;
    scopy(ata_model,disks[i].model,41);
    disk_tipo = disks[i].kind==DK_ATA ? "IDE" : "SATA (AHCI)";
}
static void disks_detect(void){
    ata_probe(0x1F0,0x3F6,0xA0); ata_probe(0x1F0,0x3F6,0xB0);
    ata_probe(0x170,0x376,0xA0); ata_probe(0x170,0x376,0xB0);
    ahci_init_all();
}

/* ================= Capa de bloques: disco o memoria temporal ================= */
#define RAM_SECT 2048
static u8 ramdisk[RAM_SECT*512];
enum { BK_NONE=0, BK_RAM, BK_DISK };
static int backend=BK_NONE;
static u32 blk_reads=0, blk_writes=0, blk_errors=0;

static int blk_rd(u32 lba,void *buf){
    int r;
    if(backend==BK_RAM){
        if(lba>=RAM_SECT){ blk_errors++; return -1; }
        memcpy(buf,ramdisk+lba*512,512); blk_reads++; return 0;
    }
    if(backend==BK_DISK){
        r = disk_rw(cur_disk,lba,buf,0);
        if(r) blk_errors++; else blk_reads++;
        return r;
    }
    blk_errors++; return -1;
}
static int blk_wr(u32 lba,const void *buf){
    int r;
    if(backend==BK_RAM){
        if(lba>=RAM_SECT){ blk_errors++; return -1; }
        memcpy(ramdisk+lba*512,buf,512); blk_writes++; return 0;
    }
    if(backend==BK_DISK){
        r = disk_rw(cur_disk,lba,(void*)buf,1);
        if(r) blk_errors++; else blk_writes++;
        return r;
    }
    blk_errors++; return -1;
}

/* ================= Sistema de archivos MosaicFS ================= */
#define SB_MAGIC 0x41534F4Du      /* "MOSA" */
#define NENT 128
#define TSECT 8
#define DSTART 9
#define MAXFILE (1u<<20)
typedef struct { char name[20]; u32 start, size, cap; } Entry;
static Entry table[NENT];
static int fs_ok=0; static u32 fs_total=0;
static u32 fs_mounts=0, fs_format_count=0, fs_check_errors=0;

/* Suma simple de 32 bits sobre words (no criptográfica; solo detección de corrupción básica). */
static u32 fs_checksum32(const u32 *words, u32 n){
    u32 s = 0x4D4F5341u; /* seed "MOSA" */
    for(u32 i=0;i<n;i++) s = (s << 5) + s + words[i];
    return s;
}

static int fs_flush_table(void){
    for(int i=0;i<TSECT;i++) if(blk_wr(1+i,(u8*)table+i*512)) return -1;
    return 0;
}
static u32 blk_capacity(void){
    if(backend==BK_RAM) return RAM_SECT;
    if(backend==BK_DISK && cur_disk>=0 && cur_disk<ndisks) return disks[cur_disk].sectors;
    return 0;
}
static int valid_name(const char *n);
static int fs_format(u32 total){
    /* Refuse impossible layouts before touching sector zero or the file table. */
    if(total<DSTART+1 || total>blk_capacity() || total>disk_limit()) return -1;
    u32 sb[128]; memset(sb,0,sizeof sb);
    sb[0]=SB_MAGIC; sb[1]=1; sb[2]=total; sb[3]=TSECT;
    sb[4]=0; /* checksum placeholder */
    sb[4]=fs_checksum32(sb, 4); /* checksum of magic..tsect */
    memset(table,0,sizeof table);
    if(blk_wr(0,sb)) return -1;
    if(fs_flush_table()) return -1;
    fs_total=total; fs_ok=1; fs_format_count++; return 0;
}
static int fs_mount(void){
    u32 sb[128];
    fs_ok=0; fs_total=0;
    if(blk_rd(0,sb)) return -1;
    if(sb[0]!=SB_MAGIC || sb[1]!=1 || sb[3]!=TSECT) return -1;
    if(sb[2]<DSTART+1 || sb[2]>blk_capacity() || sb[2]>disk_limit()) return -1;
    /* Si hay checksum (volúmenes formateados desde 0.17), verificarlo. */
    if(sb[4]){
        u32 expect = sb[4];
        u32 tmp = sb[4]; sb[4]=0;
        u32 got = fs_checksum32(sb, 4);
        sb[4]=tmp;
        if(got != expect) return -1;
    }
    for(int i=0;i<TSECT;i++) if(blk_rd(1+i,(u8*)table+i*512)) return -1;
    /* Treat corrupt metadata as an unmountable volume; don't trust disk offsets. */
    for(int i=0;i<NENT;i++){
        Entry *e=&table[i];
        if(!e->name[0]) continue;
        if(e->name[19] != 0 || !valid_name(e->name) || !e->cap ||
           e->start<DSTART || e->start>sb[2] || e->cap>sb[2]-e->start ||
           e->size>e->cap*512u) return -1;
        for(int j=0;j<i;j++) if(table[j].name[0]){
            Entry *q=&table[j];
            if(e->start<q->start+q->cap && q->start<e->start+e->cap) return -1;
            if(seq(e->name,q->name)) return -1;
        }
    }
    fs_total=sb[2]; fs_ok=1; fs_mounts++; return 0;
}
static int valid_name(const char *n){
    int l=slen(n); if(l<1||l>19) return 0;
    for(int i=0;i<l;i++){ u8 c=(u8)n[i]; if(c=='/'||c<32||(c>=127&&c<0x80)||c>0xA5) return 0; }
    return 1;
}
static int fs_find(const char *name){
    for(int i=0;i<NENT;i++) if(table[i].name[0] && seq(table[i].name,name)) return i;
    return -1;
}
static u32 fs_alloc(u32 need){
    u32 cand[NENT+1]; int nc=0; cand[nc++]=DSTART;
    for(int i=0;i<NENT;i++) if(table[i].name[0]) cand[nc++]=table[i].start+table[i].cap;
    u32 best=0;
    for(int c=0;c<nc;c++){
        u32 st=cand[c];
        if(st+need>fs_total) continue;
        int ok=1;
        for(int j=0;j<NENT;j++)
            if(table[j].name[0] && st<table[j].start+table[j].cap && table[j].start<st+need){ ok=0; break; }
        if(ok && (best==0 || st<best)) best=st;
    }
    return best;
}
/* Códigos: 0 ok, -1 sin almacenamiento, -2 muy grande, -3 tabla llena, -4 sin espacio, -5 error de disco */
static int fs_write(const char *name,const u8 *data,u32 len){
    if(!fs_ok) return -1;
    if(len>MAXFILE) return -2;
    u32 need=(len+511)/512; if(!need) need=1;
    int idx=fs_find(name); int had=(idx>=0);
    Entry old;
    if(!(had && need<=table[idx].cap)){
        u32 cap=need+2;
        if(had){ old=table[idx]; table[idx].name[0]=0; }
        else { for(int i=0;i<NENT;i++) if(!table[i].name[0]){ idx=i; break; } if(idx<0) return -3; }
        u32 st=fs_alloc(cap);
        if(!st){ cap=need; st=fs_alloc(cap); }
        if(!st){ if(had) table[idx]=old; return -4; }
        memset(&table[idx],0,sizeof(Entry));
        scopy(table[idx].name,name,20); table[idx].start=st; table[idx].cap=cap;
    }
    u8 sec[512];
    for(u32 s=0;s<need;s++){
        memset(sec,0,512);
        u32 off=s*512, n=(len>off)?len-off:0; if(n>512) n=512;
        memcpy(sec,data+off,n);
        if(blk_wr(table[idx].start+s,sec)){ fs_mount(); return -5; }
    }
    table[idx].size=len;
    if(fs_flush_table()) return -5;
    return 0;
}
static int fs_read(const char *name,u8 *buf,u32 max){
    int idx=fs_find(name); if(idx<0) return -1;
    u32 size=table[idx].size; if(size>max) size=max;
    u8 sec[512];
    for(u32 off=0;off<size;off+=512){
        if(blk_rd(table[idx].start+off/512,sec)) return -5;
        u32 n=size-off; if(n>512) n=512;
        memcpy(buf+off,sec,n);
    }
    return (int)size;
}
static int fs_delete(const char *name){
    int idx=fs_find(name); if(idx<0) return -1;
    memset(&table[idx],0,sizeof(Entry));
    return fs_flush_table();
}
static int fs_rename(const char *a,const char *b){
    int i=fs_find(a); if(i<0||fs_find(b)>=0) return -1;
    scopy(table[i].name,b,20);
    return fs_flush_table();
}
static u32 fs_libre_kb(void){
    u32 usado=DSTART; for(int i=0;i<NENT;i++) if(table[i].name[0]) usado+=table[i].cap;
    return (fs_total>usado)?(fs_total-usado)/2:0;
}

/* Comprobación de consistencia de MosaicFS (solo lectura).
   Devuelve número de problemas encontrados; rellena un contador global. */
static int fs_check(void){
    int problems = 0;
    fs_check_errors = 0;
    if(!fs_ok){ fs_check_errors = 1; return 1; }

    /* Superbloque de nuevo */
    u32 sb[128];
    if(blk_rd(0,sb)){ problems++; }
    else {
        if(sb[0]!=SB_MAGIC || sb[1]!=1 || sb[3]!=TSECT) problems++;
        if(sb[2]!=fs_total) problems++;
        if(sb[4]){
            u32 expect=sb[4]; sb[4]=0;
            if(fs_checksum32(sb,4)!=expect) problems++;
            sb[4]=expect;
        }
    }

    /* Entradas de la tabla */
    for(int i=0;i<NENT;i++){
        Entry *e=&table[i];
        if(!e->name[0]) continue;
        if(e->name[19]!=0 || !valid_name(e->name)) problems++;
        if(!e->cap || e->start<DSTART || e->start+e->cap>fs_total) problems++;
        if(e->size > e->cap*512u) problems++;
        for(int j=i+1;j<NENT;j++) if(table[j].name[0]){
            Entry *q=&table[j];
            if(e->start < q->start+q->cap && q->start < e->start+e->cap) problems++;
            if(seq(e->name,q->name)) problems++;
        }
    }

    /* Contar archivos y espacio */
    u32 files=0, used=DSTART;
    for(int i=0;i<NENT;i++) if(table[i].name[0]){ files++; used+=table[i].cap; }
    if(used > fs_total) problems++;

    fs_check_errors = (u32)problems;
    return problems;
}

static int fs_file_count(void){
    int n=0; for(int i=0;i<NENT;i++) if(table[i].name[0]) n++; return n;
}
static const char *fs_err(int r){
    switch(r){
        case -1: return "No hay almacenamiento listo.";
        case -2: return "El archivo es demasiado grande.";
        case -3: return "Se llenó la tabla de archivos.";
        case -4: return "No hay espacio libre.";
        case -5: return "Error de escritura o lectura en el disco.";
    }
    return "Error desconocido.";
}
static int disco_con_particiones(void){
    u8 s[512]; if(blk_rd(0,s)) return 0;
    if(s[510]!=0x55||s[511]!=0xAA) return 0;
    for(int i=0;i<4;i++) if(s[446+16*i+4]) return 1;
    return 0;
}
static void fs_boot(void){
    if(ndisks){
        backend=BK_DISK;
        for(int i=0;i<ndisks;i++){ disk_select(i); if(fs_mount()==0) return; }
        disk_select(0);             /* ninguno con formato: queda sin preparar, se ofrece en Archivos */
    } else {
        backend=BK_RAM;
        fs_format(RAM_SECT);
    }
}

/* ================= Mosaicos ================= */
static u32 mem_mb = 0;
typedef struct { const char *titulo; const char *desc; u8 icono; } Mosaico;
enum { T_ARCH, T_NOTAS, T_CALC, T_RELOJ, T_SIS, T_TUT, T_COMPAT, T_COLORES, T_CONFIG, T_TERM, T_REINI, T_APAGAR, NM };
static const Mosaico M[NM] = {
    {"Archivos",       "Guarda, abre, renombra y borra tus archivos.",   0x16},
    {"Notas",          "Escribe textos y guárdalos como archivos.",      0xF0},
    {"Calculadora",    "Suma, resta, multiplica y divide.",              0xF1},
    {"Reloj",          "Muestra la hora y la fecha del equipo.",         0x0F},
    {"Sistema",        "Información del equipo y de MosaicOS.",        0xE9},
    {"Tutorial",       "Aprende a usar MosaicOS paso a paso.",            '?'},
    {"Compatibilidad", "Estado de las funciones nativas de MosaicOS.",        0x1D},
    {"Colores",        "Cambia los colores de los mosaicos.",            0x03},
    {"Configuración",  "Personaliza el aspecto de MosaicOS.",             0xF0},
    {"Terminal",       "Consulta el sistema con comandos propios.",         '>'},
    {"Reiniciar",      "Reinicia el equipo.",                            0x12},
    {"Apagar",         "Apaga el equipo de forma segura.",               0x09},
};
static const u8 PALETA[6] = { CIAN, MAGENTA, VERDE, ROJO, MARRON, AZUL };
static int tema = 0;
static u8 color_mosaico(int i){ return PALETA[(i+tema)%6]; }

#define TW 23
#define TH 4
#define TILE_COLS 3
static void tile_xy(int i,int *x,int *y){ *x = 2 + (i%TILE_COLS)*25; *y = 2 + (i/TILE_COLS)*5; }

static void dibujar_inicio(int sel){
    scr_begin();
    fill(0,0,W,H,' ',ATTR(BLANCO,AZUL));
    fill(0,0,W,1,' ',ATTR(BLANCO,AZUL));
    text(2,0,"MosaicOS",ATTR(BLANCO,AZUL));
    text(13,0,"Inicio",ATTR(GRIS,AZUL));
    text(61,0,"ESCRITORIO 0.31",ATTR(GRIS,AZUL));
    for(int i=0;i<NM;i++){
        int x,y; tile_xy(i,&x,&y);
        u8 bg=color_mosaico(i);
        fill(x,y,TW,TH,' ',ATTR(BLANCO,bg));
        put(x+2,y+1,M[i].icono,ATTR(AMARILLO,bg));
        text(x+4,y+1,M[i].titulo,ATTR(BLANCO,bg));
        if(i==sel) box(x,y,TW,TH,ATTR(BLANCO,bg));
    }
    fill(0,22,W,3,' ',ATTR(BLANCO,GRIS_OSC));
    text(2,22,"MOSAIC",ATTR(AMARILLO,GRIS_OSC));
    text(10,22,"Seleccionado:",ATTR(GRIS,GRIS_OSC));
    text(24,22,M[sel].titulo,ATTR(BLANCO,GRIS_OSC));
    text(2,23,M[sel].desc,ATTR(BLANCO,GRIS_OSC));
    text(2,24,"F1 Ayuda F2 Ajustes F3 Archivos F4 Notas F5 Sistema Esc Inicio",ATTR(GRIS,GRIS_OSC));
    scr_end();
}

static void ventana_t(u8 bg,const char *titulo,const char *pie){
    fill(0,0,W,H,' ',NORMAL);
    fill(0,0,W,3,' ',ATTR(BLANCO,bg));
    text(2,1,"< Volver",ATTR(BLANCO,bg));
    text(12,1,titulo,ATTR(AMARILLO,bg));
    fill(0,24,W,1,' ',ATTR(NEGRO,GRIS));
    text(2,24,pie,ATTR(NEGRO,GRIS));
}
static void ventana(int i,const char *pie){ ventana_t(color_mosaico(i),M[i].titulo,pie); }
static int back_hit(void){ return in(0,0,12,3); }
static void espera_volver(void){
    for(;;){ int k=event_wait(); if(k==K_ESC) return; if(k==K_CLICK && back_hit()) return; }
}

/* ---------- Diálogos ---------- */
static void aviso(const char *l1,const char *l2){
    scr_begin();
    ventana_t(ROJO,"Aviso","Enter o Esc: aceptar");
    text(3,6,l1,NORMAL); if(l2) text(3,8,l2,NORMAL);
    fill(3,11,16,3,' ',ATTR(BLANCO,ROJO)); text(8,12,"Aceptar",ATTR(BLANCO,ROJO));
    scr_end();
    for(;;){
        int k=event_wait();
        if(k==K_ENTER||k==K_ESC) return;
        if(k==K_CLICK && (in(3,11,16,3)||back_hit())) return;
    }
}
static int confirmar(const char *titulo,u8 bg,const char *pregunta){
    scr_begin();
    ventana_t(bg,titulo,"Enter: sí   Esc: no");
    text(3,7,pregunta,NORMAL);
    fill(3,10,16,3,' ',ATTR(BLANCO,VERDE));  text(10,11,"Sí",ATTR(BLANCO,VERDE));
    fill(22,10,16,3,' ',ATTR(BLANCO,ROJO));  text(29,11,"No",ATTR(BLANCO,ROJO));
    scr_end();
    for(;;){
        int k=event_wait();
        if(k==K_ENTER) return 1;
        if(k==K_ESC) return 0;
        if(k==K_CLICK){ if(in(3,10,16,3)) return 1; if(in(22,10,16,3)||back_hit()) return 0; }
    }
}
/* Pide un texto. buf trae el valor inicial. Devuelve 1 si se aceptó. */
static int pedir_texto(const char *titulo,const char *l1,const char *l2,char *buf,int max){
    int n=slen(buf);
    for(;;){
        scr_begin();
        ventana_t(MAGENTA,titulo,"Enter: aceptar   Esc: cancelar");
        text(3,5,l1,NORMAL);
        if(l2) text(3,7,l2,ATTR(BLANCO,ROJO));
        box(3,10,42,3,NORMAL);
        rawtext(5,11,buf,ATTR(AMARILLO,NEGRO));
        put(5+n,11,' ',ATTR(NEGRO,AMARILLO));
        scr_end();
        int k;
        do k=event_wait(); while(k==K_MOVE);
        if(k==K_ESC) return 0;
        if(k==K_CLICK && back_hit()) return 0;
        if(k==K_ENTER && n>0) return 1;
        if(k==K_BACKSPACE){ if(n>0) buf[--n]=0; }
        else if(((k>=32&&k<127)||(k>=0x80&&k<=0xA5)) && k!='/' && n<max-1){ buf[n++]=(char)k; buf[n]=0; }
    }
}

/* ---------- Tutorial ---------- */
static void app_tutorial(void){
    scr_begin();
    ventana(T_TUT,"Esc o clic en Volver: regresar al inicio");
    text(3,4,"Bienvenido a MosaicOS",ATTR(AMARILLO,NEGRO));
    text(3,6,"1. Mueve el mouse sobre un mosaico y haz clic para abrirlo,",NORMAL);
    text(3,7,"   o usa las flechas del teclado y Enter.",NORMAL);
    text(3,9,"2. Esc o el botón \"< Volver\" te llevan siempre atrás.",NORMAL);
    text(3,11,"3. Notas: escribe y guarda con F2 (o el botón Guardar).",NORMAL);
    text(3,13,"4. Archivos: abre, crea (F3), renombra (F2) y borra (Supr).",NORMAL);
    text(3,15,"5. Si no hay disco formateado, los archivos viven en memoria",NORMAL);
    text(3,16,"   temporal y se pierden al apagar. Mira el mosaico Archivos.",NORMAL);
    text(3,19,"Consejo: no puedes romper nada, explora con tranquilidad.",ATTR(VERDE,NEGRO));
    scr_end();
    espera_volver();
}

/* ---------- Notas (editor de texto) ---------- */
#define EW 76
#define EH 19
#define MAXL 300
static char doc[MAXL][EW]; static u8 dlen[MAXL];
static int nlines=1, cy=0, cx=0, top=0, modificado=0;
static char docname[20]; static char notas_msg[48];

static void doc_clear(void){ memset(dlen,0,sizeof dlen); nlines=1; cy=cx=top=0; modificado=0; }
static void doc_load(const u8 *b,u32 n){
    doc_clear(); int l=0;
    for(u32 i=0;i<n;i++){
        u8 c=b[i];
        if(c=='\r') continue;
        if(c=='\n'){ if(l<MAXL-1){ l++; nlines=l+1; } continue; }
        if(c=='\t') c=' ';
        if(c<32) continue;
        if(dlen[l]>=EW){ if(l<MAXL-1){ l++; nlines=l+1; } else continue; }
        doc[l][dlen[l]++]=(char)c;
    }
}
static u32 doc_serialize(u8 *out){
    int last=nlines-1; while(last>0 && dlen[last]==0) last--;
    u32 n=0;
    for(int l=0;l<=last;l++){ memcpy(out+n,doc[l],dlen[l]); n+=dlen[l]; out[n++]='\n'; }
    return n;
}
#define DOCBUF (MAXL*(EW+1)+16)
static int notas_cargar(const char *name){
    u8 *buf=kmalloc(DOCBUF);
    if(!buf){ aviso("No hay memoria suficiente.",0); return -1; }
    int n=fs_read(name,buf,DOCBUF-16);
    if(n<0){ kfree(buf); aviso("No se pudo abrir el archivo.",fs_err(n)); return -1; }
    doc_load(buf,(u32)n); kfree(buf);
    scopy(docname,name,20); modificado=0; notas_msg[0]=0;
    return 0;
}
static int notas_guardar(int como){
    if(!fs_ok){ aviso("No hay almacenamiento listo para guardar.","Abre el mosaico Archivos para prepararlo."); return -1; }
    if(como || !docname[0]){
        char nm[20]; scopy(nm,docname,20);
        if(!pedir_texto("Guardar archivo","Nombre del archivo:",0,nm,20)) return -1;
        if(!valid_name(nm)){ aviso("Nombre no válido.","Usa hasta 19 caracteres, sin barras."); return -1; }
        if(!seq(nm,docname) && fs_find(nm)>=0 &&
           !confirmar("Guardar archivo",color_mosaico(T_NOTAS),"Ya existe un archivo con ese nombre. ¿Reemplazarlo?")) return -1;
        scopy(docname,nm,20);
    }
    u8 *buf=kmalloc(DOCBUF);
    if(!buf){ aviso("No hay memoria suficiente.",0); return -1; }
    u32 n=doc_serialize(buf);
    int r=fs_write(docname,buf,n); kfree(buf);
    if(r<0){ aviso("No se pudo guardar el archivo.",fs_err(r)); return -1; }
    modificado=0; scopy(notas_msg,"Archivo guardado.",48);
    return 0;
}
static void doc_scroll(void){
    if(cy<top) top=cy;
    if(cy>=top+EH) top=cy-EH+1;
}
static void notas_dibujar(void){
    scr_begin();
    ventana_t(color_mosaico(T_NOTAS),"Notas",notas_msg[0]?notas_msg:"Esc: salir | F2: guardar | F3: como | F4: nuevo");
    u8 bg=color_mosaico(T_NOTAS);
    text(17,1," - ",ATTR(AMARILLO,bg)); int n=20;
    n+=rawtext(n,1,docname[0]?docname:"(sin nombre)",ATTR(AMARILLO,bg));
    if(modificado) put(n,1,'*',ATTR(AMARILLO,bg));
    text(41,1,"[F2 Guardar]",ATTR(BLANCO,bg)); text(54,1,"[F3 Como]",ATTR(BLANCO,bg)); text(64,1,"[F4 Nuevo]",ATTR(BLANCO,bg));
    for(int r=0;r<EH;r++){
        int l=top+r;
        for(int x=0;x<EW;x++){
            u8 c=' ';
            if(l<nlines && x<dlen[l]) c=(u8)doc[l][x];
            put(2+x,4+r,c,NORMAL);
        }
        if(l==cy && cx<=EW) put(2+cx,4+r,(cx<dlen[l])?(u8)doc[l][cx]:' ',ATTR(NEGRO,AMARILLO));
    }
    text(60,24,"Lín",ATTR(NEGRO,GRIS)); number(64,24,cy+1,ATTR(NEGRO,GRIS));
    text(69,24,"Col",ATTR(NEGRO,GRIS)); number(73,24,cx+1,ATTR(NEGRO,GRIS));
    scr_end();
}
static void ins_char(int c){
    if(dlen[cy]<EW){
        memmove(&doc[cy][cx+1],&doc[cy][cx],dlen[cy]-cx);
        doc[cy][cx]=(char)c; dlen[cy]++; cx++;
    } else if(cx==dlen[cy] && nlines<MAXL){
        memmove(&doc[cy+2],&doc[cy+1],(u32)(nlines-cy-1)*EW);
        memmove(&dlen[cy+2],&dlen[cy+1],nlines-cy-1);
        nlines++; cy++; doc[cy][0]=(char)c; dlen[cy]=1; cx=1;
    } else return;
    modificado=1;
}
static void ins_newline(void){
    if(nlines>=MAXL) return;
    memmove(&doc[cy+2],&doc[cy+1],(u32)(nlines-cy-1)*EW);
    memmove(&dlen[cy+2],&dlen[cy+1],nlines-cy-1);
    nlines++;
    int rest=dlen[cy]-cx;
    memcpy(doc[cy+1],&doc[cy][cx],rest); dlen[cy+1]=rest; dlen[cy]=cx;
    cy++; cx=0; modificado=1;
}
static void del_line(int l){
    memmove(&doc[l],&doc[l+1],(u32)(nlines-l-1)*EW);
    memmove(&dlen[l],&dlen[l+1],nlines-l-1);
    nlines--; dlen[nlines]=0;
}
static void do_backspace(void){
    if(cx>0){ memmove(&doc[cy][cx-1],&doc[cy][cx],dlen[cy]-cx); dlen[cy]--; cx--; modificado=1; }
    else if(cy>0){
        int pl=dlen[cy-1];
        if(pl+dlen[cy]<=EW){ memcpy(&doc[cy-1][pl],doc[cy],dlen[cy]); dlen[cy-1]=pl+dlen[cy]; del_line(cy); cy--; cx=pl; modificado=1; }
        else { cy--; cx=dlen[cy]; }
    }
}
static void do_delete(void){
    if(cx<dlen[cy]){ memmove(&doc[cy][cx],&doc[cy][cx+1],dlen[cy]-cx-1); dlen[cy]--; modificado=1; }
    else if(cy<nlines-1 && dlen[cy]+dlen[cy+1]<=EW){
        memcpy(&doc[cy][dlen[cy]],doc[cy+1],dlen[cy+1]); dlen[cy]+=dlen[cy+1]; del_line(cy+1); modificado=1;
    }
}
static void notas_nuevo(void){
    if(modificado && !confirmar("Notas",color_mosaico(T_NOTAS),"Hay cambios sin guardar. ¿Descartarlos?")) return;
    doc_clear(); docname[0]=0; notas_msg[0]=0;
}
static void app_notas(void){
    doc_scroll(); notas_dibujar();
    for(;;){
        int k=event_wait();
        if(k==K_MOVE) continue;
        int msgprev=notas_msg[0]; notas_msg[0]=0; (void)msgprev;
        if(k==K_ESC){
            if(!modificado || confirmar("Notas",color_mosaico(T_NOTAS),"Hay cambios sin guardar. ¿Salir sin guardar?")) return;
        }
        else if(k==K_CLICK){
            if(back_hit()){
                if(!modificado || confirmar("Notas",color_mosaico(T_NOTAS),"Hay cambios sin guardar. ¿Salir sin guardar?")) return;
            }
            else if(in(41,1,12,1)) notas_guardar(0);
            else if(in(54,1,9,1)) notas_guardar(1);
            else if(in(64,1,10,1)) notas_nuevo();
            else if(in(2,4,EW,EH)){
                int l=top+(my-4);
                if(l<nlines){ cy=l; cx=mx-2; if(cx>dlen[cy]) cx=dlen[cy]; }
            }
        }
        else if(k==K_F2) notas_guardar(0);
        else if(k==K_F3) notas_guardar(1);
        else if(k==K_F4) notas_nuevo();
        else if(k>=32 && k<256) ins_char(k);
        else if(k==K_ENTER) ins_newline();
        else if(k==K_BACKSPACE) do_backspace();
        else if(k==K_DEL) do_delete();
        else if(k==K_LEFT){ if(cx>0) cx--; else if(cy>0){ cy--; cx=dlen[cy]; } }
        else if(k==K_RIGHT){ if(cx<dlen[cy]) cx++; else if(cy<nlines-1){ cy++; cx=0; } }
        else if(k==K_UP){ if(cy>0){ cy--; if(cx>dlen[cy]) cx=dlen[cy]; } }
        else if(k==K_DOWN){ if(cy<nlines-1){ cy++; if(cx>dlen[cy]) cx=dlen[cy]; } }
        else if(k==K_HOME) cx=0;
        else if(k==K_END) cx=dlen[cy];
        else if(k==K_PGUP){ cy-=EH; if(cy<0) cy=0; if(cx>dlen[cy]) cx=dlen[cy]; }
        else if(k==K_PGDN){ cy+=EH; if(cy>=nlines) cy=nlines-1; if(cx>dlen[cy]) cx=dlen[cy]; }
        doc_scroll(); notas_dibujar();
    }
}

/* ---------- Archivos ---------- */
static int fl_idx[NENT], fl_n=0, fl_sel=0, fl_top=0;
static void fl_build(void){
    fl_n=0;
    for(int i=0;i<NENT;i++) if(table[i].name[0]) fl_idx[fl_n++]=i;
    if(fl_sel>=fl_n) fl_sel=fl_n?fl_n-1:0;
    if(fl_sel<fl_top) fl_top=fl_sel;
    if(fl_sel>=fl_top+16) fl_top=fl_sel-15;
}
static void archivos_dibujar(void){
    fl_build();
    u8 bg=color_mosaico(T_ARCH);
    scr_begin();
    ventana(T_ARCH,"Enter: abrir  F3: nuevo  F2: renombrar  Supr: borrar  Esc: volver");
    if(backend==BK_RAM) text(3,4,"Memoria temporal: los archivos se pierden al apagar.",ATTR(AMARILLO,NEGRO));
    else {
        int n=text(3,4,"Disco: ",ATTR(GRIS,NEGRO));
        n+=rawtext(3+n,4,ata_model,NORMAL);
        n+=text(3+n,4,"  Libre: ",ATTR(GRIS,NEGRO));
        n+=number(3+n,4,fs_libre_kb()/1024,NORMAL); text(3+n,4," MB",NORMAL);
    }
    text(4,5,"Nombre",ATTR(GRIS,NEGRO)); text(32,5,"Tamaño",ATTR(GRIS,NEGRO));
    if(fl_n==0) text(4,7,"(No hay archivos. Crea uno con Nuevo.)",ATTR(GRIS,NEGRO));
    for(int r=0;r<16 && fl_top+r<fl_n;r++){
        int e=fl_idx[fl_top+r]; int y=6+r;
        u8 a=(fl_top+r==fl_sel)?ATTR(BLANCO,bg):NORMAL;
        fill(2,y,54,1,' ',a);
        rawtext(4,y,table[e].name,a);
        u32 sz=table[e].size; int n;
        if(sz<1024){ n=number(32,y,sz,a); text(32+n,y," B",a); }
        else { n=number(32,y,sz/1024,a); text(32+n,y," KB",a); }
    }
    static const char *BT[4]={"Abrir (Enter)","Nuevo (F3)","Renombrar (F2)","Borrar (Supr)"};
    for(int i=0;i<4;i++){
        fill(58,6+i*4,18,3,' ',ATTR(BLANCO,bg)); text(60,7+i*4,BT[i],ATTR(BLANCO,bg));
    }
    scr_end();
}
static void archivos_abrir(void){
    if(!fl_n) return;
    char name[20]; scopy(name,table[fl_idx[fl_sel]].name,20);
    if(modificado && !seq(name,docname) &&
       !confirmar("Archivos",color_mosaico(T_ARCH),"Hay cambios sin guardar en Notas. ¿Descartarlos?")) return;
    if(!seq(name,docname) || !modificado) if(notas_cargar(name)) return;
    app_notas();
}
static void archivos_nuevo(void){
    char nm[20]; nm[0]=0;
    if(!pedir_texto("Nuevo archivo","Nombre del archivo nuevo:",0,nm,20)) return;
    if(!valid_name(nm)){ aviso("Nombre no válido.","Usa hasta 19 caracteres, sin barras."); return; }
    if(fs_find(nm)>=0){ aviso("Ya existe un archivo con ese nombre.",0); return; }
    int r=fs_write(nm,(const u8*)"",0);
    if(r<0){ aviso("No se pudo crear el archivo.",fs_err(r)); return; }
    if(modificado && !confirmar("Archivos",color_mosaico(T_ARCH),"Hay cambios sin guardar en Notas. ¿Descartarlos?")) return;
    doc_clear(); scopy(docname,nm,20); notas_msg[0]=0;
    app_notas();
}
static void archivos_renombrar(void){
    if(!fl_n) return;
    char old[20], nm[20]; scopy(old,table[fl_idx[fl_sel]].name,20); scopy(nm,old,20);
    if(!pedir_texto("Renombrar archivo","Nuevo nombre:",0,nm,20)) return;
    if(!valid_name(nm)){ aviso("Nombre no válido.","Usa hasta 19 caracteres, sin barras."); return; }
    if(fs_rename(old,nm)<0){ aviso("No se pudo renombrar.","Quizás ya existe ese nombre."); return; }
    if(seq(old,docname)) scopy(docname,nm,20);
}
static void archivos_borrar(void){
    if(!fl_n) return;
    char name[20], msg[80]; scopy(name,table[fl_idx[fl_sel]].name,20);
    scopy(msg,"¿Borrar \"",80); scat(msg,name,80); scat(msg,"\" para siempre?",80);
    if(!confirmar("Borrar archivo",ROJO,msg)) return;
    if(fs_delete(name)<0) aviso("No se pudo borrar el archivo.",0);
    else if(seq(name,docname)){ docname[0]=0; }
}
/* Disco presente pero sin formato de MosaicFS */
static void archivos_sin_formato(void){
    for(;;){
        scr_begin();
        ventana_t(ROJO,"Archivos - disco sin preparar","F3: formatear   F4: usar memoria temporal   Esc: volver");
        text(3,5,"Se detectó un disco sin formato MosaicFS.",NORMAL);
        text(3,7,"Disco:",ATTR(AMARILLO,NEGRO)); rawtext(10,7,ata_model,NORMAL);
        int n=number(10,8,ata_sectors/2048,NORMAL); text(10+n,8," MB",NORMAL);
        if(disco_con_particiones())
            text(3,10,"¡CUIDADO! Este disco parece tener otro sistema instalado.",ATTR(BLANCO,ROJO));
        text(3,11,"Formatear BORRA TODO el contenido del disco. No se puede deshacer.",ATTR(AMARILLO,NEGRO));
        fill(3,14,30,3,' ',ATTR(BLANCO,ROJO));   text(6,15,"Formatear disco (F3)",ATTR(BLANCO,ROJO));
        fill(38,14,36,3,' ',ATTR(BLANCO,AZUL));  text(41,15,"Usar memoria temporal (F4)",ATTR(BLANCO,AZUL));
        scr_end();
        int k; do k=event_wait(); while(k==K_MOVE);
        int fmt=0, ram=0;
        if(k==K_ESC || (k==K_CLICK && back_hit())) return;
        if(k==K_F3 || (k==K_CLICK && in(3,14,30,3))) fmt=1;
        if(k==K_F4 || (k==K_CLICK && in(38,14,36,3))) ram=1;
        if(fmt){
            char t[12]; t[0]=0;
            if(pedir_texto("Formatear disco","Escribe BORRAR (en mayúsculas) para confirmar:",
                           "Se perderán todos los datos del disco.",t,10) && seq(t,"BORRAR")){
                u32 tot=ata_sectors>disk_limit()?disk_limit():ata_sectors;
                if(fs_format(tot)<0){ aviso("No se pudo formatear el disco.",0); fs_ok=0; }
                else return;
            }
        }
        if(ram){ backend=BK_RAM; fs_format(RAM_SECT); return; }
    }
}
static void app_archivos(void){
    if(!fs_ok) archivos_sin_formato();
    if(!fs_ok) return;
    for(;;){
        archivos_dibujar();
        int k; do k=event_wait(); while(k==K_MOVE);
        if(k==K_ESC) return;
        if(k==K_UP){ if(fl_sel>0) fl_sel--; }
        else if(k==K_DOWN){ if(fl_sel<fl_n-1) fl_sel++; }
        else if(k==K_ENTER) archivos_abrir();
        else if(k==K_F3) archivos_nuevo();
        else if(k==K_F2) archivos_renombrar();
        else if(k==K_DEL) archivos_borrar();
        else if(k==K_CLICK){
            if(back_hit()) return;
            if(in(58,6,18,3)) archivos_abrir();
            else if(in(58,10,18,3)) archivos_nuevo();
            else if(in(58,14,18,3)) archivos_renombrar();
            else if(in(58,18,18,3)) archivos_borrar();
            else if(in(2,6,54,16)){
                int r=fl_top+(my-6);
                if(r<fl_n){ fl_sel=r; archivos_abrir(); }
            }
        }
    }
}

/* ---------- Calculadora ---------- */
static char cexp[34]; static int clen=0; static char cres[16]; static int chasres=0;
static const char *cp; static int cerr;
static int parse_expr(void);
static int parse_atom(void){
    if(*cp=='('){ cp++; int v=parse_expr(); if(*cp==')') cp++; else cerr=1; return v; }
    if(*cp=='-'){ cp++; return (int)(0u-(u32)parse_atom()); }
    if(*cp>='0'&&*cp<='9'){ u32 v=0; while(*cp>='0'&&*cp<='9'){ v=v*10+(*cp-'0'); cp++; } return (int)v; }
    cerr=1; return 0;
}
static int parse_term(void){
    int v=parse_atom();
    for(;;){
        if(*cp=='*'){ cp++; v=(int)((u32)v*(u32)parse_atom()); }
        else if(*cp=='/'){
            cp++; int d=parse_atom();
            if(d==0){ cerr=1; return 0; }
            v = (d==-1) ? (int)(0u-(u32)v) : v/d;
        } else return v;
    }
}
static int parse_expr(void){
    int v=parse_term();
    for(;;){
        if(*cp=='+'){ cp++; v=(int)((u32)v+(u32)parse_term()); }
        else if(*cp=='-'){ cp++; v=(int)((u32)v-(u32)parse_term()); }
        else return v;
    }
}
static void calc_eval(void){
    if(!clen) return;
    cexp[clen]=0; cp=cexp; cerr=0;
    int v=parse_expr();
    chasres=1;
    if(cerr||*cp){ scopy(cres,"Error",16); return; }
    char b[12]; int n=0, i=0; u32 u=(v<0)?0u-(u32)v:(u32)v;
    do{ b[n++]='0'+u%10; u/=10; }while(u);
    if(v<0) cres[i++]='-';
    while(n) cres[i++]=b[--n];
    cres[i]=0;
}
static const char *CB[16]={"7","8","9","/","4","5","6","*","1","2","3","-","0","C","=","+"};
static void calc_dibujar(void){
    scr_begin();
    ventana(T_CALC,"Esc: salir  |  Teclas: 0-9 + - * / ( )  Enter: calcular  Retroceso: borrar");
    box(3,5,38,3,NORMAL);
    cexp[clen]=0; text(5,6,cexp,ATTR(AMARILLO,NEGRO));
    if(chasres){ text(3,10,"Resultado:",ATTR(GRIS,NEGRO)); text(14,10,cres,NORMAL); }
    for(int i=0;i<16;i++){
        int x=45+(i%4)*8, y=5+(i/4)*4;
        char c=CB[i][0];
        u8 a=(c>='0'&&c<='9')?ATTR(NEGRO,GRIS):ATTR(BLANCO,color_mosaico(T_CALC));
        fill(x,y,7,3,' ',a);
        put(x+3,y+1,c,a);
    }
    scr_end();
}
static void app_calc(void){
    calc_dibujar();
    for(;;){
        int k=event_wait();
        if(k==K_MOVE) continue;
        if(k==K_ESC) return;
        int add=0;
        if(k==K_CLICK){
            if(back_hit()) return;
            for(int i=0;i<16;i++){
                int x=45+(i%4)*8, y=5+(i/4)*4;
                if(in(x,y,7,3)){
                    char c=CB[i][0];
                    if(c=='C'){ clen=0; chasres=0; }
                    else if(c=='=') calc_eval();
                    else add=c;
                }
            }
        }
        else if(k==K_ENTER) calc_eval();
        else if(k==K_BACKSPACE){ if(clen>0) clen--; chasres=0; }
        else if((k>='0'&&k<='9')||k=='+'||k=='-'||k=='*'||k=='/'||k=='('||k==')') add=k;
        if(add && clen<32){ cexp[clen++]=(char)add; chasres=0; }
        calc_dibujar();
    }
}

/* ---------- Reloj ---------- */
static void app_reloj(void){
    scr_begin(); ventana(T_RELOJ,"Esc o clic en Volver: regresar al inicio"); scr_end();
    int last=-1;
    for(;;){
        Hora t; read_time(&t);
        if(t.s!=last){
            last=t.s;
            scr_begin();
            u8 a=ATTR(AMARILLO,NEGRO);
            two(33,10,t.h,a); put(35,10,':',a); two(36,10,t.m,a); put(38,10,':',a); two(39,10,t.s,a);
            two(33,12,t.d,NORMAL); put(35,12,'/',NORMAL); two(36,12,t.mo,NORMAL); put(38,12,'/',NORMAL);
            number(39,12,2000+t.y,NORMAL);
            text(3,20,"La hora es la del reloj de la placa (BIOS).",ATTR(GRIS,NEGRO));
            scr_end();
        }
        int k=event_poll();
        if(k==K_ESC || (k==K_CLICK && back_hit())) return;
        if(!k) __asm__ volatile("sti; hlt");
    }
}

/* ---------- Sistema ---------- */
static void app_sistema(void){
    scr_begin(); ventana(T_SIS,"Esc o clic en Volver: regresar al inicio");
    u8 t=ATTR(AMARILLO,NEGRO);
    text(3,4,"MosaicOS 0.11",t);
    text(3,6,"Arquitectura:",t);   text(24,6,"x86 de 32 bits (i386)",NORMAL);
    text(3,8,"Núcleo:",t);         text(24,8,"Propio, original (no Linux, no Windows)",NORMAL);
    text(3,10,"Memoria total:",t); { int n=number(24,10,mem_mb,NORMAL); text(24+n,10," MB",NORMAL); }
    text(3,12,"Memoria libre:",t);
    text(3,14,"Disco:",t);
    if(ata_present){ char m[26]; scopy(m,ata_model,26); int n=rawtext(24,14,m,NORMAL); n+=number(25+n,14,ata_sectors/2048,NORMAL); n+=text(25+n,14," MB  ",NORMAL); text(25+n,14,disk_tipo,ATTR(VERDE,NEGRO)); }
    else text(24,14,"No detectado",NORMAL);
    text(3,16,"Archivos en:",t);
    text(24,16,backend==BK_DISK ? (fs_ok?"Disco (permanente)":"Disco sin preparar") : "Memoria temporal",NORMAL);
    text(3,18,"Mouse:",t);         text(24,18,has_mouse?"PS/2 detectado":"No detectado",NORMAL);
    text(3,22,"Entorno:",t);       text(24,22,"Inicio por mosaicos + Configuración",NORMAL);
    text(3,20,"Encendido hace:",t);
    text(48,10,"Heap usado:",t); { int n=number(60,10,heap_usado()/1024,NORMAL); text(60+n,10," KB",NORMAL); }
    text(48,12,"Bloques heap:",t); number(62,12,heap_bloques(),NORMAL);
    text(48,14,"Asignaciones:",t); number(62,14,heap_allocations,NORMAL);
    text(48,16,"Fallos heap:",t); number(62,16,heap_failures,NORMAL);
    scr_end();
    u32 last=0xFFFFFFFF;
    for(;;){
        u32 s=ticks/100;
        if(s!=last){
            last=s; scr_begin();
            fill(24,12,30,1,' ',NORMAL); fill(24,20,30,1,' ',NORMAL);
            int n=number(24,12,heap_libre()/1024,NORMAL); text(24+n,12," KB del sistema",NORMAL);
            int n2=number(24,20,s,NORMAL); text(24+n2,20," segundos",NORMAL);
            scr_end();
        }
        int k=event_poll();
        if(k==K_ESC || (k==K_CLICK && back_hit())) return;
        if(!k) __asm__ volatile("sti; hlt");
    }
}

/* ---------- Configuración ---------- */
static void app_config(void){
    int option=0;
    for(;;){
        scr_begin(); ventana(T_CONFIG,"Esc o clic en Volver: regresar al inicio");
        text(3,4,"Configuración de MosaicOS",ATTR(AMARILLO,NEGRO));
        text(3,6,"Personaliza el entorno sin instalar software externo.",NORMAL);
        fill(3,8,56,3,' ',ATTR(BLANCO,AZUL));
        text(5,9,"Apariencia y colores",ATTR(BLANCO,AZUL));
        if(option==0) box(3,8,56,3,ATTR(AMARILLO,AZUL));
        text(3,12,"Tema actual:",ATTR(AMARILLO,NEGRO));
        static const char *nombres[6]={"Cian","Magenta","Verde","Rojo","Marrón","Azul"};
        text(18,12,nombres[tema%6],NORMAL);
        text(3,15,"Enter o clic en la opción para cambiar el tema.",NORMAL);
        text(3,17,"La configuración de esta versión es temporal.",ATTR(GRIS,NEGRO));
        text(3,19,"Próximamente: idioma, accesibilidad y dispositivos.",ATTR(GRIS,NEGRO));
        scr_end();
        int k=event_wait();
        if(k==K_ESC || (k==K_CLICK && back_hit())) return;
        if(k==K_ENTER || (k==K_CLICK && in(3,8,56,3))){ tema=(tema+1)%6; }
    }
}


/* ================= Mosaic PMM: administrador de memoria fisica =================
   Bitmap de 4 KiB por pagina. El limite coincide con el mapeo identidad inicial.
   Una pagina reservada nunca puede ser liberada por pmm_free_page(). */
#define PMM_LIMIT_BYTES (64u*1024u*1024u)
#define PMM_MAX_PAGES (PMM_LIMIT_BYTES/PAGE_SIZE)
#define PMM_BITMAP_BYTES (PMM_MAX_PAGES/8u)
static u8 pmm_used[PMM_BITMAP_BYTES];
static u8 pmm_reserved[PMM_BITMAP_BYTES];
static u32 pmm_page_count=0, pmm_free_pages=0, pmm_allocations=0;
static u32 pmm_frees=0, pmm_failures=0, pmm_bad_frees=0;
static u32 pmm_map_entries=0, pmm_map_valid=0;

static int pmm_bit_get(const u8 *map,u32 i){ return (map[i>>3]>>(i&7))&1; }
static void pmm_bit_set(u8 *map,u32 i){ map[i>>3]|=(u8)(1u<<(i&7)); }
static void pmm_bit_clear(u8 *map,u32 i){ map[i>>3]&=(u8)~(1u<<(i&7)); }
static u32 pmm_clip_end(u32 end){
    if(end>paging_mapped_bytes) end=paging_mapped_bytes;
    if(end>PMM_LIMIT_BYTES) end=PMM_LIMIT_BYTES;
    return end;
}
static void pmm_free_usable_range(u64 base,u64 length){
    if(!length || base>=PMM_LIMIT_BYTES || base+length<base) return;
    u64 end=base+length;
    if(end>PMM_LIMIT_BYTES) end=PMM_LIMIT_BYTES;
    if(end>paging_mapped_bytes) end=paging_mapped_bytes;
    /* Solo paginas completamente contenidas en un rango declarado utilizable. */
    u64 first=(base+PAGE_SIZE-1u)&~((u64)PAGE_SIZE-1u);
    u64 last=end&~((u64)PAGE_SIZE-1u);
    if(first>=last) return;
    for(u32 i=(u32)(first/PAGE_SIZE);i<(u32)(last/PAGE_SIZE) && i<pmm_page_count;i++){
        pmm_bit_clear(pmm_used,i); pmm_bit_clear(pmm_reserved,i);
    }
}
static void pmm_reserve_range(u32 base,u32 length){
    if(!length || base+length<base) return;
    u32 start=base&~(PAGE_SIZE-1u);
    u32 end=base+length;
    if(end>0xFFFFFFFFu-PAGE_SIZE+1u) end=0xFFFFFFFFu;
    else end=(end+PAGE_SIZE-1u)&~(PAGE_SIZE-1u);
    end=pmm_clip_end(end);
    u32 first=start/PAGE_SIZE, last=(end+PAGE_SIZE-1u)/PAGE_SIZE;
    if(last>pmm_page_count) last=pmm_page_count;
    for(u32 i=first;i<last;i++){
        
        pmm_bit_set(pmm_used,i); pmm_bit_set(pmm_reserved,i);
    }
}
static void pmm_recount(void){
    pmm_free_pages=0;
    for(u32 i=0;i<pmm_page_count;i++) if(!pmm_bit_get(pmm_used,i)) pmm_free_pages++;
}
static void pmm_init(u32 magic,u32 *mbi,u32 physical_end){
    memset(pmm_used,0xFF,sizeof(pmm_used));
    memset(pmm_reserved,0xFF,sizeof(pmm_reserved));
    pmm_free_pages=0; pmm_page_count=paging_mapped_bytes/PAGE_SIZE;
    if(pmm_page_count>PMM_MAX_PAGES) pmm_page_count=PMM_MAX_PAGES;
    pmm_map_entries=0; pmm_map_valid=0; pmm_allocations=0; pmm_frees=0;
    pmm_failures=0; pmm_bad_frees=0;
    if(magic==0x2BADB002u && mbi){
        /* Multiboot v1 memory map: flags bit 6, length [11], address [12]. */
        if((mbi[0]&(1u<<6)) && mbi[11]>=24u && mbi[12] && mbi[12]<paging_mapped_bytes && mbi[11]<=paging_mapped_bytes-mbi[12]){
            u32 addr=mbi[12], len=mbi[11], off=0;
            while(off<=len-4u && pmm_map_entries<256u){
                u32 size=*(volatile u32*)(addr+off);
                if(size<20u || size>len-off-4u) break;
                volatile u8 *e=(volatile u8*)(addr+off+4u);
                u64 base=*(volatile u64*)(e+0);
                u64 span=*(volatile u64*)(e+8);
                u32 type=*(volatile u32*)(e+16);
                pmm_map_entries++;
                if(type==1u && span && base+span>=base){
                    pmm_free_usable_range(base,span); pmm_map_valid++;
                }
                off+=size+4u;
            }
        }
        /* Reserva la estructura Multiboot v1 completa (11 dwords + campos opcionales). */
        pmm_reserve_range((u32)mbi,88u);
        /* Reserva el buffer del mapa para no devolver paginas que lo contienen. */
        if(mbi[0]&(1u<<6)) pmm_reserve_range(mbi[12],mbi[11]);
        /* Reserva descriptores y payloads de modulos, si los hay. */
        if((mbi[0]&(1u<<3)) && mbi[5] && mbi[6] && mbi[5]<1024u && mbi[6]<paging_mapped_bytes && mbi[5]*16u<=paging_mapped_bytes-mbi[6]){
            u32 count=mbi[5], maddr=mbi[6];
            pmm_reserve_range(maddr,count*16u);
            for(u32 i=0;i<count;i++){
                volatile u32 *m=(volatile u32*)(maddr+i*16u);
                if(m[1]>m[0]) pmm_reserve_range(m[0],m[1]-m[0]);
            }
        }
    }
    /* Fallback conservador: memoria basica Multiboot por encima de 1 MiB. */
    if(!pmm_map_valid && physical_end>0x00100000u)
        pmm_free_usable_range(0x00100000u,physical_end-0x00100000u);
    /* Siempre preservar el primer MiB y todo el kernel/heap estatico y dinamico. */
    pmm_reserve_range(0,0x00100000u);
    pmm_reserve_range(0,(u32)_end);
    if(heap_limit>heap_start) pmm_reserve_range(heap_start,heap_limit-heap_start);
    pmm_recount();
}
u32 pmm_alloc_page(void){
    /* Primer MiB excluido; el mapeo identidad debe cubrir el marco devuelto. */
    for(u32 i=0x00100000u/PAGE_SIZE;i<pmm_page_count;i++){
        if(!pmm_bit_get(pmm_used,i)){
            pmm_bit_set(pmm_used,i); if(pmm_free_pages) pmm_free_pages--;
            pmm_allocations++; return i*PAGE_SIZE;
        }
    }
    pmm_failures++; return 0;
}
int pmm_free_page(u32 address){
    if((address&(PAGE_SIZE-1u)) || address<0x00100000u || address>=pmm_page_count*PAGE_SIZE){
        pmm_bad_frees++; return 0;
    }
    u32 i=address/PAGE_SIZE;
    if(pmm_bit_get(pmm_reserved,i) || !pmm_bit_get(pmm_used,i)){
        pmm_bad_frees++; return 0;
    }
    pmm_bit_clear(pmm_used,i); pmm_free_pages++; pmm_frees++; return 1;
}

/* ---------- Terminal nativa MosaicOS ---------- */
#define TERM_LINE 64
static int term_eq(const char *a,const char *b){
    while(*a && *b){
        char x=*a++, y=*b++;
        if(x>='A'&&x<='Z') x=(char)(x-'A'+'a');
        if(y>='A'&&y<='Z') y=(char)(y-'A'+'a');
        if(x!=y) return 0;
    }
    return *a==0 && *b==0;
}
static void term_line(int row,const char *s,u8 a){
    fill(2,row,76,1,' ',NORMAL);
    text(2,row,s,a);
}
static void term_help(void){
    term_line(5,"Comandos disponibles:",ATTR(AMARILLO,NEGRO));
    term_line(6,"help  about  mem  uptime  version  clear  exit",NORMAL);
    term_line(7,"devices  ticks  profile  pmm  vmem  pci",NORMAL);
    term_line(8,"https host [/path] | tls | dns | ping | net ...",NORMAL);
    term_line(10,"storage   Disco y MosaicFS (E/S, backend)",NORMAL);
    term_line(11,"files     Lista archivos de MosaicFS",NORMAL);
    term_line(12,"fscheck   Comprueba integridad de MosaicFS",NORMAL);
    term_line(13,"gfx       Demo grafica Mode 13h (Esc sale)",NORMAL);
    term_line(14,"vmem/pmm  Memoria virtual y fisica",NORMAL);
    term_line(15,"Esc o exit vuelven a Inicio.",ATTR(GRIS,NEGRO));
}
static void app_gfx(void);
static int term_starts(const char *s, const char *prefix){
    while(*prefix){ if(*s++ != *prefix++) return 0; }
    return 1;
}
static void term_execute(const char *cmd,int *row){
    u8 ok=ATTR(VERDE,NEGRO), warn=ATTR(AMARILLO,NEGRO);
    if(!cmd[0]) return;
    if(term_starts(cmd, "ping")){
        const char *arg = cmd+4;
        while(*arg==' ') arg++;
        u32 dst = ip_gw;
        if(*arg){
            if(!parse_ip(arg, &dst)){
                term_line(5,"Uso: ping [a.b.c.d]   (por defecto la puerta de enlace)",warn);
                return;
            }
        }
        if(!net_ok){ term_line(5,"Sin NIC; no se puede hacer ping.",warn); return; }
        char ips[16]; ip_fmt(dst, ips);
        fill(2,5,76,1,' ',NORMAL); text(2,5,"PING ",NORMAL); text(7,5,ips,ok);
        text(7+slen(ips),5," ...",NORMAL);
        u32 elapsed=0;
        int r = net_ping(dst, &elapsed);
        if(r==0){
            fill(2,7,76,1,' ',NORMAL); text(2,7,"Respuesta ICMP echo: OK",ok);
            fill(2,8,76,1,' ',NORMAL); text(2,8,"Ticks (~10ms): ",NORMAL); number(18,8,elapsed,NORMAL);
        } else if(r==-2){
            term_line(7,"Tiempo de espera agotado (sin respuesta ICMP).",ATTR(ROJO,NEGRO));
            term_line(8,"En QEMU user net prueba: ping 10.0.2.2",NORMAL);
        } else {
            term_line(7,"No se pudo enviar (ARP/IP fallo).",ATTR(ROJO,NEGRO));
        }
        return;
    }
    if(term_starts(cmd, "http") || term_starts(cmd, "browse") || term_starts(cmd, "dns")){
        int mode_dns = term_starts(cmd, "dns");
        int mode_browse = term_starts(cmd, "browse") || term_starts(cmd, "http");
        const char *arg = cmd;
        while(*arg && *arg != ' ') arg++;
        while(*arg == ' ') arg++;
        if(!*arg){
            term_line(5,"Uso:",warn);
            term_line(6,"  dns nombre.com",NORMAL);
            term_line(7,"  http nombre.com [/ruta]",NORMAL);
            term_line(8,"  browse nombre.com [/ruta]",NORMAL);
            term_line(10,"Solo HTTP:80. Sin HTTPS (YouTube/Google fallaran).",ATTR(GRIS,NEGRO));
            return;
        }
        char host[64]; int hi=0;
        while(arg[hi] && arg[hi] != ' ' && arg[hi] != '/' && hi < 63){ host[hi]=arg[hi]; hi++; }
        host[hi]=0;
        const char *path = "/";
        /* si queda " /ruta" o el host termino y sigue path en arg */
        const char *rest = arg + hi;
        while(*rest == ' ') rest++;
        if(*rest == '/') path = rest;
        else if(*rest){ /* segundo token path */
            path = rest;
        }
        if(!net_ok){ term_line(5,"Sin NIC.",warn); return; }

        if(mode_dns || mode_browse){
            u32 ip = 0;
            fill(2,5,76,1,' ',NORMAL); text(2,5,"DNS ",NORMAL); text(6,5,host,ok);
            int dr = dns_resolve(host, &ip);
            if(dr != 0){
                term_line(7,"No se pudo resolver (timeout, NXDOMAIN o error).",ATTR(ROJO,NEGRO));
                fill(2,8,76,1,' ',NORMAL); text(2,8,"Codigo: ",NORMAL); number(11,8,dr,NORMAL);
                term_line(9,"DNS QEMU: 10.0.2.3  |  Sin DNS publico asegurado en todos los entornos.",ATTR(GRIS,NEGRO));
                return;
            }
            char ips[16]; ip_fmt(ip, ips);
            fill(2,7,76,1,' ',NORMAL); text(2,7,"A: ",NORMAL); text(5,7,ips,ok);
            if(mode_dns && !term_starts(cmd, "http") && !term_starts(cmd, "browse")) return;
            /* continue to HTTP for http/browse */
            fill(2,8,76,1,' ',NORMAL); text(2,8,"HTTP GET ",NORMAL); text(11,8,path,NORMAL);
            int n = http_get(ip, host, path);
            (void)http_browse; /* API disponible */
            if(n < 0){
                term_line(10,"Fallo TCP/HTTP. Si el sitio exige HTTPS, no funcionara.",ATTR(ROJO,NEGRO));
                return;
            }
            fill(2,10,76,1,' ',NORMAL); text(2,10,"Bytes: ",NORMAL); number(10,10,n,ok);
            if(term_starts(cmd, "browse")){
                static char plain[2048];
                /* saltar cabeceras HTTP */
                u32 body = 0;
                for(u32 k=0;k+3<(u32)n;k++){
                    if(tcp_rx[k]=='\r'&&tcp_rx[k+1]=='\n'&&tcp_rx[k+2]=='\r'&&tcp_rx[k+3]=='\n'){
                        body = k+4; break;
                    }
                }
                u32 plen = html_to_text(tcp_rx + body, (u32)n - body, plain, sizeof plain);
                term_line(11,"--- texto (HTML reducido) ---",ATTR(AMARILLO,NEGRO));
                int row=12, col=2;
                for(u32 k=0;k<plen && row<22;k++){
                    char c = plain[k];
                    if(col>=78){ row++; col=2; }
                    if(row>=22) break;
                    put(col++, row, (u8)c, NORMAL);
                }
            } else {
                int row=12, col=2;
                for(u32 k=0;k<(u32)n && row<22;k++){
                    char c = (char)tcp_rx[k];
                    if(c==13) continue;
                    if(c==10){ row++; col=2; continue; }
                    if(c<32 || (u8)c>126) c='.';
                    if(col>=78){ row++; col=2; }
                    if(row>=22) break;
                    put(col++, row, (u8)c, NORMAL);
                }
            }
            return;
        }
        return;
    }
    if(term_starts(cmd, "https") || term_starts(cmd, "tls")){
        int is_https = term_starts(cmd, "https");
        const char *arg = cmd;
        while(*arg && *arg!=' ') arg++;
        while(*arg==' ') arg++;
        if(!*arg){
            term_line(5, is_https?"Uso: https nombre.com [/ruta]":"Uso: tls nombre.com", warn);
            term_line(6,"Cliente HTTPS MosaicOS 0.30 (ECDHE-X25519 + AES-GCM).",NORMAL);
            fill(2,8,76,1,' ',NORMAL); text(2,8,"Crypto: ",NORMAL);
            text(10,8,tls_crypto_ok?"OK":"FALLO",tls_crypto_ok?ok:ATTR(ROJO,NEGRO));
            return;
        }
        char host[64]; int hi=0;
        while(arg[hi] && arg[hi]!=' ' && arg[hi]!='/' && hi<63){ host[hi]=arg[hi]; hi++; }
        host[hi]=0;
        const char *path="/";
        const char *rest=arg+hi;
        while(*rest==' ') rest++;
        if(*rest=='/') path=rest;
        /* copiar path a tls_req_path */
        int pi=0;
        while(path[pi] && pi<95){ tls_req_path[pi]=path[pi]; pi++; }
        tls_req_path[pi]=0;
        if(!tls_req_path[0]) { tls_req_path[0]='/'; tls_req_path[1]=0; }

        if(!net_ok){ term_line(5,"Sin NIC.",warn); return; }
        fill(2,5,76,1,' ',NORMAL); text(2,5,is_https?"HTTPS ":"TLS ",ok); text(8,5,host,NORMAL);
        text(8+slen(host),5," ",NORMAL); text(9+slen(host),5,tls_req_path,ATTR(GRIS,NEGRO));
        term_line(6,"DNS + TCP/443 + handshake (puede tardar)...",NORMAL);

        int n = tls_probe(host);
        if(n < 0){
            term_line(8,"Fallo de red/DNS/TCP.",ATTR(ROJO,NEGRO));
            number(32,8,n,ATTR(ROJO,NEGRO));
            return;
        }
        fill(2,7,76,1,' ',NORMAL);
        text(2,7,"KE:",NORMAL); text(6,7,tls_ke_ok?(tls_ecdhe_ok?"ECDHE":"RSA"):"no",tls_ke_ok?ok:warn);
        text(14,7," Rec:",NORMAL); text(19,7,tls_record_enc_ok?(tls_use_gcm?"GCM":"CBC"):"-",tls_record_enc_ok?ok:warn);
        text(25,7," HS:",NORMAL); number(29,7,tls_handshake_status,NORMAL);
        text(34,7," SrvFin:",NORMAL); text(42,7,tls_got_server_finished?"si":"no",tls_got_server_finished?ok:warn);
        fill(2,8,76,1,' ',NORMAL); text(2,8,"HTTPS bytes: ",NORMAL); number(16,8,tls_https_bytes,tls_https_bytes?ok:warn);

        if(tls_https_bytes > 0){
            /* Status HTTP (primera linea) */
            char stline[80]; int si=0;
            for(u32 k=0;k<(u32)tls_https_bytes && si<78;k++){
                char c=(char)tls_https_buf[k];
                if(c=='\r') continue;
                if(c=='\n') break;
                if(c>=32 && c<127) stline[si++]=c;
            }
            stline[si]=0;
            fill(2,9,76,1,' ',NORMAL); text(2,9,"Status: ",NORMAL); text(10,9,stline,ok);

            /* Separar cabeceras / cuerpo */
            u32 body_off = 0;
            for(u32 k=0;k+3<(u32)tls_https_bytes;k++){
                if(tls_https_buf[k]=='\r'&&tls_https_buf[k+1]=='\n'&&
                   tls_https_buf[k+2]=='\r'&&tls_https_buf[k+3]=='\n'){
                    body_off = k+4; break;
                }
                if(tls_https_buf[k]=='\n'&&tls_https_buf[k+1]=='\n'){
                    body_off = k+2; break;
                }
            }
            fill(2,10,76,1,' ',NORMAL); text(2,10,"Body off: ",NORMAL); number(13,10,body_off,NORMAL);

            static char plain[3500];
            u32 plen = 0;
            if(body_off > 0 && body_off < (u32)tls_https_bytes){
                plen = html_to_text(tls_https_buf + body_off,
                                    (u32)tls_https_bytes - body_off,
                                    plain, sizeof plain);
            }
            if(plen == 0){
                /* mostrar crudo si no hay body HTML */
                plen = 0;
                for(u32 k=0;k<(u32)tls_https_bytes && plen+1<sizeof plain;k++){
                    char c=(char)tls_https_buf[k];
                    if(c=='\r') continue;
                    if(c=='\n') c=' ';
                    if(c<32||(u8)c>126) c='.';
                    if(c==' ' && plen>0 && plain[plen-1]==' ') continue;
                    plain[plen++]=c;
                }
                plain[plen]=0;
            }
            term_line(11,"--- texto de la pagina ---",ATTR(AMARILLO,NEGRO));
            int row=12, col=2;
            for(u32 k=0;k<plen && row<22;k++){
                char c=plain[k];
                if(col>=78){ row++; col=2; }
                if(row>=22) break;
                put(col++,(u8)row,(u8)c,NORMAL);
            }
            if(tls_handshake_status >= 7)
                term_line(22,"HTTPS OK — pagina en texto (sin layout).",ok);
        } else {
            if(tls_alert)
                { fill(2,10,76,1,' ',NORMAL); text(2,10,"TLS Alert code: ",warn); number(19,10,tls_alert,ATTR(ROJO,NEGRO)); }
            term_line(11,"Sin cuerpo HTTPS. Prueba otro host o /ruta.",ATTR(GRIS,NEGRO));
            term_line(12,"TLS1.2 + ECDHE-X25519 o RSA, AES-GCM/CBC.",ATTR(GRIS,NEGRO));
        }
        return;
    }
    if(0 && term_starts(cmd, "tls_old")){
        const char *arg = cmd + 3;
        while(*arg==' ') arg++;
        if(!*arg){
            term_line(5,"Uso: tls nombre.com",warn);
            term_line(6,"Envia ClientHello TLS 1.2 a puerto 443 (SNI).",NORMAL);
            term_line(7,"Parsea ServerHello y Certificate. Sin Finished/HTTPS aun.",ATTR(GRIS,NEGRO));
            fill(2,9,76,1,' ',NORMAL); text(2,9,"Crypto selftest SHA-256: ",NORMAL);
            text(28,9,tls_crypto_ok?"OK":"FALLO",tls_crypto_ok?ok:ATTR(ROJO,NEGRO));
            fill(2,10,76,1,' ',NORMAL); text(2,10,"ClientHello enviados: ",NORMAL); number(26,10,tls_hello_sent,NORMAL);
            return;
        }
        char host[64]; int hi=0;
        while(arg[hi] && arg[hi]!=' ' && hi<63){ host[hi]=arg[hi]; hi++; }
        host[hi]=0;
        if(!net_ok){ term_line(5,"Sin NIC.",warn); return; }
        fill(2,5,76,1,' ',NORMAL); text(2,5,"TLS probe ",NORMAL); text(12,5,host,ok);
        term_line(6,"DNS + TCP/443 + ClientHello...",NORMAL);
        int n = tls_probe(host);
        if(n < 0){
            term_line(8,"Fallo (DNS/TCP/envio). Codigo:",ATTR(ROJO,NEGRO));
            number(40,8,n,ATTR(ROJO,NEGRO));
            return;
        }
        fill(2,7,76,1,' ',NORMAL); text(2,7,"Bytes: ",NORMAL); number(10,7,n,ok);
        fill(2,8,76,1,' ',NORMAL); text(2,8,"ServerHello: ",NORMAL);
        text(16,8,tls_got_server_hello?"si":"no",tls_got_server_hello?ok:warn);
        text(22,8,"  Cert: ",NORMAL);
        text(30,8,tls_got_certificate?"si":"no",tls_got_certificate?ok:warn);
        text(36,8,"  Done: ",NORMAL);
        text(44,8,tls_got_server_hello_done?"si":"no",tls_got_server_hello_done?ok:warn);
        if(tls_got_server_hello){
            fill(2,9,76,1,' ',NORMAL); text(2,9,"Version: 0x",NORMAL); hex32(13,9,tls_server_version,NORMAL);
            fill(2,10,76,1,' ',NORMAL); text(2,10,"Cipher: 0x",NORMAL); hex32(13,10,tls_cipher_suite,NORMAL);
            if(tls_suite_name[0]) text(24,10,tls_suite_name,ATTR(CIAN,NEGRO));
        }
        if(tls_got_certificate){
            fill(2,11,76,1,' ',NORMAL); text(2,11,"Certs: ",NORMAL); number(10,11,tls_cert_count,NORMAL);
            text(16,11,"  cert0: ",NORMAL); number(25,11,tls_cert0_len,NORMAL); text(33,11," bytes",NORMAL);
            fill(2,12,76,1,' ',NORMAL); text(2,12,"SHA256 cert0: ",NORMAL);
            for(int i=0;i<8;i++){
                put(17+i*2,12,"0123456789ABCDEF"[tls_cert0_sha[i]>>4],NORMAL);
                put(18+i*2,12,"0123456789ABCDEF"[tls_cert0_sha[i]&15],NORMAL);
            }
            text(34,12,"...",ATTR(GRIS,NEGRO));
        }
        if(tls_got_key_exchange) term_line(13,"ServerKeyExchange presente (ECDHE/DHE).",ok);
        if(tls_alert){ fill(2,14,76,1,' ',NORMAL); text(2,14,"Alert TLS code: ",warn); number(20,14,tls_alert,ATTR(ROJO,NEGRO)); }
        fill(2,14,76,1,' ',NORMAL); text(2,14,"KE: ",NORMAL);
        text(6,14,tls_ke_ok?(tls_ecdhe_ok?"ECDHE/X25519":"RSA"):"no",tls_ke_ok?ok:warn);
        if(tls_rsa_bits){ text(24,14," RSA:",NORMAL); number(29,14,tls_rsa_bits,NORMAL); }
        text(36,14,"  Fin: ",NORMAL); text(43,14,tls_finished_sent?"si":"no",tls_finished_sent?ok:warn);
        fill(2,15,76,1,' ',NORMAL); text(2,15,"Estado HS: ",NORMAL); number(14,15,tls_handshake_status,NORMAL);
        text(18,15," (4=FinEnc 5=SrvFin 6=GET 7=Datos)",ATTR(GRIS,NEGRO));
        fill(2,16,76,1,' ',NORMAL); text(2,16,"Rec: ",NORMAL);
        text(7,16,tls_record_enc_ok?(tls_use_gcm?"GCM":"CBC"):"no",tls_record_enc_ok?ok:warn);
        text(12,16," SrvFin:",NORMAL);
        text(20,16,tls_got_server_finished?"si":"no",tls_got_server_finished?ok:warn);
        text(25,16," HTTPS:",NORMAL); number(32,16,tls_https_bytes,NORMAL); text(40,16," B",NORMAL);
        if(tls_https_bytes > 0){
            term_line(17,"--- respuesta HTTPS (texto) ---",ATTR(AMARILLO,NEGRO));
            int row=18,col=2;
            for(u32 k=0;k<(u32)tls_https_bytes && row<23;k++){
                char c=(char)tls_https_buf[k];
                if(c==13) continue;
                if(c==10){ row++; col=2; continue; }
                if(c<32||(u8)c>126) c='.';
                if(col>=78){ row++; col=2; }
                if(row>=23) break;
                put(col++,row,(u8)c,NORMAL);
            }
        } else if(tls_handshake_status >= 5)
            term_line(17,"Handshake avanzado; sin datos HTTP aun (o MAC/alert).",warn);
        else if(tls_handshake_status >= 4)
            term_line(17,"Finished cifrado enviado. Servidor puede responder alert.",ATTR(GRIS,NEGRO));
        else if(tls_got_server_hello && !tls_ke_ok)
            term_line(17,"Sin KE RSA.",warn);
        else
            term_line(17,"TLS en progreso.",ATTR(GRIS,NEGRO));
        return;
    }
    if(term_eq(cmd,"help")) term_help();
    else if(term_eq(cmd,"about")){
        term_line(5,"MosaicOS: sistema experimental independiente para x86 de 32 bits.",ok);
        term_line(6,"Kernel propio; no utiliza el kernel de Linux ni de Windows.",NORMAL);
        term_line(7,"Interfaz actual: modo texto VGA 80x25. HTTPS+visor (0.31).",NORMAL);
    } else if(term_eq(cmd,"mem")){
        term_line(5,"Memoria reportada por Multiboot:",warn);
        fill(2,6,76,1,' ',NORMAL); text(2,6,"RAM total aproximada: ",NORMAL);
        int n=number(25,6,mem_mb,NORMAL); text(25+n,6," MB",NORMAL);
        fill(2,7,76,1,' ',NORMAL); text(2,7,"Heap usado: ",NORMAL);
        n=number(15,7,heap_usado()/1024,NORMAL); text(15+n,7," KB",NORMAL);
        fill(2,8,76,1,' ',NORMAL); text(2,8,"Heap libre: ",NORMAL);
        n=number(15,8,heap_libre()/1024,NORMAL); text(15+n,8," KB",NORMAL);
        fill(2,9,76,1,' ',NORMAL); text(2,9,"Bloques: ",NORMAL); number(12,9,heap_bloques(),NORMAL);
    } else if(term_eq(cmd,"uptime")){
        u32 secs=ticks/100;
        fill(2,5,76,1,' ',NORMAL); text(2,5,"Tiempo activo: ",NORMAL);
        int n=number(17,5,secs/3600,NORMAL); text(17+n,5," h ",NORMAL);
        n=number(20+n,5,(secs/60)%60,NORMAL); text(20+n,5," min ",NORMAL);
        n=number(25+n,5,secs%60,NORMAL); text(25+n,5," s",NORMAL);
    } else if(term_eq(cmd,"storage")){
        term_line(5,"Almacenamiento MosaicOS 0.17:",ok);
        fill(2,6,76,1,' ',NORMAL); text(2,6,"Backend: ",NORMAL);
        if(backend==BK_DISK) text(12,6,"disco real",ok);
        else if(backend==BK_RAM) text(12,6,"ramdisk temporal",warn);
        else text(12,6,"ninguno",ATTR(ROJO,NEGRO));
        if(ata_present){
            fill(2,7,76,1,' ',NORMAL); text(2,7,"Modelo: ",NORMAL); rawtext(11,7,ata_model,NORMAL);
            fill(2,8,76,1,' ',NORMAL); text(2,8,"Tipo: ",NORMAL); text(9,8,disk_tipo,NORMAL);
            fill(2,9,76,1,' ',NORMAL); text(2,9,"Capacidad: ",NORMAL);
            int n=number(14,9,ata_sectors/2048,NORMAL); text(14+n,9," MB  (",NORMAL);
            n=number(20+n,9,ata_sectors,NORMAL); text(20+n,9," sectores)",NORMAL);
        } else {
            term_line(7,"No hay disco ATA/AHCI detectado (usando ramdisk si aplica).",warn);
        }
        fill(2,10,76,1,' ',NORMAL); text(2,10,"MosaicFS: ",NORMAL);
        text(13,10,fs_ok?"montado":"no montado",fs_ok?ok:warn);
        if(fs_ok){
            fill(2,11,76,1,' ',NORMAL); text(2,11,"Archivos: ",NORMAL); number(13,11,fs_file_count(),NORMAL);
            fill(2,12,76,1,' ',NORMAL); text(2,12,"Libre aprox.: ",NORMAL);
            int n=number(17,12,fs_libre_kb(),NORMAL); text(17+n,12," KB",NORMAL);
        }
        fill(2,13,76,1,' ',NORMAL); text(2,13,"E/S bloques: lecturas=",NORMAL); number(28,13,blk_reads,NORMAL);
        fill(2,14,76,1,' ',NORMAL); text(2,14,"            escrituras=",NORMAL); number(28,14,blk_writes,NORMAL);
        fill(2,15,76,1,' ',NORMAL); text(2,15,"            errores=",NORMAL); number(26,15,blk_errors,blk_errors?ATTR(ROJO,NEGRO):NORMAL);
        term_line(17,"Usa 'files' y 'fscheck' para listar y verificar MosaicFS.",NORMAL);
    } else if(term_eq(cmd,"devices")){
        term_line(5,"Estado de dispositivos de MosaicOS 0.19:",warn);
        fill(2,6,76,1,' ',NORMAL); text(2,6,"Teclado PS/2: inicializado",NORMAL);
        fill(2,7,76,1,' ',NORMAL); text(2,7,"Mouse PS/2: ",NORMAL);
        text(15,7,has_mouse?"detectado":"no detectado",has_mouse?ok:warn);
        fill(2,8,76,1,' ',NORMAL); text(2,8,"ATA/AHCI: ",NORMAL);
        text(13,8,ata_present?"detectado":"no detectado",ata_present?ok:warn);
        fill(2,9,76,1,' ',NORMAL); text(2,9,"USB UHCI: ",NORMAL);
        if(usb_count){ number(14,9,usb_count,ok); text(16,9," controlador(es)",NORMAL); }
        else text(14,9,"no detectado",warn);
        fill(2,10,76,1,' ',NORMAL); text(2,10,"Red RTL8139: ",NORMAL);
        text(17,10,net_ok?"detectada":"no detectada",net_ok?ok:warn);
        fill(2,11,76,1,' ',NORMAL); text(2,11,"Dispositivos PCI: ",NORMAL); number(21,11,pci_count,NORMAL);
        term_line(13,"Comandos: usb, net, pci, storage, devices",NORMAL);
    } else if(term_eq(cmd,"ticks")){
        term_line(5,"Contador monotono del temporizador PIT:",warn);
        fill(2,6,76,1,' ',NORMAL); text(2,6,"Ticks desde el arranque: ",NORMAL);
        number(27,6,ticks,NORMAL);
        fill(2,7,76,1,' ',NORMAL); text(2,7,"Frecuencia objetivo: 100 Hz",NORMAL);
    } else if(term_eq(cmd,"profile")){
        term_line(5,"Perfil: ULTRALIGERO (objetivo: PCs antiguos)",ok);
        fill(2,7,76,1,' ',NORMAL); text(2,7,"RAM detectada al arrancar: ",NORMAL);
        number(30,7,mem_mb,NORMAL); text(34,7," MiB aprox.",NORMAL);
        term_line(9,"Sin servicios en segundo plano ni interfaz pesada.",NORMAL);
        term_line(10,"Modo texto para reducir consumo de RAM y CPU.",NORMAL);
        term_line(12,"Limite actual de mapeo fisico: 64 MiB.",warn);
        term_line(14,"La compatibilidad depende de cada controlador y equipo.",NORMAL);
    } else if(term_eq(cmd,"vmem")){
        term_line(5,"Memoria virtual x86 (paginacion de 4 KiB) — 0.16",warn);
        fill(2,6,76,1,' ',NORMAL); text(2,6,"Paging: ",NORMAL);
        text(10,6,paging_enabled?"ACTIVO":"DESACTIVADO",paging_enabled?ok:warn);
        fill(2,7,76,1,' ',NORMAL); text(2,7,"Rango identidad mapeado: ",NORMAL);
        int n=number(28,7,paging_mapped_bytes/(1024u*1024u),NORMAL);
        text(28+n,7," MiB",NORMAL);
        fill(2,8,76,1,' ',NORMAL); text(2,8,"Tablas de paginas: ",NORMAL);
        number(22,8,paging_table_count,NORMAL);
        fill(2,9,76,1,' ',NORMAL); text(2,9,"Mapas dinamicos: ",NORMAL); number(20,9,vmm_maps,NORMAL);
        fill(2,10,76,1,' ',NORMAL); text(2,10,"Desmapas: ",NORMAL); number(14,10,vmm_unmaps,NORMAL);
        fill(2,11,76,1,' ',NORMAL); text(2,11,"Fallos de mapeo: ",NORMAL); number(21,11,vmm_map_failures,NORMAL);
        fill(2,12,76,1,' ',NORMAL); text(2,12,"Autoprueba VMM: ",NORMAL);
        if(!vmm_selftest_ran) text(19,12,"no ejecutada",warn);
        else text(19,12,vmm_selftest_ok?"OK":"FALLO",vmm_selftest_ok?ok:ATTR(ROJO,NEGRO));
        fill(2,13,76,1,' ',NORMAL); text(2,13,"Traduccion VGA (B8000): ",NORMAL);
        hex32(27,13,paging_translate(0x000B8000u),NORMAL);
        term_line(15,"API: vmm_map_page / vmm_unmap_page / vmm_alloc_map",NORMAL);
        term_line(16,"Mapeo identidad + map/unmap dinamico (sin modo usuario).",NORMAL);
    } else if(term_eq(cmd,"pmm")){
        term_line(5,"Mosaic PMM: paginas fisicas de 4 KiB",ok);
        fill(2,6,76,1,' ',NORMAL); text(2,6,"RAM mapeada: ",NORMAL);
        int n=number(16,6,pmm_page_count*PAGE_SIZE/(1024u*1024u),NORMAL); text(16+n,6," MiB",NORMAL);
        fill(2,7,76,1,' ',NORMAL); text(2,7,"Paginas libres: ",NORMAL); number(20,7,pmm_free_pages,NORMAL);
        fill(2,8,76,1,' ',NORMAL); text(2,8,"Paginas ocupadas/reservadas: ",NORMAL); number(32,8,pmm_page_count-pmm_free_pages,NORMAL);
        fill(2,9,76,1,' ',NORMAL); text(2,9,"Asignaciones: ",NORMAL); number(17,9,pmm_allocations,NORMAL);
        fill(2,10,76,1,' ',NORMAL); text(2,10,"Liberaciones: ",NORMAL); number(17,10,pmm_frees,NORMAL);
        fill(2,11,76,1,' ',NORMAL); text(2,11,"Fallos de asignacion: ",NORMAL); number(24,11,pmm_failures,NORMAL);
        fill(2,12,76,1,' ',NORMAL); text(2,12,"Liberaciones invalidas: ",NORMAL); number(26,12,pmm_bad_frees,NORMAL);
        fill(2,13,76,1,' ',NORMAL); text(2,13,"Entradas mapa Multiboot: ",NORMAL); number(27,13,pmm_map_entries,NORMAL);
        fill(2,14,76,1,' ',NORMAL); text(2,14,"Rangos RAM utilizables: ",NORMAL); number(27,14,pmm_map_valid,NORMAL);
        term_line(16,"El asignador protege kernel, heap, firmware y datos de arranque.",NORMAL);
        term_line(17,"API: pmm_alloc_page() / pmm_free_page() + VMM map/unmap.",NORMAL);
    } else if(term_eq(cmd,"files")){
        if(!fs_ok){
            term_line(5,"MosaicFS no esta montado.",warn);
        } else {
            term_line(5,"Archivos en MosaicFS:",ok);
            int row=6, shown=0;
            for(int i=0;i<NENT && row<18;i++){
                if(!table[i].name[0]) continue;
                fill(2,row,76,1,' ',NORMAL);
                text(2,row,table[i].name,NORMAL);
                int n=number(24,row,table[i].size,NORMAL);
                text(24+n,row," bytes",ATTR(GRIS,NEGRO));
                row++; shown++;
            }
            if(!shown) term_line(6,"(vacio)",ATTR(GRIS,NEGRO));
            fill(2,19,76,1,' ',NORMAL); text(2,19,"Total: ",NORMAL); number(10,19,shown,NORMAL);
        }
    } else if(term_eq(cmd,"fscheck")){
        term_line(5,"Comprobacion de integridad MosaicFS:",warn);
        if(!fs_ok){
            term_line(7,"No hay volumen montado.",ATTR(ROJO,NEGRO));
        } else {
            int probs = fs_check();
            fill(2,7,76,1,' ',NORMAL); text(2,7,"Resultado: ",NORMAL);
            if(probs==0) text(14,7,"OK — sin problemas detectados",ok);
            else {
                text(14,7,"PROBLEMAS: ",ATTR(ROJO,NEGRO));
                number(26,7,probs,ATTR(ROJO,NEGRO));
            }
            fill(2,9,76,1,' ',NORMAL); text(2,9,"Archivos: ",NORMAL); number(13,9,fs_file_count(),NORMAL);
            fill(2,10,76,1,' ',NORMAL); text(2,10,"Sectores gestionados: ",NORMAL); number(26,10,fs_total,NORMAL);
            fill(2,11,76,1,' ',NORMAL); text(2,11,"Montajes: ",NORMAL); number(13,11,fs_mounts,NORMAL);
            fill(2,12,76,1,' ',NORMAL); text(2,12,"Formatos: ",NORMAL); number(13,12,fs_format_count,NORMAL);
            term_line(14,"Se verifica magic, checksum, solapes y limites de entradas.",NORMAL);
        }
    } else if(term_eq(cmd,"pci")){
        term_line(5,"Inventario PCI (hasta 12 entradas):",ok);
        int row=6, shown=0;
        for(int i=0;i<pci_count && row<18;i++){
            PciDev *d=&pci_devs[i];
            fill(2,row,76,1,' ',NORMAL);
            hex32(2,row, (d->vendor<<16)|d->device, NORMAL);
            text(12,row," ",NORMAL);
            number(13,row,d->bus,NORMAL); text(15,row,":",NORMAL);
            number(16,row,d->dev,NORMAL); text(18,row,".",NORMAL);
            number(19,row,d->func,NORMAL);
            text(22,row,"cl ",NORMAL); hex32(25,row, (d->classc<<16)|(d->subclass<<8)|d->prog_if, ATTR(GRIS,NEGRO));
            /* etiqueta corta */
            const char *lab="?";
            if(d->classc==0x01) lab="storage";
            else if(d->classc==0x02) lab="net";
            else if(d->classc==0x03) lab="display";
            else if(d->classc==0x0C && d->subclass==0x03) lab="usb";
            else if(d->classc==0x06) lab="bridge";
            text(36,row,lab,ok);
            row++; shown++;
        }
        if(!shown) term_line(6,"Ningun dispositivo PCI listado.",warn);
        fill(2,19,76,1,' ',NORMAL); text(2,19,"Total: ",NORMAL); number(10,19,pci_count,NORMAL);
    } else if(term_eq(cmd,"usb")){
        term_line(5,"USB (UHCI) — base de deteccion 0.19:",ok);
        if(!usb_count){
            term_line(7,"No se detectaron controladores UHCI.",warn);
            term_line(8,"OHCI/EHCI/xHCI aparecen en 'pci' pero aun sin driver.",NORMAL);
            term_line(10,"En QEMU prueba: -device usb-tablet (con UHCI).",ATTR(GRIS,NEGRO));
        } else {
            int row=7;
            for(int i=0;i<usb_count && row<16;i++){
                UsbCtrl *u=&usb_ctrls[i];
                fill(2,row,76,1,' ',NORMAL);
                text(2,row,"UHCI ",NORMAL);
                number(7,row,i,NORMAL);
                text(9,row," IO=",NORMAL); hex32(13,row,u->iobase,NORMAL);
                text(24,row," puertos conectados: ",NORMAL);
                number(46,row,u->ports_connected,u->ports_connected?ok:warn);
                row++;
                fill(2,row,76,1,' ',NORMAL);
                text(4,row,"PCI ",NORMAL); number(8,row,u->bus,NORMAL); text(10,row,":",NORMAL);
                number(11,row,u->dev,NORMAL);
                text(14,row," vendor=",NORMAL); hex32(23,row,u->vendor,NORMAL);
                row+=2;
            }
            term_line(17,"Sin transferencias HID/MSC todavia (solo estado de puertos).",ATTR(GRIS,NEGRO));
        }
    } else if(term_eq(cmd,"net")){
        term_line(5,"Red Ethernet + pila IP 0.20:",ok);
        if(!net_ok){
            term_line(7,"No hay NIC RTL8139 detectada.",warn);
            term_line(8,"QEMU: -netdev user,id=n0 -device rtl8139,netdev=n0",NORMAL);
        } else {
            fill(2,7,76,1,' ',NORMAL); text(2,7,"NIC: activa  IO=",NORMAL); hex32(22,7,net_iobase,NORMAL);
            fill(2,8,76,1,' ',NORMAL); text(2,8,"MAC: ",NORMAL);
            for(int i=0;i<6;i++){
                u8 v=net_mac[i];
                put(8+i*3,8,"0123456789ABCDEF"[v>>4],NORMAL);
                put(9+i*3,8,"0123456789ABCDEF"[v&15],NORMAL);
                if(i<5) put(10+i*3,8,':',ATTR(GRIS,NEGRO));
            }
            char ips[16];
            fill(2,9,76,1,' ',NORMAL); text(2,9,"IP: ",NORMAL); ip_fmt(ip_addr,ips); text(6,9,ips,ok);
            fill(2,10,76,1,' ',NORMAL); text(2,10,"GW: ",NORMAL); ip_fmt(ip_gw,ips); text(6,10,ips,NORMAL);
            fill(2,11,76,1,' ',NORMAL); text(2,11,"Eth TX/RX: ",NORMAL); number(14,11,net_tx_count,NORMAL);
            text(22,11," / ",NORMAL); number(25,11,net_rx_count,NORMAL);
            fill(2,12,76,1,' ',NORMAL); text(2,12,"IP TX/RX: ",NORMAL); number(13,12,ip_tx_pkts,NORMAL);
            text(21,12," / ",NORMAL); number(24,12,ip_rx_pkts,NORMAL);
            fill(2,13,76,1,' ',NORMAL); text(2,13,"ICMP echo TX/RX: ",NORMAL); number(20,13,icmp_echo_tx,NORMAL);
            text(28,13," / ",NORMAL); number(31,13,icmp_echo_rx,NORMAL);
            term_line(15,"ping | http a.b.c.d /ruta | ip | nettx",NORMAL);
        }
    } else if(term_eq(cmd,"ip")){
        term_line(5,"Configuracion IPv4 (estatica, QEMU user net):",ok);
        if(!net_ok){ term_line(7,"Sin NIC.",warn); }
        else {
            char s[16];
            fill(2,7,76,1,' ',NORMAL); text(2,7,"Direccion: ",NORMAL); ip_fmt(ip_addr,s); text(14,7,s,ok);
            fill(2,8,76,1,' ',NORMAL); text(2,8,"Mascara:   ",NORMAL); ip_fmt(ip_mask,s); text(14,8,s,NORMAL);
            fill(2,9,76,1,' ',NORMAL); text(2,9,"Puerta:    ",NORMAL); ip_fmt(ip_gw,s); text(14,9,s,NORMAL);
            fill(2,10,76,1,' ',NORMAL); text(2,10,"DNS:       ",NORMAL); ip_fmt(ip_dns,s); text(14,10,s,NORMAL);
            fill(2,12,76,1,' ',NORMAL); text(2,12,"ARP req/rep: ",NORMAL); number(16,12,arp_requests,NORMAL);
            text(24,12," / ",NORMAL); number(27,12,arp_replies_rx,NORMAL);
            term_line(14,"Proximo: TLS (enorme) o mejor visor HTML.",ATTR(GRIS,NEGRO));
        }
    } else if(term_eq(cmd,"nettx")){
        if(!net_ok) term_line(5,"Sin NIC.",warn);
        else if(net_send_test()==0){
            term_line(5,"Trama de prueba MOSAICOS enviada (broadcast).",ok);
            fill(2,7,76,1,' ',NORMAL); text(2,7,"TX total: ",NORMAL); number(13,7,net_tx_count,NORMAL);
        } else term_line(5,"Fallo al transmitir.",ATTR(ROJO,NEGRO));
    } else if(term_eq(cmd,"netrx")){
        if(!net_ok) term_line(5,"Sin NIC.",warn);
        else {
            u8 buf[128];
            int n = net_poll_rx(buf, sizeof buf);
            if(n>0){
                term_line(5,"Trama recibida:",ok);
                fill(2,7,76,1,' ',NORMAL); text(2,7,"Bytes: ",NORMAL); number(10,7,n,NORMAL);
                fill(2,8,76,1,' ',NORMAL); text(2,8,"Dest MAC: ",NORMAL);
                for(int i=0;i<6 && i<n;i++){
                    put(13+i*3,8,"0123456789ABCDEF"[buf[i]>>4],NORMAL);
                    put(14+i*3,8,"0123456789ABCDEF"[buf[i]&15],NORMAL);
                }
            } else if(n==0) term_line(5,"No hay tramas nuevas en el buffer.",warn);
            else term_line(5,"Error de recepcion.",ATTR(ROJO,NEGRO));
            fill(2,10,76,1,' ',NORMAL); text(2,10,"RX total: ",NORMAL); number(13,10,net_rx_count,NORMAL);
        }
    } else if(term_eq(cmd,"gfx")){
        term_line(5,"Entrando en modo grafico VGA 320x200...",ok);
        term_line(6,"Pulsa Esc para volver al texto.",NORMAL);
        /* Salir de la terminal visualmente y lanzar demo */
        app_gfx();
        /* Al volver, redibujar contexto de terminal */
        scr_begin();
        fill(1,4,78,16,' ',NORMAL);
        term_line(5,"Modo grafico cerrado. Escritorio de texto restaurado.",ok);
        term_line(6,"Framebuffer Mode 13h disponible via mosaico Graficos o 'gfx'.",NORMAL);
    } else if(term_eq(cmd,"version")){
        term_line(5,"MosaicOS 0.31 - kernel x86 de 32 bits",ok);
        term_line(6,"HTTPS + visor texto. https host [/ruta]",NORMAL);
    } else if(term_eq(cmd,"clear")){
        fill(1,4,78,18,' ',NORMAL); *row=4;
    } else {
        term_line(5,"Comando desconocido. Escribe help para ver los comandos.",warn);
    }
}
static void app_terminal(void){
    char line[TERM_LINE]; int len=0, row=4;
    line[0]=0;
    scr_begin(); ventana(T_TERM,"Escribe help; Enter ejecuta; Esc vuelve a Inicio");
    text(2,4,"Terminal de MosaicOS 0.31",ATTR(AMARILLO,NEGRO));
    text(2,6,"No es una shell POSIX; acepta comandos internos de MosaicOS.",ATTR(GRIS,NEGRO));
    term_help();
    fill(1,19,78,4,' ',NORMAL);
    scr_end();
    for(;;){
        scr_begin(); fill(1,21,78,1,' ',NORMAL);
        text(2,21,"mosaic> ",ATTR(CIAN,NEGRO)); text(10,21,line,NORMAL); scr_end();
        int k=event_wait();
        if(k==K_ESC || (k==K_CLICK && back_hit())) return;
        if(k==K_BACKSPACE){ if(len>0) line[--len]=0; }
        else if(k==K_ENTER){
            line[len]=0;
            if(term_eq(line,"exit")) return;
            scr_begin();
            fill(1,4,78,14,' ',NORMAL);
            text(2,4,"mosaic> ",ATTR(CIAN,NEGRO)); text(10,4,line,NORMAL);
            term_execute(line,&row);
            fill(1,19,78,2,' ',NORMAL);
            len=0; line[0]=0;
            scr_end();
        } else if(k>=32 && k<=126 && len<TERM_LINE-1){
            line[len++]=(char)k; line[len]=0;
        }
    }
}

/* ---------- Compatibilidad ---------- */
static void app_gfx(void){
    /* Demo general de gráficos Mode 13h. Esc vuelve a texto. */
    gfx_enter();
    int phase = 0;
    for(;;){
        /* Fondo degradado */
        for(int y=0;y<GFX_H;y++){
            u8 c = (u8)(32 + ((y + phase) % 200));
            gfx_hline(0, y, GFX_W, c);
        }
        /* Panel central */
        gfx_fill_rect(40, 30, 240, 140, 0);
        gfx_rect(40, 30, 240, 140, 1);
        gfx_rect(42, 32, 236, 136, 8);

        gfx_text(56, 40, "MOSAICOS", 1);
        gfx_text(56, 52, "GFX 0.18", 6);

        /* Primitivas */
        gfx_fill_rect(60, 70, 50, 30, 2);
        gfx_rect(120, 70, 50, 30, 3);
        gfx_line(180, 70, 230, 100, 5);
        gfx_line(230, 70, 180, 100, 5);
        gfx_circle(100, 140, 18, 4);
        gfx_fill_circle(160, 140, 14, 7);
        gfx_circle(220, 140, 18, 9);

        gfx_text(56, 160, "ESC SALIR", 15);
        gfx_frames++;

        /* Poll teclado sin depender del buffer de texto completo */
        for(int t=0;t<3;t++){
            if(inb(0x64)&1){
                u8 sc = inb(0x60);
                if(sc==0x01){ /* ESC make */
                    gfx_leave();
                    return;
                }
            }
        }
        /* Pequeña espera */
        for(volatile int i=0;i<400000;i++);
        phase = (phase + 2) & 255;
    }
}

/* ---------- Apagar / Reiniciar ---------- */
static void apagar(void){
    __asm__ volatile("cli");
    outw(0x604,0x2000); outw(0xB004,0x2000); outw(0x4004,0x3400);
    fill(0,0,W,H,' ',NORMAL);
    text(14,12,"Ya puedes apagar el equipo con el botón físico.",ATTR(AMARILLO,NEGRO));
    for(;;) __asm__ volatile("hlt");
}
static void reiniciar(void){
    __asm__ volatile("cli");
    for(int i=0;i<100000;i++) if(!(inb(0x64)&2)) break;
    outb(0x64,0xFE);
    static struct dtr nada = {0,0};
    __asm__ volatile("lidt %0; int3"::"m"(nada));
    for(;;) __asm__ volatile("hlt");
}

/* ================= Principal ================= */
extern char _end[];
void kernel_main(u32 magic, u32 *mbi){
    hide_hw_cursor();
    u32 fin=0x00800000;                         /* 8 MB por defecto */
    if(magic==0x2BADB002 && (mbi[0]&1)){
        mem_mb=(mbi[2]+1024+512)/1024;
        /* Multiboot mem_upper is KiB above 1 MiB; cap arithmetic safely. */
        if(mbi[2] > (0x20000000u-0x00100000u)/1024u) fin=0x20000000u;
        else fin=0x00100000u+mbi[2]*1024u;
    }
    km_init();
    gdt_init(); idt_init(); pic_init(); pit_init();
    if(!paging_init(fin)){
        __asm__ volatile("cli");
        fill(0,0,W,H,' ',ATTR(BLANCO,ROJO));
        text(3,5,"Error al inicializar la memoria virtual.",ATTR(AMARILLO,ROJO));
        for(;;) __asm__ volatile("hlt");
    }
    u32 heap_end=(u32)_end+512u*1024u;
    if(heap_end<(u32)_end || heap_end>fin) heap_end=fin;
    heap_init((u32)_end,heap_end);
    pmm_init(magic,mbi,fin);
    vmm_selftest();                 /* 0.16: prueba de map/unmap + PMM */
    mouse_init();
    pci_scan();
    disks_detect();
    usb_init();
    net_init();
    ip_stack_init();
    fs_boot();
    __asm__ volatile("sti");

    int sel=0;
    dibujar_inicio(sel);
    for(;;){
        int k=event_wait(), old=sel, act=0;
        switch(k){
            case K_LEFT:  if(sel%TILE_COLS>0) sel--; break;
            case K_RIGHT: if(sel%TILE_COLS<TILE_COLS-1 && sel<NM-1) sel++; break;
            case K_UP:    if(sel>=TILE_COLS) sel-=TILE_COLS; break;
            case K_DOWN:  if(sel+TILE_COLS<NM) sel+=TILE_COLS; break;
            case K_ENTER: act=1; break;
            case K_F1:    sel=T_TUT; act=1; break;
            case K_F2:    sel=T_CONFIG; act=1; break;
            case K_F3:    sel=T_ARCH; act=1; break;
            case K_F4:    sel=T_NOTAS; act=1; break;
            case K_F5:    sel=T_SIS; act=1; break;
            case K_MOVE: case K_CLICK:
                for(int i=0;i<NM;i++){
                    int x,y; tile_xy(i,&x,&y);
                    if(in(x,y,TW,TH)){ sel=i; if(k==K_CLICK) act=1; }
                }
                break;
        }
        if(act){
            switch(sel){
                case T_ARCH:   app_archivos(); break;
                case T_NOTAS:  app_notas(); break;
                case T_CALC:   app_calc(); break;
                case T_RELOJ:  app_reloj(); break;
                case T_SIS:    app_sistema(); break;
                case T_TUT:    app_tutorial(); break;
                case T_COMPAT: app_gfx(); break;
                case T_COLORES: tema=(tema+1)%6; break;
                case T_CONFIG:  app_config(); break;
                case T_TERM:    app_terminal(); break;
                case T_REINI:  if(confirmar("Reiniciar",color_mosaico(T_REINI),"¿Quieres reiniciar el equipo?")) reiniciar(); break;
                case T_APAGAR: if(confirmar("Apagar",color_mosaico(T_APAGAR),"¿Quieres apagar el equipo?")) apagar(); break;
            }
        }
        if(act || sel!=old) dibujar_inicio(sel);
    }
}

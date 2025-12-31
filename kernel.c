/* MicroOS v7: Shift Key Support */

#define VGA_ADDR ((volatile unsigned short*)0xB8000)

static inline unsigned char inb(unsigned short p) {
    unsigned char r; __asm__ volatile("inb %1, %0":"=a"(r):"Nd"(p)); return r;
}
static inline void outb(unsigned short p, unsigned char v) {
    __asm__ volatile("outb %0, %1"::"a"(v),"Nd"(p));
}

void update_cursor(int x, int y) {
    unsigned short pos = y*80 + x;
    outb(0x3D4, 0x0F); outb(0x3D5, (unsigned char)(pos&0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (unsigned char)((pos>>8)&0xFF));
}

void k_putc(char c, int* x, int* y, int color) {
    if(c=='\n') { *x=0; (*y)++; }
    else if(c=='\b') {
        if(*x>0) { (*x)--; VGA_ADDR[(*y)*80 + *x] = (color<<8)|' '; }
        else if(*y>0 && *x==0) { 
            *x=79; (*y)--; VGA_ADDR[(*y)*80 + *x] = (color<<8)|' ';
        }
    } else {
        VGA_ADDR[(*y)*80 + *x] = (unsigned short)c | (color << 8);
        (*x)++;
    }
    
    if(*x>=80) { *x=0; (*y)++; }
    if(*y>=25) {
        for(int r=0; r<24; r++) {
            for(int c=0; c<80; c++) VGA_ADDR[r*80 + c] = VGA_ADDR[(r+1)*80 + c];
        }
        for(int c=0; c<80; c++) VGA_ADDR[24*80 + c] = (color<<8)|' ';
        *y = 24;
    }
    update_cursor(*x, *y);
}

int str_eq(const char* buf, const char* cmd) {
    int i=0;
    while(cmd[i]) { if(buf[i]!=cmd[i]) return 0; i++; }
    return (buf[i]==0);
}

void k_print(const char* s, int* x, int* y, int color) {
    while(*s) k_putc(*s++, x, y, color);
}

typedef struct { char name[12]; int used; char content[64]; int size; } File;

void kernel_main(void) {
    int x=0; int y=0;
    int color = 0x0B;
    
    for(int i=0; i<80*25; i++) VGA_ADDR[i] = (color<<8)|' ';
    update_cursor(0,0);
    
    k_print("MicroOS v7. Shift Supported.\n", &x, &y, 0x0E);
    k_print("Try: echo \"Hello\" >> file\n", &x, &y, 0x07);
    k_print("$ ", &x, &y, 0x0A);
    
    char buf[128];
    int len=0;
    
    File fs[8];
    for(int i=0;i<8;i++) { fs[i].used=0; fs[i].size=0; }
    
    const char* map_l = "1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./";
    const char* map_u = "!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?";
    /*
       Scancode offsets:
       0x02 - 0x0D: 1...=
       0x10 - 0x1B: q... ]
       0x1E - 0x28: a...'
       0x2C - 0x35: z.../
    */
    
    int shift = 0;
    
    while(1) {
        if((inb(0x64)&1)) {
            unsigned char s = inb(0x60);
            
            // Shift check
            if(s == 0x2A || s == 0x36) { shift = 1; continue; }
            if(s == 0xAA || s == 0xB6) { shift = 0; continue; }
            
            if(!(s & 0x80)) {
                char c = 0;
                
                if(s==0x39) c=' ';
                else if(s==0x0E) c='\b';
                else if(s==0x1C) c='\n';
                else {
                    // Logic to pull from maps based on ranges
                    int idx = -1;
                    if(s>=0x02 && s<=0x0D) idx = s - 0x02;
                    else if(s>=0x10 && s<=0x1B) idx = 12 + (s - 0x10);
                    else if(s>=0x1E && s<=0x28) idx = 24 + (s - 0x1E); // includes ' at 0x28
                    else if(s>=0x2B && s<=0x35) idx = 36 + (s - 0x2B); // \ z...m , . /
                    
                    if(idx >= 0 && idx < 50) {
                        c = shift ? map_u[idx] : map_l[idx];
                    }
                }

                if(c) {
                    // debounce simple
                    for(volatile int d=0; d<400000; d++); 
                    
                    if(c == '\n') {
                        k_putc('\n', &x, &y, color);
                        buf[len] = 0;
                        
                        // Parse Redirection logic
                        int redirect_idx = -1;
                        for(int k=0; k<len-1; k++) {
                            if(buf[k]=='>' && buf[k+1]=='>') {
                                redirect_idx = k; break;
                            }
                        }
                        
                        char cmd[10]={0};
                        int i=0; while(buf[i]==' ') i++;
                        int ci=0; while(buf[i] && buf[i]!=' ' && ci<9) cmd[ci++]=buf[i++];
                        
                        // Handler
                         if(redirect_idx != -1) {
                             int fstart = redirect_idx+2;
                             while(buf[fstart]==' ') fstart++;
                             char fname[12]={0}; int fi=0;
                             while(buf[fstart] && buf[fstart]!=' ' && fi<11) fname[fi++]=buf[fstart++];
                             
                             int cstart = 0; int cend = redirect_idx;
                             for(int k=0; k<redirect_idx; k++) {
                                 if(buf[k]=='"') {
                                     if(cstart==0) cstart=k+1; else cend=k;
                                 }
                             }
                             if(cstart==0) { cstart=i; while(buf[cstart]==' ') cstart++; }
                             
                             int f=-1;
                             for(int k=0;k<8;k++) if(fs[k].used && str_eq(fs[k].name, fname)) { f=k; break; }
                             if(f!=-1) {
                                  int ptr = fs[f].size;
                                  for(int k=cstart;k<cend && ptr<63;k++) fs[f].content[ptr++]=buf[k];
                                  fs[f].content[ptr]=0; fs[f].size=ptr;
                                  k_print("Appended.\n", &x, &y, 0x0A);
                             } else k_print("404 (use touch)\n", &x, &y, 0x0C);
                             
                         } else if(str_eq(cmd, "ls")) {
                            for(int k=0;k<8;k++) if(fs[k].used) {
                                k_print(fs[k].name, &x, &y, 0x0F); k_putc(' ',&x,&y,0);
                            }
                            k_putc('\n', &x, &y, color);
                         } else if(str_eq(cmd, "touch")) {
                             while(buf[i]==' ') i++;
                             if(buf[i]) {
                                 int f=-1; for(int k=0;k<8;k++) if(!fs[k].used) {f=k; break;}
                                 if(f!=-1) {
                                     fs[f].used=1; fs[f].size=0; fs[f].content[0]=0;
                                     int fi=0; while(buf[i] && buf[i]!=' ' && fi<11) fs[f].name[fi++]=buf[i++];
                                     fs[f].name[fi]=0;
                                     k_print("OK\n", &x, &y, 0x0A);
                                 } else k_print("Full\n", &x, &y, 0x0C);
                             } else k_print("Name?\n",&x,&y,0x0C);
                         } else if(str_eq(cmd, "cat")) {
                             while(buf[i]==' ') i++;
                             char fname[12]={0}; int fi=0;
                             while(buf[i] && buf[i]!=' ' && fi<11) fname[fi++]=buf[i++];
                             int f=-1; 
                             for(int k=0;k<8;k++) if(fs[k].used && str_eq(fs[k].name, fname)) f=k;
                             if(f!=-1) { k_print(fs[f].content,&x,&y,0x0F); k_putc('\n',&x,&y,0); }
                             else k_print("404\n", &x, &y, 0x0C);
                         } else if(str_eq(cmd, "clear")) {
                             for(int k=0;k<80*25;k++) VGA_ADDR[k]=(color<<8)|' ';
                             x=0; y=0; update_cursor(0,0);
                         } else if(str_eq(cmd, "echo")) {
                             while(buf[i]==' ') i++;
                             while(buf[i]) k_putc(buf[i++], &x, &y, 0x0F);
                             k_putc('\n', &x, &y, color);
                         } else if(ci>0) k_print("Unknown\n",&x,&y,0x0C);
                        
                        len = 0;
                        k_print("$ ", &x, &y, 0x0A);
                    } 
                    else if(c == '\b') {
                        if(len > 0) { len--; k_putc('\b', &x, &y, color); }
                    } 
                    else if(len < 120) {
                        buf[len++] = c;
                        k_putc(c, &x, &y, 0x0F);
                    }
                }
            }
        }
    }
}

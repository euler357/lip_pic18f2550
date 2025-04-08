/*********************************************/
/* Large Integer Math Library                */
/* for arithmetic in the prime field         */
/* modulus = 2^256 - 2^224 + 2^192 + 2^96 -1 */
/* CPU: Microchip PIC18F2550                 */
/*********************************************/ 
#include <18F2550.h>
 
#fuses HSPLL,NOWDT,NOPROTECT,NOLVP,NODEBUG,USBDIV,PLL1,CPUDIV2,VREGEN
#DEVICE ADC=10 
 
#use delay(clock=32000000) 

/* Define additional registers needed for ASM code */
#define STATUS    0x0FD8
#define FSR0L     0x0FE9
#define FSR0H     0x0FEA 
#define FSR1L     0x0FE1
#define FSR1H     0x0FE2
#define FSR2L     0x0FD9
#define FSR2H     0x0FDA
#define INDF0     0x0FEF
#define INDF1     0x0FE7
#define INDF2     0x0FDF
#define POSTINC0  0x0FEE
#define POSTINC1  0x0FE6
#define POSTINC2  0x0FDE
#define POSTDEC0  0x0FED
#define POSTDEC1  0x0FE5
#define POSTDEC2  0x0FDD
#define PREINC0   0x0FEC
#define PREINC1   0x0FE4
#define PREINC2   0x0FDC
#define PRODH     0x0FF4
#define PRODL     0x0FF3

/* Format of large integers */
/* 0x010203 ... 32 = {0x32, 0x31, 0x29, ... 0x01} */
/* This is done to easily increment pointers starting with the LSB */

/* Define my secret number */
/* This should be randomly generated it's easier to demonstrate with a fixed number */
byte secret[32] = {  0xDE,0xAD,0xBE,0xEF,0xDE,0xAD,0xBE,0xEF,
				         0xAB,0xAD,0xBA,0xBE,0xAB,0xAD,0xBA,0xBE,
                     0xDE,0xAD,0xBE,0xEF,0xDE,0xAD,0xBE,0xEF,
				         0xAB,0xAD,0xBA,0xBE,0xAB,0xAD,0xBA,0xBE };

/* Using NIST prime field p256 = 2^256 - 2^224 + 2^192 + 2^96 -1 */

/* Define Base Point on Elliptic Curve */

byte Gx[32]={  0x96, 0xc2, 0x98, 0xd8, 0x45, 0x39, 0xa1, 0xf4,
               0xa0, 0x33, 0xeb, 0x2d, 0x81, 0x7d, 0x03, 0x77,
               0xf2, 0x40, 0xa4, 0x63, 0xe5, 0xe6, 0xbc, 0xf8,
               0x47, 0x42, 0x2c, 0xe1, 0xf2, 0xd1, 0x17, 0x6b };
               
byte Gy[32]={ 0xf5, 0x51, 0xbf, 0x37, 0x68, 0x40, 0xb6, 0xcb,
               0xce, 0x5e, 0x31, 0x6b, 0x57, 0x33, 0xce, 0x2b,
               0x16, 0x9e, 0x0f, 0x7c, 0x4a, 0xeb, 0xe7, 0x8e,
               0x9b, 0x7f, 0x1a, 0xfe, 0xe2, 0x42, 0xe3, 0x4f };

byte x[32] = {	0x11,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
   				0x11,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
	    			0x11,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		   		0x11,0x02,0x03,0x04,0x05,0x06,0x07,0xf8};

byte y[32] = {	0x01,0xC2,0x03,0x04,0x05,0x06,0x07,0x08,
			   	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
				   0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
   				0x01,0x02,0x03,0x04,0x05,0x06,0x07,0xF8};

byte z[32] = {	0,0,0,0,0,0,0,0,
   				0,0,0,0,0,0,0,0,
	   			0,0,0,0,0,0,0,0,
		   		0,0,0,0,0,0,0,0};

byte bz[64] = {1,1,1,1,1,1,1,1,
   				0,0,0,0,0,0,0,0,
	   			0,0,0,0,0,0,0,0,
		   		0,0,0,0,0,0,0,0,
            	0,0,0,0,0,0,0,0,
   				0,0,0,0,0,0,0,0,
	   			0,0,0,0,0,0,0,0,
		   		0,0,0,0,0,0,0,0};

byte tempm[32];   /* Temp for mod p and invert */
byte tempm2[32];  /* Temp for mod p and invert */
byte tempas[32];  /* Temp for add/subtract */
byte u[32],v[32],x1[32],x2[32];

/* This is added after a subtract if the answer is negative (2's complement */
static byte modulus[32]={  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                           0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
                           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                           0x01,0x00,0x00,0x00,0xff,0xff,0xff,0xff };

/* This is added after an addition if the answer has a carry */
static byte reducer[32]={  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                           0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,
                           0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                           0xfe,0xff,0xff,0xff,0x00,0x00,0x00,0x00};

/* Function prototypes */
void lip_mul(byte *A, byte *B, byte *C);
void lip_inv(byte *A, byte *B);
void lip_copy(byte *A, byte *B);
void lip_modp(byte *A, byte *B);
void lip_add(byte *A, byte *B, byte *C);
void lip_sub(byte *A, byte *B, byte *C);

/* Internal Functions */
int lip_addt(byte *A, byte *B, byte *C);
int lip_subt(byte *A, byte *B, byte *C);
void lip_rshift(byte *A);
int lip_notzero(byte *A);

/********************************/
/* Function Name: lip_add       */
/*    C = A + B                 */
/*    A,B,C = 32 byte arrays    */
/*    Return: N/A               */
/********************************/
void lip_add(byte *A, byte *B, byte *C)
{
   if(lip_addt(A,B,C))
   {
      lip_addt(C,reducer,tempas);
      lip_copy(tempas,C);      
   }   
}

/********************************/
/* Function Name: lip_sub       */
/*    C = A - B                 */
/*    A,B,C = 32 byte arrays    */
/*    Return: N/A               */
/********************************/
void lip_sub(byte *A, byte *B, byte *C)
{
   if(lip_subt(A,B,C))
   {
      lip_addt(C,modulus,tempas);
      lip_copy(tempas,C);
   }
}

/********************************/
/* Function Name: lip_modp      */
/*    A = B mod modulus         */
/*    A = 32 byte array         */
/*    B = 64 byte array         */
/*    Return: N/A               */
/********************************/
void lip_modp(byte *A, byte *B)
{
   BYTE n;

   lip_copy(B,A); /* A = T */ 

   /* S1 */
   for(n=0;n<12;n++)   
      tempm[n]=0;
   for(n=12;n<32;n++)   
      tempm[n]=B[n+32];
   lip_add(A,tempm,tempm2); /* tempm2=T+S1 */
   lip_add(tempm2,tempm,A); /* A=T+S1+S1 */

   /* S2 */
   for(n=0;n<12;n++)   
      tempm[n]=0;
   for(n=12;n<28;n++)   
      tempm[n]=B[n+36];
   for(n=28;n<32;n++)   
      tempm[n]=0;
   lip_add(A,tempm,tempm2); /* tempm2=T+S1+S1+S2 */
   lip_add(tempm2,tempm,A); /* A=T+S1+S1+S2+S2 */

   /* S3 */
   for(n=0;n<12;n++)   
      tempm[n]=B[n+32];
   for(n=12;n<24;n++)   
      tempm[n]=0;
   for(n=24;n<32;n++)   
      tempm[n]=B[n+32];
   lip_add(A,tempm,tempm2); /* tempm2=T+S1+S1+S2++S2+S3 */

   /* S4 */
   for(n=0;n<12;n++)   
      tempm[n]=B[n+36];
   for(n=12;n<24;n++)   
      tempm[n]=B[n+40];
   for(n=24;n<28;n++)   
      tempm[n]=B[n+28];
   for(n=28;n<32;n++)   
      tempm[n]=B[n+4];
   lip_add(tempm2,tempm,A); /* A=T+S1+S1+S2++S2+S3+S4 */

   /* D1 */
   for(n=0;n<12;n++)   
      tempm[n]=B[n+44];
   for(n=12;n<24;n++)   
      tempm[n]=0;
   for(n=24;n<28;n++)   
      tempm[n]=B[n+8];
   for(n=28;n<32;n++)   
      tempm[n]=B[n+12];
   lip_sub(A,tempm,tempm2); /* tempm2=T+S1+S1+S2++S2+S3+S4-D1 */

   /* D2 */
   for(n=0;n<16;n++)   
      tempm[n]=B[n+48];
   for(n=16;n<24;n++)   
      tempm[n]=0;
   for(n=24;n<28;n++)   
      tempm[n]=B[n+12];
   for(n=28;n<32;n++)   
      tempm[n]=B[n+16];
   lip_sub(tempm2,tempm,A); /* A=T+S1+S1+S2++S2+S3+S4-D1-D2 */

   /* D3 */
   for(n=0;n<12;n++)   
      tempm[n]=B[n+52];
   for(n=12;n<24;n++)   
      tempm[n]=B[n+20];
   for(n=24;n<28;n++)   
      tempm[n]=0;
   for(n=28;n<32;n++)   
      tempm[n]=B[n+20];;
   lip_sub(A,tempm,tempm2); /* tempm2=T+S1+S1+S2++S2+S3+S4-D1-D2-D3 */

   /* D4 */
   for(n=0;n<8;n++)   
      tempm[n]=B[n+56];
   for(n=8;n<12;n++)   
      tempm[n]=0;
   for(n=12;n<24;n++)   
      tempm[n]=B[n+24];
   for(n=24;n<28;n++)   
      tempm[n]=0;
   for(n=28;n<32;n++)   
      tempm[n]=B[n+24];
   lip_sub(tempm2,tempm,A); /* A=T+S1+S1+S2++S2+S3+S4-D1-D2-D3-D4 */
}

/********************************/
/* Function Name: lip_mul       */
/*    C = A x B                 */
/*    A,B = 32 byte arrays      */
/*    C = 64 byte array         */
/*    Return: N/A               */
/********************************/
void lip_mul(byte *A, byte *B, byte *C)
{
   BYTE shift=0;           /* Keeps track of digit in output */
   BYTE out_loop;          /* Main loop counter */
   BYTE loop;              /* Loop variable */
   BYTE carryL;            /* CarryL variable */
   BYTE carryH;            /* CarryH variable */

#asm

; Clear output variable = C
movff C, FSR1L
   movff &C+1, FSR1H
   movlw 64                ; Copy 32 to w
   movwf loop              ; Copy w to loop 
loopClear:
   clrf POSTINC1           ; Clear C(n)       
   decfsz loop             ; loop test
   bra loopClear           ; jump
   
   movlw 31                ; Set outer loop counter to 31
   movwf out_loop          

; Set up pointers 
   movff A, FSR0L
   movff &A+1, FSR0H
   movff B, FSR1L
   movff &B+1, FSR1H
   movff C, FSR2L
   movff &C+1, FSR2H

   movlw 31                ; Set loop counter to 31
   movwf loop          

   movf POSTINC0,w         ; Move A to w
   mulwf INDF1             ; PRODH:PRODL = A(0) * B(0)  
   movff PRODL, POSTINC2   ; C(0)=PRODL
   movff PRODH, INDF2      ; C(1)=PRODL

loop01:
   movf POSTINC0, w        ; w=A(i)
   mulwf INDF1             ; PRODH:PRODL = A(i)* B(n)
   movf PRODL, w           ; w=PRODL
   addwf POSTINC2          ; C(n)=C(n)+w
   movf PRODH, w           ; w=PRODH
   addwfc INDF2            ; C(n)=C(n)+w+carry flag
   decfsz loop             ; loop test
   bra loop01              ; jump

   movlw 31                ; Set outer loop counter to 31
   movwf out_loop          

   clrf carryL
   clrf carryH

   movff B, FSR1L
   movff &B+1, FSR1H
loopOuter:

   movff A, FSR0L
   movff &A+1, FSR0H
   movff C, FSR2L
   movff &C+1, FSR2H

   ; Shift index of c by shift 
   incf shift              ; Increment shift 
   movf shift,w            ; Copy shift to w
   addwf FSR2L, 1          ; Add shift to FSR2L
   movlw 0                 ; Zero the w register
   addwfc FSR2H, 1         ; Add carry to FSR2H

   movf POSTINC0,w         ; Move A to w
   mulwf PREINC1           ; PRODH:PRODL = A(i) * B(n)  and ++n
   movf PRODL,w            ; w=PRODL
   addwf POSTINC2          ; C(k)=C(k)+PRODL and k++
   movf PRODH, w           ; w=PRODH
   addwfc INDF2            ; C(k)=C(k)+PRODH+carryflag
   
   clrf carryH
   btfsc STATUS,0          ; test carry flag
   bsf carryH,0            ; carryH=1

   movlw 31                ; Set loop counter to 31
   movwf loop          

loop02:

   movf POSTINC0,w         ; Move A to w
   mulwf INDF1             ; PRODH:PRODL = A(i) * B(n)
   
   movf PRODL,w            ; w=PRODL
   addwf POSTINC2          ; C(k)=C(k)+PRODL and k++
   
   movf PRODH, w           ; w=PRODH
   addwfc INDF2            ; C(k)=C(k)+PRODH+carryflag
   movf carryH, w          ; w=carryL

   clrf carryH             ; carryH=0
   btfsc STATUS,0          ; test for carryflag
   incf carryH             ; carryH++

   addwf INDF2            ; C(k)=C(k)+carryH

   btfsc STATUS,0          ; test for carryflag
   incf carryH             ; carryH++

   decfsz loop             ; loop test
   bra loop02              ; jump

   decfsz out_loop         ; loop test
   bra loopOuter           ; jump
#endasm   
}

/********************************/
/* Function Name: lip_subt      */
/*    C = A - B                 */
/*    A,B,C = 32 byte arrays    */
/*    Return: borrow            */
/********************************/
int lip_subt(byte *A, byte *B, byte *C)
{
BYTE loop;
#asm

   movff A, FSR0L  ; Copy address to FSR for indirect access
   movff &A+1, FSR0H
   movff B, FSR1L
   movff &B+1, FSR1H
   movff C, FSR2L
   movff &C+1, FSR2H
   
   movlw 31                ; Copy 32 to w
   movwf loop              ; Copy w to loop 
   movff POSTINC0,INDF2    ; Copy B to C since adds are done in-place
   movf POSTINC1,0         ; Copy A to W
   subwf POSTINC2          ; C = C-w

loopSub:
   movff POSTINC0,INDF2    ; Copy B to C since adds are done in-place
   movf POSTINC1,0
   subwfb POSTINC2          ; C = C-w-c
   decfsz loop             ; loop test
   bra loopSub             ; jump

   btfsc STATUS.0  ; test borrow
   bra noborrow
   nop
   movlw 0x01
   movwf loop
noborrow: 
#endasm

   return loop;
}

/********************************/
/* Function Name: lip_rshift    */
/*    A  A >> 1                 */
/*    A = 32 byte array         */
/*    Return: N/A               */
/********************************/
void lip_rshift(byte *A)
{
   BYTE loop;
   BYTE *temp;

   temp=A+31;
   
#asm
   movff temp, FSR0L       ; Copy address to FSR for indirect access
   movff &temp+1, FSR0H

   movlw 32                ; Copy 32 to w
   movwf loop              ; Copy w to loop 
   bcf STATUS,0            ; Clear carry flag
loopRot:
   
   rrcf POSTDEC0
   decfsz loop             ; loop test
   bra loopRot             ; jump
#endasm

}

/********************************/
/* Function Name: lip_notzero   */
/*    A = 32 byte arrays        */
/*    Return: 1 if not zero     */
/********************************/
int lip_notzero(byte *A)
{
   byte n;
   for(n=0;n<32;n++)
      if (A[n]!=0)
         break;

   if (n==32)
      return 0;
   else
      return 1;
}

/********************************/
/* Function Name: lip_isone     */
/*    A = 32 byte arrays        */
/*    Return: 1 if value is one */
/********************************/
int lip_isone(byte *A)
{
   byte n;
   for(n=1;n<32;n++)
      if (A[n]!=0)
         break;

   if ((n==32)&&(A[0]==1))
      return 1;
   else
      return 0;
}

/********************************/
/* Function Name: lip_inv       */
/*    B = A^-1                  */
/*    A,B = 32 byte arrays      */
/*    Return: N/A               */
/* Binary Euclidean Algorithm   */
/********************************/
void lip_inv(byte *A, byte *B)
{
   byte n;

   lip_copy(A,u);
   lip_copy(modulus,v);
   for(n=0;n<32;n++)
   {
      x1[n]=0;
      x2[n]=0;
   }
   x1[0]=1;
   while (lip_notzero(u))
   {
      if((u[0]&1)!=1)   
      {
         lip_rshift(u);
         if((x1[0]&1)!=1)  
            lip_rshift(x1);
         else
         {
            lip_add(x1,modulus,tempm);
            lip_rshift(tempm);
            lip_copy(tempm,x1);
         }
      }
      if((v[0]&1)!=1)   /* While v is even */
      {
         lip_rshift(v);
         if((x2[0]&1)!=1)  /* If x1 is even */
            lip_rshift(x2);
         else
         {
            lip_add(x2,modulus,tempm);
            lip_rshift(tempm);
            lip_copy(tempm,x2);
         }
      }
      if(lip_subt(u,v,tempm)==0)    /* If u > v */
      {
         lip_copy(tempm,u);
         lip_sub(x1,x2,tempm);
         lip_copy(tempm,x1);
      }
      else
      {
         lip_sub(v,u,tempm);
         lip_copy(tempm,u);
         lip_sub(x2,x1,tempm);
         lip_copy(tempm,x2);
      }
   }
   if(lip_isone(u))
      lip_copy(x1,B);
   else
      lip_copy(x2,B);
}

/********************************/
/* Function Name: lip_copy      */
/*    B = A                     */
/*    A,B = 32 byte arrays      */
/*    Return: N/A               */
/********************************/
void lip_copy(byte *A, byte *B)
{
BYTE loop;

#asm
   movff A, FSR0L   ; Copy address to FSR for indirect access
   movff &A+1, FSR0H
   movff B, FSR1L
   movff &B+1, FSR1H
   
   movlw 32                ; Copy 32 to w
   movwf loop              ; Copy w to loop 
loopCopy:
   movff POSTINC0,POSTINC1 ; Copy B to A
   decfsz loop             ; loop test
   bra loopCopy            ; jump
#endasm
}

/********************************/
/* Function Name: lip_addt      */
/*    C = A + B                 */
/*    A,B,C = 32 byte arrays    */
/*    Return: carry             */
/********************************/
int lip_addt(byte *A, byte *B, byte *C)
{
BYTE loop;
#asm
   movff A, FSR0L  ; Copy address to FSR for indirect access
   movff &A+1, FSR0H
   movff B, FSR1L
   movff &B+1, FSR1H
   movff C, FSR2L
   movff &C+1, FSR2H
   
   movlw 31                ; Copy 32 to w
   movwf loop              ; Copy w to loop 
   movff POSTINC1,INDF2    ; Copy B to C since adds are done in-place
   movf POSTINC0,0         ; Copy A to W
   addwf POSTINC2          ; C = W+C

loopAdd:
   movff POSTINC1,INDF2    ; Copy B to C since adds are done in-place
   movf POSTINC0,0
   addwfc POSTINC2
   decfsz loop             ; loop test
   bra loopAdd             ; jump

   btfss STATUS.0  ; test carry
   bra nocarry
   nop
   movlw 0x01
   movwf loop
nocarry: 
#endasm
   return loop;
}

void main(void)
{
   lip_inv(x,y);
   while(1);
}

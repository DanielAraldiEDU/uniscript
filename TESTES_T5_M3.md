var a: int = 0;

for(var i: int = 0; i < 10; i++){
    a = a+i;
}

.data
  a_s0: 0
  i_s1: 0
.text
    JMP main
main:
    LDI 0
    STO a_s0
    LDI 0
    STO i_s1
R1:
    LD i_s1
    STO 1000
    LDI 10
    STO 1001
    LD 1000
    SUB 1001
    BGE R2
    LD a_s0
    ADD i_s1
    STO a_s0
    LD i_s1
    ADDI 1
    STO i_s1
    JMP R1
R2:
  HLT 0

RESULTADO NO BIP: a = 45 CORRETO!
------

var a: int = 0;

for(var i: int = 0; i < 10; i = i+2){
    a = a+i;
}

.data
  a_s0: 0
  i_s1: 0
.text
    JMP main
main:
    LDI 0
    STO a_s0
    LDI 0
    STO i_s1
R1:
    LD i_s1
    STO 1000
    LDI 10
    STO 1001
    LD 1000
    SUB 1001
    BGE R2
    LD a_s0
    ADD i_s1
    STO a_s0
    LD i_s1
    ADDI 2
    STO i_s1
    JMP R1
R2:
  HLT 0

RESULTADO NO BIP: a = 20 CORRETO!
---
var abc: int = 0;
var c: int = 3;
for(var i: int = 0; i < 10; i++){
    abc = abc + 6 - c;
    for(var j: int = 0; j < 10; j++){
        abc = abc + j;
    }
}

.data
  abc_s0: 0
  c_s0: 3
  i_s1: 0
  j_s2: 0
.text
    JMP main
main:
    LDI 0
    STO abc_s0
    LDI 3
    STO c_s0
    LDI 0
    STO i_s1
R1:
    LD i_s1
    STO 1000
    LDI 10
    STO 1001
    LD 1000
    SUB 1001
    BGE R2
    LD abc_s0
    STO 1000
    LD 1000
    ADDI 6
    STO 1000
    LD 1000
    SUB c_s0
    STO 1000
    LD 1000
    STO abc_s0
    LDI 0
    STO j_s2
R3:
    LD j_s2
    STO 1000
    LDI 10
    STO 1001
    LD 1000
    SUB 1001
    BGE R4
    LD abc_s0
    ADD j_s2
    STO abc_s0
    LD j_s2
    ADDI 1
    STO j_s2
    JMP R3
R4:
    LD i_s1
    ADDI 1
    STO i_s1
    JMP R1
R2:
  HLT 0

RESULTADO NO BIP = 480 CORRETO!

---

var acc: int = 0;
var data: int[] = [1, 2, 3, 4];

for(var i: int = 0; i < 4; i++){
	var k: int = 0;
	var a: int = 0;
	while(k < 2){
		data[i] = data[i] + k;
		k = k + 1;
        do {
            data[i] = data[i] + k + 3 - a;
            a = a + 1;
        } while(a < 3)
	}
	acc = acc + data[i];
}

.data
  acc_s0: 0
  data_s0: 0,0,0,0
  i_s1: 0
  k_s1: 0
  a_s1: 0
.text
    JMP main
main:
    LDI 0
    STO acc_s0
    LDI 0
    STO data_s0
    LDI 0
    STO $indr
    LDI 1
    STOV data_s0
    LDI 1
    STO $indr
    LDI 2
    STOV data_s0
    LDI 2
    STO $indr
    LDI 3
    STOV data_s0
    LDI 3
    STO $indr
    LDI 4
    STOV data_s0
    LDI 0
    STO i_s1
R1:
    LD i_s1
    STO 1000
    LDI 4
    STO 1001
    LD 1000
    SUB 1001
    BGE R2
    LDI 0
    STO k_s1
    LDI 0
    STO a_s1
R3:
    LD k_s1
    STO 1000
    LDI 2
    STO 1001
    LD 1000
    SUB 1001
    BGE R4
    LD k_s1
    ADDI 1
    STO k_s1
R5:
    LD i_s1
    STO 900
    LD 900
    STO 1002
    LD a_s1
    STO 901
    LDI 3
    STO 902
    LD k_s1
    STO 903
    LD i_s1
    STO 904
    LD 904
    STO 1002
    LD 1002
    STO $indr
    LDV data_s0
    STO 905
    LD 905
    ADD 903
    STO 906
    LD 906
    ADD 902
    STO 907
    LD 907
    SUB 901
    STO 908
    LD 908
    STO 1000
    LD 900
    STO 1002
    LD 1002
    STO $indr
    LD 1000
    STOV data_s0
    LD a_s1
    ADDI 1
    STO a_s1
    LD a_s1
    STO 1000
    LDI 3
    STO 1001
    LD 1000
    SUB 1001
    BLT R5
    JMP R3
R4:
    LD acc_s0
    STO 1000
    LD i_s1
    STO $indr
    LDV data_s0
    STO 1001
    LD 1000
    ADD 1001
    STO 1000
    LD 1000
    STO acc_s0
    LD i_s1
    ADDI 1
    STO i_s1
    JMP R1
R2:
  HLT 0

RESULTADO NO BIP: acc = 45 CORRETO!

---
var vetoor: int[] = [1,2,3,4];
var acc: int = 0;

for(var i: int = 0; i < 4; i++){
    if(i <= 2){
        acc = acc + vetoor[i];
    } else {
        acc = acc + vetoor[i] + 5;
    }
} 


.data
  vetoor_s0: 0,0,0,0
  acc_s0: 0
  i_s1: 0
.text
    JMP main
main:
    LDI 0
    STO $indr
    LDI 1
    STOV vetoor_s0
    LDI 1
    STO $indr
    LDI 2
    STOV vetoor_s0
    LDI 2
    STO $indr
    LDI 3
    STOV vetoor_s0
    LDI 3
    STO $indr
    LDI 4
    STOV vetoor_s0
    LDI 0
    STO acc_s0
    LDI 0
    STO i_s1
R1:
    LD i_s1
    STO 1000
    LDI 4
    STO 1001
    LD 1000
    SUB 1001
    BGE R2
    LD i_s1
    STO 1000
    LDI 2
    STO 1001
    LD 1000
    SUB 1001
    BGT R3
    LD acc_s0
    STO 1000
    LD i_s1
    STO $indr
    LDV vetoor_s0
    STO 1001
    LD 1000
    ADD 1001
    STO 1000
    LD 1000
    STO acc_s0
    JMP R4
R3:
    LD acc_s0
    STO 1000
    LD i_s1
    STO $indr
    LDV vetoor_s0
    STO 1001
    LD 1000
    ADD 1001
    STO 1000
    LD 1000
    ADDI 5
    STO 1000
    LD 1000
    STO acc_s0
R4:
    LD i_s1
    ADDI 1
    STO i_s1
    JMP R1
R2:
  HLT 0

RESULTADO NO BIP: acc = 15 CORRETO!

---

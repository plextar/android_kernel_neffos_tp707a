#include <linux/linkage.h>

#include <asm/opcodes-sec.h>
#include <asm/opcodes-virt.h>
#include <asm/unwind.h>

   .macro SMCCC instr
UNWIND(    .fnstart)
   mov r12, sp
   push    {r4-r7}
UNWIND(    .save   {r4-r7})
   ldm r12, {r4-r7}
   \instr
   pop {r4-r7}
   ldr r12, [sp, #(4 * 4)]
   stm r12, {r0-r3}
   bx  lr
UNWIND(    .fnend)
   .endm

/*
 * void smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2,
 *       unsigned long a3, unsigned long a4, unsigned long a5,
 *       unsigned long a6, unsigned long a7, struct arm_smccc_res *res)
 */
ENTRY(arm_smccc_smc)
   SMCCC __SMC(0)
ENDPROC(arm_smccc_smc)

/*
 * void smccc_hvc(unsigned long a0, unsigned long a1, unsigned long a2,
 *       unsigned long a3, unsigned long a4, unsigned long a5,
 *       unsigned long a6, unsigned long a7, struct arm_smccc_res *res)
 */
ENTRY(arm_smccc_hvc)
   SMCCC __HVC(0)
ENDPROC(arm_smccc_hvc)

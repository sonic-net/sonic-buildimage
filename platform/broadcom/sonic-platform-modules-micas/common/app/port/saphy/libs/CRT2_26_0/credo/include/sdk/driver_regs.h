#ifndef REGS_H
#define REGS_H

/*
 * Register related macros
 */

#define EXPAND__(m, ...) m(__VA_ARGS__)
#define GLUE__(x, y)     x##y

#define NUM_ARGS(...)                                             NUM_ARGS_N(0, __VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, X)
#define IS_SINGLE_ARGS(...)                                       NUM_ARGS_N(0, __VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 1, X)
#define NUM_ARGS_N(X, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N

/* continuous registers */
#define REGBITR(...)      EXPAND__(REGBITR_n, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define REGBITR_n(N, ...) REGBITR_##N(__VA_ARGS__)

#define REGBITR_4(HIVE, BASE_OFFSET, ADDR, LSB) HIVE, (BASE_OFFSET) + (ADDR), 0x1, LSB, LSB
#define REGBITR_5(HIVE, BASE_OFFSET, ADDR, MSB, LSB) \
    HIVE, (BASE_OFFSET) + (ADDR), (((uint64_t)1 << ((MSB) - (LSB) + 1)) - 1), MSB, LSB

/* non-continous registers */ /* example : REGMASK(TOP, 0x2C9, 0x4F00) */
#define REGMASK(...)                             EXPAND__(REGMASK_n, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)
#define REGMASK_n(N, ...)                        REGMASK_##N(__VA_ARGS__)
#define REGMASK_4(HIVE, BASE_OFFSET, ADDR, MASK) HIVE, (BASE_OFFSET) + (ADDR), MASK, 15, 0

#define HIVE(H)            H, 0
#define SUBHIVE(H, OFFSET) H, OFFSET

/* MDIO */
#define addrReg(DIE, REG)                                EXPAND__(ADDR_REG1, DIE, 0, REG)
#define addrRegLane(DIE, LANE, REG)                      EXPAND__(ADDR_REG1, DIE, LANE, REG)
#define ADDR_REG1(DIE, LANE, HIVE, ADDR, MASK, MSB, LSB) cr_addr_reg(DIE, LANE, HIVE, ADDR)
#define ADDR_REG1(DIE, LANE, HIVE, ADDR, MASK, MSB, LSB) cr_addr_reg(DIE, LANE, HIVE, ADDR)
#define addrMsb(REG)                                     EXPAND__(ADDR_MSB1, REG)
#define ADDR_MSB1(HIVE, ADDR, MASK, MSB, LSB)            MSB
#define addrLsb(REG)                                     EXPAND__(ADDR_LSB1, REG)
#define ADDR_LSB1(HIVE, ADDR, MASK, MSB, LSB)            LSB

#define writeTop(DIE, OFFSET, VALUE)                             EXPAND__(WRITE1, DIE, OFFSET, VALUE)
#define readTop(DIE, OFFSET, VALUE)                              EXPAND__(READ1, DIE, OFFSET, VALUE)
#define WRITE1(DIE, OFFSET, VALUE)                               cr_slice_write(DIE, OFFSET, VALUE)
#define READ1(DIE, OFFSET, VALUE)                                cr_slice_read(DIE, OFFSET, VALUE)
#define writeReg(DIE, REG, VALUE)                                EXPAND__(WRITE_REG1, DIE, 0, REG, VALUE)
#define writeRegLane(DIE, LANE, REG, VALUE)                      EXPAND__(WRITE_REG1, DIE, LANE, REG, VALUE)
#define WRITE_REG1(DIE, LANE, HIVE, ADDR, MASK, MSB, LSB, VALUE) cr_write_reg(DIE, LANE, HIVE, ADDR, MASK, LSB, VALUE)
#define readReg(DIE, REG, VALUE)                                 READ_REG1(DIE, 0, REG, VALUE)
#define readRegLane(DIE, LANE, REG, VALUE)                       READ_REG1(DIE, LANE, REG, VALUE)
#define READ_REG1(DIE, LANE, HIVE, ADDR, MASK, MSB, LSB, VALUE)  cr_read_reg(DIE, LANE, HIVE, ADDR, MASK, LSB, VALUE)
#define readRegSigned(DIE, REG, VALUE)                           READ_REG_SIGNED1(DIE, 0, REG, VALUE)
#define readRegSignedLane(DIE, LANE, REG, VALUE)                 READ_REG_SIGNED1(DIE, LANE, REG, VALUE)
#define READ_REG_SIGNED1(DIE, LANE, HIVE, ADDR, MASK, MSB, LSB, VALUE) \
    cr_read_reg_signed(DIE, LANE, HIVE, ADDR, MASK, MSB, LSB, VALUE)

/* TCM */
#define readTCMReg(DIE, REG, VALUE)                            READ_TCM_REG1(DIE, REG, VALUE)
#define READ_TCM_REG1(DIE, HIVE, ADDR, MASK, MSB, LSB, VALUE)  cr_read_tcm_reg(DIE, HIVE, ADDR, MASK, LSB, VALUE)
#define writeTCMReg(DIE, REG, VALUE)                           WRITE_TCM_REG1(DIE, REG, VALUE)
#define WRITE_TCM_REG1(DIE, HIVE, ADDR, MASK, MSB, LSB, VALUE) cr_write_tcm_reg(DIE, HIVE, ADDR, MASK, LSB, VALUE)

#endif

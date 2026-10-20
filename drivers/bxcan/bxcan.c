/*
 * bxcan.c — Driver bxCAN tối giản cho STM32F103 (GĐ 4 ĐAKS)
 * Code do Thắng viết qua các bài 10–13, đã chấm đạt.
 *
 * Các chỗ đã sửa theo lúc chấm (so với bản viết trên giấy):
 *   - can_enter_init: CAN->MSR -> CAN1->MSR, bỏ ";;" thừa, sửa comment INRQ
 *   - can_exit_init : comment "INAK về 0"
 *   - can_send      : ép (uint32_t) từng byte TRƯỚC khi dịch; CAN_TI0R_TXRQ (số 0)
 *   - can_receive   : nhả FIFO bằng "=" (RF0R có bit rc_w1, "|=" sẽ xoá nhầm cờ FOVR/FULL)
 *   - Can_Init      : tên macro RCC_APB2ENR_..., RCC_APB1ENR_..., remap bằng macro REMAP2
 *
 * Cấu hình hiện tại: 500 kbps với APB1 = 36 MHz, chế độ LOOPBACK (LBKM = 1).
 * Chân: PB8 = CAN_RX, PB9 = CAN_TX (remap).
 *
 * TODO:
 *   - Can_Init đang cố định chân PB8/PB9 (remap) và BTR cho APB1 = 36 MHz, chế độ loopback (LBKM = 1).
 *   - Khi ra bus thật: bỏ LBKM (bit 30 của BTR).
 *   - Khi dùng cho F407: tính lại BTR (APB1 = 42 MHz) và chân (PD0/PD1, AF9).
 */
#include "stm32f1xx.h"
#include "bxcan.h"

/* ===== Bài 10: vào chế độ init ===== */
uint8_t can_enter_init(void)
{
    uint32_t count = 0;

    CAN1->MCR &= ~CAN_MCR_SLEEP;   /* thoát chế độ ngủ (sau reset bxCAN đang ngủ) */
    CAN1->MCR |= CAN_MCR_INRQ;     /* yêu cầu vào init */

    /* chờ phần cứng xác nhận INAK = 1, có giới hạn */
    while ((CAN1->MSR & CAN_MSR_INAK) == 0)
    {
        count++;
        if (count > CAN_INIT_TIMEOUT)
        {
            return 1;              /* quá giờ */
        }
    }
    return 0;
}

/* ===== Bài 10: thoát chế độ init ===== */
uint8_t can_exit_init(void)
{
    uint32_t count = 0;

    CAN1->MCR &= ~CAN_MCR_INRQ;    /* yêu cầu rời init */

    /* chờ INAK về 0, có giới hạn */
    while ((CAN1->MSR & CAN_MSR_INAK) != 0)
    {
        count++;
        if (count > CAN_INIT_TIMEOUT)
        {
            return 1;              /* quá giờ */
        }
    }
    return 0;
}

/* ===== Bài 11: bộ lọc 0 nhận mọi khung vào FIFO0 ===== */
void can_filter_accept_all(void)
{
    /* 1. Vào chế độ cấu hình bộ lọc */
    CAN1->FMR |= CAN_FMR_FINIT;

    /* 2. Bộ lọc 0: chế độ mask (FBM0 = 0) */
    CAN1->FM1R &= ~CAN_FM1R_FBM0;

    /* 3. Bộ lọc 0: thang 32 bit (FSC0 = 1) */
    CAN1->FS1R |= CAN_FS1R_FSC0;

    /* 4. Bộ lọc 0: đưa vào FIFO0 (FFA0 = 0) */
    CAN1->FFA1R &= ~CAN_FFA1R_FFA0;

    /* 5. ID = 0, mask = 0 -> không quan tâm bit nào -> nhận tất cả */
    CAN1->sFilterRegister[0].FR1 = 0;   /* ID   */
    CAN1->sFilterRegister[0].FR2 = 0;   /* mask */

    /* 6. Bật bộ lọc 0 */
    CAN1->FA1R |= CAN_FA1R_FACT0;

    /* 7. Thoát chế độ cấu hình bộ lọc (FINIT = 1 thì không nhận khung) */
    CAN1->FMR &= ~CAN_FMR_FINIT;
}

/* ===== Bài 11: gửi 1 khung qua mailbox 0 ===== */
uint8_t can_send(uint16_t id, const uint8_t *data, uint8_t dlc)
{
    /* Bước 1: mailbox 0 có trống không? Không trống thì báo bận */
    if ((CAN1->TSR & CAN_TSR_TME0) == 0)
    {
        return 1;
    }

    /* Bước 2: ghi ID chuẩn 11 bit vào TIR (CHƯA bật TXRQ) */
    CAN1->sTxMailBox[0].TIR = (uint32_t)(id & 0x7FFU) << 21;

    /* Bước 3: ghi DLC */
    CAN1->sTxMailBox[0].TDTR = dlc & 0xFU;

    /* Bước 4: ghép byte 0–3 vào TDLR, byte 4–7 vào TDHR (little endian) */
    CAN1->sTxMailBox[0].TDLR = ((uint32_t)data[3] << 24) | ((uint32_t)data[2] << 16)
                             | ((uint32_t)data[1] << 8)  |  (uint32_t)data[0];
    CAN1->sTxMailBox[0].TDHR = ((uint32_t)data[7] << 24) | ((uint32_t)data[6] << 16)
                             | ((uint32_t)data[5] << 8)  |  (uint32_t)data[4];

    /* Bước 5: bấm nút gửi — bật TXRQ, làm SAU CÙNG */
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;

    return 0;
}

/* ===== Bài 12: đọc 1 khung từ FIFO0 ===== */
uint8_t can_receive(uint16_t *id, uint8_t *data, uint8_t *dlc)
{
    uint32_t lo, hi;

    /* Bước 1: FIFO0 có khung chờ không? (FMP0 = 0 thì rỗng) */
    if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0)
    {
        return 1;
    }

    /* Bước 2: lấy ID 11 bit từ RIR */
    *id = (uint16_t)((CAN1->sFIFOMailBox[0].RIR >> 21) & 0x7FFU);

    /* Bước 3: lấy DLC từ RDTR */
    *dlc = (uint8_t)(CAN1->sFIFOMailBox[0].RDTR & 0xFU);

    /* Bước 4: tách byte (little endian) */
    lo = CAN1->sFIFOMailBox[0].RDLR;
    hi = CAN1->sFIFOMailBox[0].RDHR;
    data[0] = (uint8_t)(lo & 0xFFU);
    data[1] = (uint8_t)((lo >> 8) & 0xFFU);
    data[2] = (uint8_t)((lo >> 16) & 0xFFU);
    data[3] = (uint8_t)((lo >> 24) & 0xFFU);
    data[4] = (uint8_t)(hi & 0xFFU);
    data[5] = (uint8_t)((hi >> 8) & 0xFFU);
    data[6] = (uint8_t)((hi >> 16) & 0xFFU);
    data[7] = (uint8_t)((hi >> 24) & 0xFFU);

    /* Bước 5: nhả ô đầu FIFO0 — làm SAU CÙNG, dùng "=" vì RF0R có bit rc_w1 */
    CAN1->RF0R = CAN_RF0R_RFOM0;

    return 0;
}

/* ===== Bài 13: khởi tạo toàn bộ ===== */
uint8_t Can_Init(void)
{
    /* ① Bật clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;   /* GPIOB, AFIO */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;                        /* CAN1 */

    /* ② Remap CAN1 -> PB8/PB9: xoá trường CAN_REMAP rồi đặt = 10 (nhị phân) */
    AFIO->MAPR &= ~AFIO_MAPR_CAN_REMAP;
    AFIO->MAPR |= AFIO_MAPR_CAN_REMAP_REMAP2;                  /* = (0x2U << 13) */

    /* ③ Chân: PB8 = input pull-up (0x8), PB9 = AF push-pull 50 MHz (0xB) */
    GPIOB->CRH &= ~(0xFFU << 0);                               /* xoá 8 bit của PB8, PB9 */
    GPIOB->CRH |= (0x8U << 0) | (0xBU << 4);
    GPIOB->ODR |= (1U << 8);                                   /* pull-up cho PB8 */

    /* ④ Vào init */
    if (can_enter_init() != 0) { return 1; }

    /* ⑤ Bit timing: BRP=8, TS1=5, TS2=0, SJW=0, LBKM=1 -> 0x40050008 */
    CAN1->BTR = (0U << 24) | (0U << 20) | (5U << 16) | 8U | (1U << 30);

    /* ⑥ Bộ lọc */
    can_filter_accept_all();

    /* ⑦ Thoát init */
    if (can_exit_init() != 0) { return 2; }

    return 0;
}

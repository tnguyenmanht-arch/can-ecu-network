/*
 * SchM_Com.h — Vùng tới hạn của COM (tên theo AUTOSAR SchM = Schedule Manager).
 * Bộ đệm PDU nhận vừa bị ISR ghi (Com_RxIndication), vừa bị task đọc, nên COM bọc
 * đoạn chép bộ đệm giữa SchM_Enter_Com / SchM_Exit_Com.
 * Mỗi ECU tự hiện thực 2 hàm này (code COM giống hệt nhau, chỉ khác ở đây):
 *   - ECU1 (Trampoline): SuspendOSInterrupts / ResumeOSInterrupts
 *   - ECU2, ECU3 (F103 bare-metal): tắt / bật ngắt
 * File này CHỈ khai báo.
 */
#ifndef SCHM_COM_H
#define SCHM_COM_H

/* Vào vùng tới hạn: chặn ISR CAN RX cho tới SchM_Exit_Com.
 * Ai gọi: COM, trước khi chép bộ đệm PDU dùng chung với ISR.
 * Chạy ở: task.
 * Trả về: không. */
void SchM_Enter_Com(void);

/* Ra vùng tới hạn: mở lại ngắt đã chặn ở SchM_Enter_Com.
 * Ai gọi: COM, ngay sau đoạn chép bộ đệm.
 * Chạy ở: task.
 * Trả về: không. */
void SchM_Exit_Com(void);

#endif /* SCHM_COM_H */

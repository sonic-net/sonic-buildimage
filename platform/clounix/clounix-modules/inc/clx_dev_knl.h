
#ifndef NETIF_PKT_KNL_H
#define NETIF_PKT_KNL_H

#define NETIF_PKT_PKT_TX_GPD_NUM      (1024)
#define NETIF_PKT_PKT_RX_GPD_NUM      (1024)
#define NETIF_PKT_PKT_GPD_BUF_LEN     (1024)
#define NETIF_PKT_PKT_RX_SCHED_MODE   (0)
#define NETIF_PKT_PKT_TX_QUEUE_LEN    (NETIF_PKT_PKT_TX_GPD_NUM * 10)
#define NETIF_PKT_PKT_RX_QUEUE_LEN    (NETIF_PKT_PKT_RX_GPD_NUM * 10)
#define NETIF_PKT_PKT_RX_QUEUE_WEIGHT (0)

#define NETIF_PKT_PKT_RX_ISR_THREAD_PRI   (80)
#define NETIF_PKT_PKT_RX_ISR_THREAD_STACK (64 * 1024)
/* customize PKT RX ISR thread stack size in bytes */
#define NETIF_PKT_PKT_RX_FREE_THREAD_PRI   (80)
#define NETIF_PKT_PKT_RX_FREE_THREAD_STACK (64 * 1024)
/* customize PKT RX free thread stack size in bytes */
#define NETIF_PKT_PKT_TX_ISR_THREAD_PRI   (80)
#define NETIF_PKT_PKT_TX_ISR_THREAD_STACK (64 * 1024)
/* customize PKT TX ISR thread stack size in bytes */
#define NETIF_PKT_PKT_TX_FREE_THREAD_PRI   (80)
#define NETIF_PKT_PKT_TX_FREE_THREAD_STACK (64 * 1024)
/* customize PKT TX free thread stack size in bytes */
#define NETIF_PKT_PKT_ERROR_ISR_THREAD_PRI   (80)
#define NETIF_PKT_PKT_ERROR_ISR_THREAD_STACK (64 * 1024)
/* customize PKT ERROR ISR thread stack size in bytes */

#define HAL_EXCPT_CPU_NUM          (256)
#define HAL_EXCPT_CPU_BASE_ID      (28 * 1024)
#define HAL_EXCPT_CPU_NON_L3_MIN   (0)
#define HAL_EXCPT_CPU_NON_L3_MAX   (HAL_EXCPT_CPU_NON_L3_MIN + HAL_EXCPT_CPU_NUM - 1)
#define HAL_EXCPT_CPU_L3_MIN       (HAL_EXCPT_CPU_NON_L3_MIN + HAL_EXCPT_CPU_NUM)
#define HAL_EXCPT_CPU_L3_MAX       (HAL_EXCPT_CPU_L3_MIN + HAL_EXCPT_CPU_NUM - 1)
#define HAL_INVALID_NVO3_ENCAP_IDX (0x3FFF)
#define HAL_INVALID_NVO3_ADJ_IDX   (0xFF)

/* #define AML_EN_I2C             */
/* #define AML_EN_CUSTOM_DMA_ADDR */
#define HAL_CLX_VENDOR_ID  (0x0E8D)
#define HAL_CL_VENDOR_ID   (0x1D9F)
#define HAL_PCIE_VENDOR_ID (0x1F83)

#define HAL_DEVICE_ID_CL8300 (0x8300)
#define HAL_DEVICE_ID_CL8400 (0x8400)
#define HAL_DEVICE_ID_CL8500 (0x8500)
#define HAL_DEVICE_ID_CL8600 (0x8600)

#define HAL_REVISION_ID_E1 (0x01)
#define HAL_REVISION_ID_E2 (0x02)

#define HAL_INVALID_DEVICE_ID (0xFFFFFFFF)

#define NETIF_KNL_DEVICE_IS_NAMCHABARWA(__dev_id__) (HAL_DEVICE_ID_CL8600 == (__dev_id__ & 0xFF00))
#define NETIF_KNL_DEVICE_IS_KAWAGARBO(__dev_id__)   (HAL_DEVICE_ID_CL8400 == (__dev_id__ & 0xFF00))

#define NETIF_KNL_DEVICE_IS_LIGHTNING(__dev_id__) (HAL_DEVICE_ID_CL8500 == (__dev_id__ & 0xFF00))
#define NETIF_KNL_DEVICE_IS_DAWN(__dev_id__)      (HAL_DEVICE_ID_CL8300 == (__dev_id__ & 0xFF00))

#endif

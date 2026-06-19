#ifndef DB_MANAGER_NETWORK_H
#define DB_MANAGER_NETWORK_H

/**
 * @brief Checks whether the local system has internet connectivity /
 *        本地系统联网检测
 *
 *        Performs two checks:
 *        1. Local physical network interface (wired/wireless) is up and running
 *        2. Outbound TCP probe to well-known public servers succeeds
 *        Both must pass to return 1.
 *
 *        执行两项检测：
 *        1. 本地物理网卡（有线/无线）已激活联通
 *        2. 外网 TCP 探测成功
 *        两项都通过才返回1。
 *
 * @return If both checks pass return 1, otherwise return 0 /
 *         两项检测都通过返回1，反之返回0
 */
int network_status_probe(void);

#endif // DB_MANAGER_NETWORK_H

#define _GNU_SOURCE

#include "network.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief Checks whether the current network interface is virtual /
 *        判断是否是虚拟网卡
 *
 * @param if_name Network interface name / 网卡名
 * @return Return 1 if the network interface is virtual, otherwise return 0 /
 *         如果网卡是虚拟网卡返回1,反之返回0
 *
 * @author Dawson Huang
 * @date 2026-6-17
 */
int is_vir_if(const char *if_name)
{
    const char *virtual_prefixes[] = {"docker", "virbr", "veth", "tun", "tap", "br-", "lo"};
    const size_t prefix_count = sizeof(virtual_prefixes) / sizeof(virtual_prefixes[0]);

    for (size_t prefix_index = 0; prefix_index < prefix_count; prefix_index++)
    {
        size_t prefix_len = strlen(virtual_prefixes[prefix_index]);

        if (strncmp(if_name, virtual_prefixes[prefix_index], prefix_len) == 0)
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Checks whether the current network interface is physical /
 *        判断是否是物理网卡
 *
 * @param if_name Network interface name / 网卡名
 * @return Return 1 if the network interface is physical, otherwise return 0 /
 *         如果网卡是物理网卡返回1，反之返回0
 *
 * @author Dawson Huang
 * @date 2026-6-17
 */
int is_phy_if(const char *if_name)
{
    return (if_name[0] == 'e' && if_name[1] == 't' && if_name[2] == 'h') || (if_name[0] == 'e' && if_name[1] == 'n') ||
           (if_name[0] == 'w' && if_name[1] == 'l');
}

/**
 * @brief Checks whether an active physical network interface exists /
 *        判断本地物理网卡（有线/无线）是否联通
 *
 * @return Return 1 if has an active physical network interface, otherwise
 *         return 0 / 如果有已激活的物理网卡返回1，反之返回0
 *
 * @author Dawson Huang
 * @date 2026-6-17
 */
int has_active_if(void)
{
    struct ifaddrs *if_addrs = NULL;

    if (getifaddrs(&if_addrs) != 0)
    {
        return 0;
    }

    int found_active_physical = 0;

    for (struct ifaddrs *entry = if_addrs; entry != NULL; entry = entry->ifa_next)
    {
        if (entry->ifa_addr == NULL || entry->ifa_name == NULL)
        {
            continue;
        }

        unsigned int addrs_family = entry->ifa_addr->sa_family;

        if (addrs_family != AF_INET && addrs_family != AF_INET6)
        {
            continue;
        }

        unsigned int if_flags = entry->ifa_flags;

        if (!(if_flags & IFF_UP) || !(if_flags & IFF_RUNNING))
        {
            continue;
        }

        if (is_phy_if(entry->ifa_name))
        {
            found_active_physical = 1;

            break;
        }
    }

    freeifaddrs(if_addrs);

    return found_active_physical;
}

typedef struct
{
    char *ip_addrs;
    int port_number;
} ip_info; // Internet port information

/**
 * @brief Probes internet connectivity by attempting TCP connections to
 *        well-known public servers / 通过 TCP 连接探测外网连通性
 *
 * @return Return 1 if at least one probe succeeds, otherwise return 0 /
 *         至少一个探测目标连通返回1，反之返回0
 */
int probe_internet()
{
    ip_info probe_targets[] = {
        {"8.8.8.8", 53},   {"1.1.1.1", 53},         {"208.67.222.222", 53}, {"9.9.9.9", 53},
        {"1.1.1.1", 80},   {"93.184.216.34", 80},   {"8.8.8.8", 443},       {"1.1.1.1", 443},
        {"223.5.5.5", 53}, {"114.114.114.114", 53}, {"119.29.29.29", 53},
    };

    int target_count = sizeof(probe_targets) / sizeof(probe_targets[0]);

    int sockets[target_count];
    struct pollfd fds[target_count];
    int active_count = 0;

    /* Initialize all entries to ensure poll() never sees garbage */
    for (int i = 0; i < target_count; i++)
    {
        sockets[i] = -1;
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    for (int i = 0; i < target_count; i++)
    {
        int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

        if (sock < 0)
        {
            continue;
        }

        struct sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons((in_port_t)probe_targets[i].port_number);

        if (inet_pton(AF_INET, probe_targets[i].ip_addrs, &addr.sin_addr) != 1)
        {
            close(sock);

            continue;
        }

        int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));

        if (ret < 0 && errno != EINPROGRESS)
        {
            close(sock);

            continue;
        }

        sockets[i] = sock;
        fds[i].fd = sock;
        fds[i].events = POLLOUT;

        active_count++;
    }

    if (active_count == 0)
    {
        return 0;
    }

    int ready = poll(fds, target_count, 2000);

    if (ready <= 0)
    {
        for (int i = 0; i < target_count; i++)
        {
            if (sockets[i] >= 0)
            {
                close(sockets[i]);
            }
        }

        return 0;
    }

    for (int i = 0; i < target_count; i++)
    {
        if (fds[i].fd < 0)
        {
            continue;
        }

        if (!(fds[i].revents & POLLOUT))
        {
            continue;
        }

        int sock_err = 0;
        socklen_t err_len = sizeof(sock_err);

        if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &sock_err, &err_len) == 0 && sock_err == 0)
        {
            for (int j = 0; j < target_count; j++)
            {
                if (sockets[j] >= 0)
                {
                    close(sockets[j]);
                }
            }

            return 1;
        }
    }

    for (int i = 0; i < target_count; i++)
    {
        if (sockets[i] >= 0)
        {
            close(sockets[i]);
        }
    }

    return 0;
}

/**
 * @brief Checks whether the local system has internet connectivity /
 *        本地系统联网检测
 *
 *        Performs two checks:
 *        - Local physical network interface (wired/wireless) is up and running
 *        - Outbound TCP probe to well-known public servers succeeds
 *        Both must pass to return 1.
 *
 *        执行两项检测：
 *        - 本地物理网卡（有线/无线）已激活联通
 *        - 外网 TCP 探测成功
 *        两项都通过才返回1。
 *
 * @return Return 1 if both checks pass, otherwise return 0 /
 *         两项检测都通过返回1，反之返回0
 *
 * @author Dawson Huang
 * @date 2026-6-17
 */
int network_status_probe()
{
    return has_active_if() && probe_internet();
}

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <netdb.h>

/* QEMU user-mode networking defaults:
 * Gateway/DNS: 10.0.2.2
 * Guest IP:    10.0.2.15
 * Netmask:     255.255.255.0
 */

static int set_ip(int fd, const char *ifname, const char *ip, unsigned long req)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
	addr->sin_family = AF_INET;
	inet_pton(AF_INET, ip, &addr->sin_addr);
	return ioctl(fd, req, &ifr);
}

static int bring_up(int fd, const char *ifname)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) return -1;
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	return ioctl(fd, SIOCSIFFLAGS, &ifr);
}

static int add_default_route(const char *gateway)
{
	struct rtentry rt;
	memset(&rt, 0, sizeof(rt));
	struct sockaddr_in *dst = (struct sockaddr_in *)&rt.rt_dst;
	struct sockaddr_in *gw = (struct sockaddr_in *)&rt.rt_gateway;
	struct sockaddr_in *mask = (struct sockaddr_in *)&rt.rt_genmask;
	dst->sin_family = AF_INET;
	gw->sin_family = AF_INET;
	mask->sin_family = AF_INET;
	inet_pton(AF_INET, gateway, &gw->sin_addr);
	rt.rt_flags = RTF_UP | RTF_GATEWAY;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) return -1;
	int rc = ioctl(fd, SIOCADDRT, &rt);
	close(fd);
	return rc;
}

static int configure_network(const char *ifname)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		printf("Cannot create socket: errno=%d\n", errno);
		return -1;
	}

	printf("Bringing up %s...\n", ifname);
	if (bring_up(fd, ifname) < 0)
	{
		printf("Failed to bring up %s: errno=%d\n", ifname, errno);
		close(fd);
		return -1;
	}

	printf("Setting IP 10.0.2.15...\n");
	if (set_ip(fd, ifname, "10.0.2.15", SIOCSIFADDR) < 0)
	{
		printf("Failed to set IP: errno=%d\n", errno);
		close(fd);
		return -1;
	}

	printf("Setting netmask 255.255.255.0...\n");
	if (set_ip(fd, ifname, "255.255.255.0", SIOCSIFNETMASK) < 0)
	{
		printf("Failed to set netmask: errno=%d\n", errno);
		close(fd);
		return -1;
	}

	close(fd);

	printf("Adding default route via 10.0.2.2...\n");
	if (add_default_route("10.0.2.2") < 0)
	{
		printf("Failed to add route: errno=%d\n", errno);
		return -1;
	}

	return 0;
}

static int test_connection(const char *host, int port)
{
	printf("Resolving %s...\n", host);
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	char port_str[8];
	snprintf(port_str, sizeof(port_str), "%d", port);
	int rc = getaddrinfo(host, port_str, &hints, &res);
	if (rc != 0)
	{
		printf("DNS lookup failed: %d\n", rc);
		return -1;
	}
	char ip_str[32];
	struct sockaddr_in *resolved = (struct sockaddr_in *)res->ai_addr;
	inet_ntop(AF_INET, &resolved->sin_addr, ip_str, sizeof(ip_str));
	printf("Resolved to %s\n", ip_str);

	printf("Connecting to %s:%d...\n", ip_str, port);
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		printf("Socket error: errno=%d\n", errno);
		freeaddrinfo(res);
		return -1;
	}

	if (connect(fd, res->ai_addr, res->ai_addrlen) < 0)
	{
		printf("Connect failed: errno=%d\n", errno);
		close(fd);
		freeaddrinfo(res);
		return -1;
	}

	freeaddrinfo(res);
	printf("Connected!\n");

	const char *req_fmt = "GET / HTTP/1.0\r\nHost: %s\r\n\r\n";
	char req[256];
	snprintf(req, sizeof(req), req_fmt, host);
	write(fd, req, strlen(req));

	char buf[4096];
	int total = 0;
	int n;
	while ((n = read(fd, buf + total, sizeof(buf) - 1 - total)) > 0)
		total += n;
	if (total > 0)
	{
		buf[total] = '\0';
		printf("Received %d bytes:\n%s\n", total, buf);
	}
	else
	{
		printf("No data received\n");
	}

	close(fd);
	return 0;
}

int main(int argc, char *argv[])
{
	const char *ifname = "eth0";
	if (argc > 1) ifname = argv[1];

	printf("Network test\n");
	printf("============\n");

	if (configure_network(ifname) < 0)
	{
		printf("\nNetwork configuration failed.\n");
		printf("Try 'nettest ens3' or 'nettest enp0s3' if eth0 doesn't work.\n");
		return 1;
	}

	printf("\nNetwork configured successfully!\n\n");

	/* Connect to mlgsoft.com port 80 */
	test_connection("mlgsoft.com", 80);

	return 0;
}

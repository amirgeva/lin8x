#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 2222
#define BUF_SIZE 4096

static int run_command(int client_fd, char *command)
{
	int pipefd[2];
	if (pipe(pipefd) < 0)
	{
		dprintf(client_fd, "pipe error: %d\n", errno);
		return -1;
	}

	pid_t pid = fork();
	if (pid == 0)
	{
		/* child: redirect stdout and stderr to pipe */
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);
		execl("/bin/sh", "sh", "-c", "--", command, (char *)0);
		_exit(127);
	}
	else if (pid < 0)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		dprintf(client_fd, "fork error: %d\n", errno);
		return -1;
	}

	close(pipefd[1]);

	/* read output and send to client */
	char buf[BUF_SIZE];
	int n;
	while ((n = read(pipefd[0], buf, sizeof(buf))) > 0)
		write(client_fd, buf, n);
	close(pipefd[0]);

	int status;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

static void handle_client(int client_fd)
{
	dprintf(client_fd, "lin8x remote shell\n");

	char buf[BUF_SIZE];
	int buf_len = 0;

	while (1)
	{
		dprintf(client_fd, "$ ");
		/* read until newline */
		while (1)
		{
			int n = read(client_fd, buf + buf_len, 1);
			if (n <= 0)
				return;
			if (buf[buf_len] == '\n' || buf[buf_len] == '\r')
			{
				buf[buf_len] = '\0';
				break;
			}
			buf_len++;
			if (buf_len >= BUF_SIZE - 1)
			{
				buf[buf_len] = '\0';
				break;
			}
		}

		if (buf_len == 0)
		{
			buf_len = 0;
			continue;
		}

		if (strcmp(buf, "exit") == 0)
		{
			dprintf(client_fd, "bye\n");
			return;
		}

		/* handle cd as builtin */
		if (strncmp(buf, "cd", 2) == 0 && (buf[2] == ' ' || buf[2] == '\0'))
		{
			char *dir = buf + 2;
			while (*dir == ' ') dir++;
			if (*dir == '\0') dir = "/";
			if (chdir(dir) != 0)
				dprintf(client_fd, "cd: %s: errno %d\n", dir, errno);
			buf_len = 0;
			continue;
		}

		run_command(client_fd, buf);
		buf_len = 0;
	}
}

static int net_set_ip(int fd, const char *ifname, const char *ip, unsigned long req)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
	addr->sin_family = AF_INET;
	inet_pton(AF_INET, ip, &addr->sin_addr);
	return ioctl(fd, req, &ifr);
}

static int net_bring_up(int fd, const char *ifname)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) return -1;
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	return ioctl(fd, SIOCSIFFLAGS, &ifr);
}

static int configure_network(const char *ifname)
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) return -1;

	if (net_bring_up(fd, ifname) < 0) { close(fd); return -1; }
	if (net_set_ip(fd, ifname, "10.0.2.15", SIOCSIFADDR) < 0) { close(fd); return -1; }
	if (net_set_ip(fd, ifname, "255.255.255.0", SIOCSIFNETMASK) < 0) { close(fd); return -1; }
	close(fd);

	/* add default route */
	struct rtentry rt;
	memset(&rt, 0, sizeof(rt));
	struct sockaddr_in *dst = (struct sockaddr_in *)&rt.rt_dst;
	struct sockaddr_in *gw = (struct sockaddr_in *)&rt.rt_gateway;
	struct sockaddr_in *mask = (struct sockaddr_in *)&rt.rt_genmask;
	dst->sin_family = AF_INET;
	gw->sin_family = AF_INET;
	mask->sin_family = AF_INET;
	inet_pton(AF_INET, "10.0.2.2", &gw->sin_addr);
	rt.rt_flags = RTF_UP | RTF_GATEWAY;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) return -1;
	int rc = ioctl(fd, SIOCADDRT, &rt);
	close(fd);
	return rc;
}

int main(int argc, char *argv[])
{
	int port = PORT;
	const char *ifname = "eth0";
	if (argc > 1) port = atoi(argv[1]);
	if (argc > 2) ifname = argv[2];

	printf("Configuring network on %s...\n", ifname);
	if (configure_network(ifname) < 0)
		printf("Network config failed (may already be configured)\n");
	else
		printf("Network configured\n");

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		printf("socket error: %d\n", errno);
		return 1;
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("bind error: %d\n", errno);
		close(server_fd);
		return 1;
	}

	if (listen(server_fd, 1) < 0)
	{
		printf("listen error: %d\n", errno);
		close(server_fd);
		return 1;
	}

	printf("Remote shell listening on port %d\n", port);

	while (1)
	{
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
		if (client_fd < 0)
		{
			printf("accept error: %d\n", errno);
			continue;
		}

		char client_ip[32];
		inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
		printf("Connection from %s\n", client_ip);

		handle_client(client_fd);
		close(client_fd);
		printf("Client disconnected\n");
	}

	close(server_fd);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>

int run_command(char* command)
{
	char *args[256];
	int i = 0;
	char *token = strtok(command, " ");
	while (token != NULL)
	{
		args[i++] = token;
		token = strtok(NULL, " ");
	}
	args[i] = NULL; // null-terminate the array
	pid_t pid = fork();
	if (pid == 0)
	{
		// Child process
		execvp(args[0], args);
		perror("execvp failed");
		exit(1);
	}
	else if (pid < 0)
	{
		perror("fork failed");
		return -1;
	}
	else
	{
		int status;
		// Parent process
		wait(&status); // wait for child to finish
		if (WIFEXITED(status))
		{
			return WEXITSTATUS(status); // return child's exit status
		}
		else if (WIFSIGNALED(status))
		{
			fprintf(stderr, "Child process terminated by signal %d\n", WTERMSIG(status));
			return -1;
		}
		else
		{
			fprintf(stderr, "Child process terminated abnormally\n");
			return -1;
		}
		return -1;
	}
	return 0;
}

typedef int (*builtin_fn)(char *args);

struct builtin {
	const char *name;
	builtin_fn fn;
};

static struct builtin builtins[];

static int builtin_exit(char *args)
{
	return -1;
}

static int builtin_help(char *args)
{
	printf("Built-in commands:");
	for (int i = 0; builtins[i].name != NULL; i++)
		printf(" %s", builtins[i].name);
	printf("\n");
	return 0;
}

static int builtin_cd(char *args)
{
	while (*args == ' ') args++;
	if (*args == '\0') args = "/";
	if (chdir(args) != 0)
		perror("cd");
	return 0;
}

static struct builtin builtins[] = {
	{ "cd",   builtin_cd },
	{ "exit", builtin_exit },
	{ "help", builtin_help },
	{ NULL, NULL }
};

static int try_builtin(char *command)
{
	for (int i = 0; builtins[i].name != NULL; i++)
	{
		int len = strlen(builtins[i].name);
		if (strncmp(command, builtins[i].name, len) == 0 &&
		    (command[len] == ' ' || command[len] == '\0'))
		{
			return builtins[i].fn(command + len);
		}
	}
	return 1;
}

static struct termios orig_termios;

static void raw_mode_on(void)
{
	struct termios raw;
	tcgetattr(STDIN_FILENO, &orig_termios);
	raw = orig_termios;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void raw_mode_off(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

static void redraw_line(const char *prompt, const char *buf, int len, int pos)
{
	printf("\r%s%s", prompt, buf);
	/* clear to end of line */
	printf("\x1b[K");
	/* move cursor to position */
	if (pos < len)
	{
		int back = len - pos;
		printf("\x1b[%dD", back);
	}
	fflush(stdout);
}

static int find_word_start(const char *buf, int pos)
{
	int i = pos;
	while (i > 0 && buf[i - 1] != ' ')
		i--;
	return i;
}

static int is_first_word(const char *buf, int pos)
{
	int i;
	for (i = 0; i < pos; i++)
		if (buf[i] == ' ') return 0;
	return 1;
}

static void tab_complete(char *buf, int *len, int *pos, const char *prompt)
{
	int word_start = find_word_start(buf, *pos);
	int prefix_len = *pos - word_start;
	char prefix[256];
	if (prefix_len <= 0 || prefix_len >= 256) return;
	strncpy(prefix, buf + word_start, prefix_len);
	prefix[prefix_len] = '\0';

	const char *search_dir;
	int completing_command = is_first_word(buf, *pos);
	if (completing_command)
		search_dir = "/bin";
	else
		search_dir = ".";

	/* also check builtins for first word */
	char matches[64][256];
	int match_count = 0;

	if (completing_command)
	{
		int i;
		for (i = 0; builtins[i].name != NULL && match_count < 64; i++)
		{
			if (strncmp(builtins[i].name, prefix, prefix_len) == 0)
			{
				strncpy(matches[match_count], builtins[i].name, 255);
				matches[match_count][255] = '\0';
				match_count++;
			}
		}
	}

	DIR *dir = opendir(search_dir);
	if (dir)
	{
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL && match_count < 64)
		{
			if (entry->d_name[0] == '.')
				continue;
			if (strncmp(entry->d_name, prefix, prefix_len) == 0)
			{
				strncpy(matches[match_count], entry->d_name, 255);
				matches[match_count][255] = '\0';
				match_count++;
			}
		}
		closedir(dir);
	}

	if (match_count == 0)
		return;

	if (match_count == 1)
	{
		/* single match: complete it */
		const char *suffix = matches[0] + prefix_len;
		int suffix_len = strlen(suffix);
		/* make room in buffer */
		memmove(buf + *pos + suffix_len, buf + *pos, *len - *pos + 1);
		memcpy(buf + *pos, suffix, suffix_len);
		*len += suffix_len;
		*pos += suffix_len;
		buf[*len] = '\0';
		redraw_line(prompt, buf, *len, *pos);
	}
	else
	{
		/* find common prefix among matches */
		int common = strlen(matches[0]);
		int i;
		for (i = 1; i < match_count; i++)
		{
			int j;
			for (j = 0; j < common; j++)
			{
				if (matches[i][j] != matches[0][j])
				{
					common = j;
					break;
				}
			}
		}
		int extra = common - prefix_len;
		if (extra > 0)
		{
			/* complete the common part */
			memmove(buf + *pos + extra, buf + *pos, *len - *pos + 1);
			memcpy(buf + *pos, matches[0] + prefix_len, extra);
			*len += extra;
			*pos += extra;
			buf[*len] = '\0';
			redraw_line(prompt, buf, *len, *pos);
		}
		else
		{
			/* show all matches */
			printf("\n");
			for (i = 0; i < match_count; i++)
				printf("%s  ", matches[i]);
			printf("\n");
			redraw_line(prompt, buf, *len, *pos);
		}
	}
}

#define HISTORY_SIZE 32

static char history[HISTORY_SIZE][256];
static int history_count = 0;
static int history_head = 0;

static void history_add(const char *cmd)
{
	/* don't add duplicates of the last entry */
	if (history_count > 0)
	{
		int last = (history_head + HISTORY_SIZE - 1) % HISTORY_SIZE;
		if (strcmp(history[last], cmd) == 0)
			return;
	}
	strncpy(history[history_head], cmd, 255);
	history[history_head][255] = '\0';
	history_head = (history_head + 1) % HISTORY_SIZE;
	if (history_count < HISTORY_SIZE)
		history_count++;
}

static const char *history_get(int index)
{
	/* index 0 = most recent, index history_count-1 = oldest */
	int pos = (history_head + HISTORY_SIZE - 1 - index) % HISTORY_SIZE;
	return history[pos];
}

static int read_line(const char *prompt, char *buf, int bufsize)
{
	int len = 0;
	int pos = 0;
	int hist_pos = -1;
	char saved[256];
	saved[0] = '\0';
	buf[0] = '\0';

	printf("%s", prompt);
	fflush(stdout);
	raw_mode_on();

	while (1)
	{
		char c;
		if (read(STDIN_FILENO, &c, 1) != 1)
		{
			raw_mode_off();
			return -1;
		}
		if (c == '\n' || c == '\r')
		{
			printf("\n");
			fflush(stdout);
			raw_mode_off();
			buf[len] = '\0';
			return len;
		}
		else if (c == '\t')
		{
			tab_complete(buf, &len, &pos, prompt);
		}
		else if (c == 127 || c == 8)
		{
			/* backspace */
			if (pos > 0)
			{
				memmove(buf + pos - 1, buf + pos, len - pos + 1);
				pos--;
				len--;
				buf[len] = '\0';
				redraw_line(prompt, buf, len, pos);
			}
		}
		else if (c == 3)
		{
			/* ctrl-c: clear line */
			len = 0;
			pos = 0;
			buf[0] = '\0';
			hist_pos = -1;
			printf("\n");
			redraw_line(prompt, buf, len, pos);
		}
		else if (c == 27)
		{
			/* escape sequence */
			char seq[2];
			if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
			if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
			if (seq[0] == '[')
			{
				if (seq[1] == 'A')
				{
					/* up arrow - older history */
					if (hist_pos < history_count - 1)
					{
						if (hist_pos == -1)
						{
							strncpy(saved, buf, 255);
							saved[255] = '\0';
						}
						hist_pos++;
						strncpy(buf, history_get(hist_pos), bufsize - 1);
						buf[bufsize - 1] = '\0';
						len = strlen(buf);
						pos = len;
						redraw_line(prompt, buf, len, pos);
					}
				}
				else if (seq[1] == 'B')
				{
					/* down arrow - newer history */
					if (hist_pos >= 0)
					{
						hist_pos--;
						if (hist_pos == -1)
						{
							strncpy(buf, saved, bufsize - 1);
							buf[bufsize - 1] = '\0';
						}
						else
						{
							strncpy(buf, history_get(hist_pos), bufsize - 1);
							buf[bufsize - 1] = '\0';
						}
						len = strlen(buf);
						pos = len;
						redraw_line(prompt, buf, len, pos);
					}
				}
				else if (seq[1] == 'D' && pos > 0)
				{
					/* left arrow */
					pos--;
					printf("\x1b[D");
					fflush(stdout);
				}
				else if (seq[1] == 'C' && pos < len)
				{
					/* right arrow */
					pos++;
					printf("\x1b[C");
					fflush(stdout);
				}
			}
		}
		else if (c >= 32)
		{
			/* printable character */
			if (len < bufsize - 1)
			{
				memmove(buf + pos + 1, buf + pos, len - pos + 1);
				buf[pos] = c;
				pos++;
				len++;
				buf[len] = '\0';
				redraw_line(prompt, buf, len, pos);
			}
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc > 3)
	{
		if (strcmp(argv[1], "-c") == 0 && strcmp(argv[2], "--")==0)
		{
			return run_command(argv[3]);
		}
	}
	else
	while (1)
	{
		char cwd[256];
		getcwd(cwd, 256);
		char prompt[280];
		snprintf(prompt, sizeof(prompt), "%s> ", cwd);
		char command[256];
		int n = read_line(prompt, command, sizeof(command));
		if (n < 0)
			break;
		if (n == 0)
			continue;
		history_add(command);
		int rc = try_builtin(command);
		if (rc < 0)
			break;
		if (rc > 0)
			run_command(command);
	}
	return 0;
}


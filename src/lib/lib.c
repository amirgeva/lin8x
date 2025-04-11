#include <fcntl.h>    // For open()
#include <unistd.h>   // For close(), read(), write()
#include <sys/types.h> // For data types like ssize_t
#include <sys/stat.h>  // For file permissions
#include <stdio.h>
#include <string.h>

typedef struct file_header
{
	char filename[16];
	char filemod[12];
	char owner_id[6];
	char group_id[6];
	char file_mode[8];
	char file_size[10];
	char suffix[2];
} FileHeader;

void copy_str(char *dst, const char *src, size_t size)
{
	for (size_t i = 0; i < size && *src;++i)
	{
		*dst++ = *src++;
	}
}

const char *strip_dirs(const char *path)
{
	const char *filename = path;
	while (*path) {
		if (*path == '/' || *path == '\\') {
			filename = path + 1;
		}
		path++;
	}
	return filename;
}

int	main(int argc, char *argv[])
{
	if (argc<2)
	{
		printf("Usage: %s <output_file> <input_file1> <input_file2> ...\n", argv[0]);
		return 1;
	}
	// Parameters are:   output_file, input_file, input_file ...
	// Open the output file
	int fd_out = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd_out < 0) {
		perror("Error opening output file");
		return 1;
	}
	char data_buffer[1024 * 1024];
	// Write output file header
	write(fd_out,"!<arch>\n", 8);
	// Loop through each input file
	for (int i = 2; i < argc; i++) {
		// Open the input file
		int fd_in = open(argv[i], O_RDONLY);
		if (fd_in < 0) {
			perror("Error opening input file");
			close(fd_out);
			return 1;
		}
		// Get the file size
		struct stat st;
		if (fstat(fd_in, &st) < 0) {
			perror("Error getting file size");
			close(fd_in);
			close(fd_out);
			return 1;
		}
		ssize_t file_size = st.st_size;

		FileHeader header;
		char field[32];
		memset(&header, ' ', sizeof(header)); // Initialize header with spaces
		copy_str(header.filename, strip_dirs(argv[i]), sizeof(header.filename));
		copy_str(header.filemod, "0", sizeof(header.filemod));
		copy_str(header.owner_id, "0", sizeof(header.owner_id));
		copy_str(header.group_id, "0", sizeof(header.group_id));
		copy_str(header.file_mode, "644", sizeof(header.file_mode));
		snprintf(field, sizeof(field), "%ld", file_size);
		copy_str(header.file_size, field, sizeof(header.file_size));
		header.suffix[0] = '`';
		header.suffix[1] = '\n';

		write(fd_out, &header, sizeof(FileHeader));
		
		ssize_t bytes_read;
		while ((bytes_read = read(fd_in, data_buffer, sizeof(data_buffer))) > 0) {
			write(fd_out, data_buffer, bytes_read);
		}

		close(fd_in);
		
		if (bytes_read < 0) {
			perror("Error reading input file");
			close(fd_out);
			return 1;
		}
	}
	close(fd_out);
	return 0;
}


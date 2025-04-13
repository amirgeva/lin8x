import os
import sys


def prompt():
	sys.stdout.write(f'\x1b[34m{os.getcwd()} \x1b[0m')

def input_command():
	cmd = input()
	return cmd


def main():
	cmd = ''
	while cmd!='exit':
		prompt()
		cmd = input_command()
		if cmd is None:
			cmd=''
		else:
			os.system(cmd)

if __name__=='__main__':
	main()


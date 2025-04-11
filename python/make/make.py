import os
import re
import sys

path_pattern = r'[a-zA-Z0-9\./_]+'
space_pattern = re.compile(r'\s+')
target_pattern = re.compile(rf'({path_pattern}):(\s+{path_pattern})*')


class Target:
    def __init__(self, path, deps, commands):
        self.path = path
        self.deps = deps
        self.commands = commands

    def __repr__(self):
        cmd_str = "\n".join(self.commands)
        return f'{self.path}: {self.deps}\n{cmd_str}'


main_target = ''
targets = {}


def read_makefile(filename):
    global main_target
    target = ''
    dependencies = []
    commands = []
    for line in open(filename).readlines():
        m = re.match(target_pattern, line)
        if m:
            if len(target) > 0:
                targets[target] = Target(target, dependencies, commands)
                commands = []
            target = m.group(1)
            dependencies = space_pattern.split(m.group(0))[1:]
            if len(main_target) == 0:
                main_target = target
        elif line.startswith('\t'):
            command = line[1:]
            commands.append(command)
    if len(target) > 0:
        targets[target] = Target(target, dependencies, commands)


def get_target_time(target):
    try:
        return os.stat(target)[8]
    except OSError:
        return 0


def make_target(target):
    print(f'Making {target}')
    if target not in targets:
        return get_target_time(target) > 0
    target = targets[target]
    target_ts = get_target_time(target.path)
    max_dep_ts=0
    for dep in target.deps:
        ts = get_target_time(dep)
        max_dep_ts=max(max_dep_ts,ts)
        if ts == 0 or ts > target_ts:
            if not make_target(dep):
                return False
    if target_ts!=0 and target_ts>=max_dep_ts:
        print(f"{target.path} Up to date")
        return True
    for command in target.commands:
        print(command)
        rc = os.system(command)
        if rc != 0:
            return False
    return True


def main():
    global main_target
    read_makefile('Makefile')
    if len(sys.argv) > 1:
        main_target = sys.argv[1]
    make_target(main_target)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import re

word_pattern = re.compile(r'\W+')


def is_executable(directory, c_files):
    for c_file in c_files:
        file_path = directory+'/'+c_file
        for line in open(file_path).readlines():
            words = word_pattern.split(line)
            if 'main' in words and 'argc' in words and 'argv' in words:
                return True
    return False


def generate_rules(directory, c_files):
    lines = []
    name = directory.split('/')[-1]
    directory = directory[2:].replace('\\', '/')
    c_paths = [f'{directory}/{f}' for f in c_files]
    o_paths = [f.replace('.c', '.o') for f in c_paths]
    objects = " ".join(o_paths)
    target = ''
    if is_executable(directory, c_files):
        deps=[]
        try:
            deps=open(f'{directory}/libs.cfg').readline()
            deps=deps.split()
            deps=['-l'+d for d in deps]
        except OSError:
            pass
        target = f'bin/{name}'
        lines.append(f'bin/{name}: {objects}')
        deps_str=' '.join(deps)
        lines.append(f'\tbin/mold -o bin/{name} {objects} -Llib -Llib/musl -lc lib/musl/crt1.o {deps_str}')
    else:
        target = f'lib/lib{name}.a'
        lines.append(f'lib/lib{name}.a: {objects}')
        lines.append(f'\tbin/lib lib/lib{name}.a {objects}')
    lines.append('')
    for c_path, o_path in zip(c_paths, o_paths):
        lines.append(f'{o_path}: {c_path}')
        lines.append(f'\tbin/lacc -c -Iinclude -Iinclude/musl -o {o_path} {c_path}')
        lines.append('')
    return target, lines


class walk:
    def __init__(self, root):
        self.queue = [root]

    def __iter__(self):
        return self

    def __next__(self):
        return self.next()

    def next(self):
        if len(self.queue) == 0:
            raise StopIteration()
        directory = self.queue[0]
        del self.queue[0]
        files = []
        for name in os.listdir(directory):
            s = os.stat(directory + '/' + name)[0]
            if s > 0x8000:
                files.append(name)
            else:
                self.queue.append(directory + '/' + name)
        return directory, files


def main():
    all_lines = []
    all_targets = []
    for name, files in walk('.'):
        c_files = [f for f in files if f.endswith('.c')]
        if len(c_files) > 0:
            target, lines = generate_rules(name, c_files)
            all_lines.extend(lines)
            all_targets.append(target)
    with open('Makefile', 'w') as f:
        f.write('all: ')
        f.write(' '.join(all_targets))
        f.write('\n\n')
        for line in all_lines:
            f.write(line)
            f.write('\n')
        f.write('\n')
        f.write('clean:\n')
        for target in all_targets:
            f.write(f'\trm -f {target}\n')


if __name__ == '__main__':
    main()

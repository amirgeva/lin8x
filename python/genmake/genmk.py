#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import re

word_pattern = re.compile(r'\W+')
global_cflags=[]
global_lflags=[]
compiler='bin/lacc'
linker='bin/mold'
lflags='-Llib/musl -lc lib/musl/crt1.o'
post_lib=''


def load_global_cflags():
    try:
        with open('cflags.cfg') as f:
            for line in f.readlines():
                line = line.strip()
                if line and not line.startswith('#'):
                    global_cflags.append(line)
    except OSError:
        pass
    if '-DDEV' in global_cflags:
        global_cflags.append('-g')
        global compiler, linker, lflags, post_lib
        compiler='gcc'
        linker='gcc'
        lflags='-g -lm'
        post_lib='ranlib'
    else:
        global_cflags.append('-Iinclude/musl')


def is_executable(directory, c_files):
    for c_file in c_files:
        file_path = directory+'/'+c_file
        for line in open(file_path).readlines():
            words = word_pattern.split(line)
            if 'main' in words and 'argc' in words and 'argv' in words:
                return True
    return False

def recursive_deps(deps):
    new_count=1
    while new_count > 0:
        new_count = 0
        for dep in deps:
            try:
                with open(f'src/{dep}/libs.cfg') as f:
                    cands = f.readline().split()
                    for cand in cands:
                        if cand not in deps:
                            deps.append(cand)
                            new_count = 1
            except OSError:
                pass

def generate_rules(directory, c_files):
    lines = []
    name = directory.split('/')[-1]
    directory = directory[2:].replace('\\', '/')
    c_paths = [f'{directory}/{f}' for f in c_files]
    o_paths = [f.replace('.c', '.o') for f in c_paths]
    objects = " ".join(o_paths)
    target = ''
    c_flags = ' '.join(global_cflags)
    if is_executable(directory, c_files):
        deps=[]
        lib_paths=""
        try:
            deps=open(f'{directory}/libs.cfg').readline()
            deps=deps.split()
            recursive_deps(deps)
            lib_paths = " ".join([f'lib/lib{d}.a' for d in deps])
            deps=['-l'+d for d in deps]
        except OSError:
            pass
        target = f'bin/{name}'
        lines.append(f'bin/{name}: {objects} {lib_paths}')
        deps_str=' '.join(deps)
        lines.append(f'\t{linker} -o bin/{name} {objects} -Llib {deps_str} {lflags}')
    else:
        target = f'lib/lib{name}.a'
        lines.append(f'lib/lib{name}.a: {objects}')
        lines.append(f'\tbin/lib lib/lib{name}.a {objects}')
        if post_lib:
            lines.append(f'\t{post_lib} lib/lib{name}.a')
    lines.append('')
    for c_path, o_path in zip(c_paths, o_paths):
        lines.append(f'{o_path}: {c_path}')
        lines.append(f'\t{compiler} -c {c_flags} -Iinclude -o {o_path} {c_path}')
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
    load_global_cflags()
    all_lines = []
    all_targets = ['bin/lib']
    source_directories = ['src', 'unit_tests']
    for start_dir in source_directories:
        for name, files in walk(f'./{start_dir}'):
            c_files = [f for f in files if f.endswith('.c')]
            if len(c_files) > 0:
                target, lines = generate_rules(name, c_files)
                all_lines.extend(lines)
                if target not in all_targets:
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
        for start_dir in source_directories:
            f.write(f'\tfind {start_dir} -name "*.o" -delete\n')


if __name__ == '__main__':
    main()

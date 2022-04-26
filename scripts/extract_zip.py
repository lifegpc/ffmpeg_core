from argparse import ArgumentParser, Namespace
from sys import exit as _exit
from traceback import print_exc
from zipfile import ZipFile
from os import makedirs, remove
from os.path import join, relpath, dirname, exists
from typing import BinaryIO


def extract(f: BinaryIO, dest):
    if exists(dest):
        remove(dest)
    with open(dest, 'wb') as g:
        t = f.read(4096)
        while len(t) > 0:
            g.write(t)
            t = f.read(4096)


def extract_file(arg: Namespace):
    with ZipFile(arg.FILE, 'r') as f:
        li = f.namelist()
        bin_dir = join(arg.OUTPUT, 'bin')
        include_dir = join(arg.OUTPUT, 'include')
        lib_dir = join(arg.OUTPUT, 'lib')
        for i in li:
            if i.startswith('bin/'):
                ll = i.split('/')
                if len(ll) > 1 and ll[1] == arg.ARCH:
                    dest = join(bin_dir, relpath(i, f'bin/{arg.ARCH}'))
                    makedirs(dirname(dest), exist_ok=True)
                    print(f'Extracting {i} to {dest}')
                    with f.open(i) as t:
                        extract(t, dest)
            if i.startswith('include/'):
                dest = join(include_dir, relpath(i, 'include/'))
                makedirs(dirname(dest), exist_ok=True)
                print(f'Extracting {i} to {dest}')
                with f.open(i) as t:
                    extract(t, dest)
            if i.startswith('lib/'):
                ll = i.split('/')
                if len(ll) > 1 and ll[1] == arg.ARCH:
                    dest = join(lib_dir, relpath(i, f'lib/{arg.ARCH}'))
                    makedirs(dirname(dest), exist_ok=True)
                    print(f'Extracting {i} to {dest}')
                    with f.open(i) as t:
                        extract(t, dest)


try:
    p = ArgumentParser(description='Extact resources.')
    p.add_argument('FILE', help='The file to extract.')
    p.add_argument('ARCH', help='The ARCH type.')
    p.add_argument('OUTPUT', help='The output directory.')
    arg = p.parse_intermixed_args()
    extract_file(arg)
except Exception:
    print_exc()
    _exit(1)

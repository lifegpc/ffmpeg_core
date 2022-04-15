from argparse import ArgumentParser
from os import system, devnull, environ, remove
from typing import List
from sys import exit
from subprocess import Popen, PIPE
from re import search, IGNORECASE
from os.path import splitext, exists, abspath
from tempfile import NamedTemporaryFile


def add_path_ext(path: str) -> str:
    p, n = splitext(path)
    if n != '':
        return path
    else:
        pext = environ['PATHEXT']
        pextl = pext.split(';')
        for ext in pextl:
            if ext == '':
                continue
            if exists(p + ext):
                return p + ext
    return path


def check_pdb(path: str) -> str:
    p = splitext(path)[0] + '.pdb'
    if exists(p):
        return p


def check_needed_prog():
    if system(f'ldd-rust --help > {devnull}'):
        return False
    if system(f'7z --help > {devnull}'):
        return False
    return True


def check_prog(prog: str) -> List[str]:
    r = Popen(f'ldd-rust {prog}', stdout=PIPE, stderr=PIPE)
    out: bytes = r.communicate()[0]
    r.wait()
    out += r.communicate()[0]
    if not r.returncode:
        sl = out.splitlines(False)
        rl = []
        for r in sl:
            r = r.decode()
            rs = search(r'=?-?> (.+) ?(\(0x[0-9a-f]+\))?$', r, IGNORECASE)
            if rs is not None:
                rl.append(abspath(rs.groups()[0]))
            else:
                raise ValueError(f'Can not find path for {r}.')
        return rl
    return None


def getUnixPath(path: str) -> str:
    rs = search(r'^[A-Z]:', path, IGNORECASE)
    if rs is None:
        return path.replace('\\', '/')
    return '/' + path[0].lower() + path[2:].replace('\\', '/')


def getWindowsPath(path: str) -> str:
    rs = search(r'^[\\/][A-Z][\\/]', path, IGNORECASE)
    if rs is None:
        return path.replace('/', '\\')
    return path[1].upper() + ":" + path[2:].replace('/', '\\')


def print_help():
    print('''pack-prog.py [-o <output_name>] [programs]''')


class Prog:
    def __init__(self):
        self._loc = []

    def add_dep(self, path: str):
        path_w = getWindowsPath(path)
        if path_w.upper().startswith('C:\\WINDOWS'):
            return
        if path_w not in self._loc:
            print(f'add dependence: "{path_w}"')
            self._loc.append(path_w)

    def add_prog(self, path: str):
        pro = add_path_ext(path)
        pro_w = getWindowsPath(pro)
        if pro_w not in self._loc:
            print(f'add program: "{pro_w}"')
            self._loc.append(pro_w)

    def add_file(self, path: str):
        p = getWindowsPath(path)
        if p not in self._loc:
            print(f'add file: "{p}"')
            self._loc.append(p)

    def to_7z(self, output: str):
        p = NamedTemporaryFile(delete=False)
        for i in self._loc:
            p.write((i + '\n').encode('UTF8'))
        fp = p.name
        p.close()
        try:
            system(f'7z a -mx9 -y {output} @{fp}')
        except Exception:
            remove(fp)
            from traceback import print_exc
            print_exc()
        remove(fp)


def main(prog: List[str], output: str = None, adds: List[str] = None,
         pdbs: List[str] = None):
    if output is None:
        output = 'programs.7z'
    if not check_needed_prog():
        print('ldd and 7z is needed.')
    p = Prog()
    for pro in prog:
        pro = abspath(pro)
        pro = add_path_ext(pro)
        # pro_u = getUnixPath(pro)
        rel = check_prog(pro)
        if rel is None:
            print(f'Can not get dependencies for {pro},')
            exit(-1)
        p.add_prog(pro)
        for i in rel:
            p.add_dep(i)
    if adds is not None:
        for f in adds:
            p.add_file(f)
    if pdbs:
        for i in pdbs:
            pro = abspath(i)
            pro = add_path_ext(pro)
            rel = check_prog(pro)
            if rel is None:
                print(f'Can not get dependencies for {pro},')
                exit(-1)
            fn = check_pdb(pro)
            if fn:
                p.add_file(fn)
            for i in rel:
                fn = check_pdb(i)
                if fn:
                    p.add_file(fn)
    p.to_7z(output)


p = ArgumentParser(description='Pack programs into a 7-zip file.')
p.add_argument('PROG', action='append', help='Program to pack', nargs='*', default=[])
p.add_argument('-p', '--pdb', action='append', help='Program\'s PDB file to pack', nargs='*', default=[], dest='pdb')
p.add_argument('-o', '--output', help='Output file', default='programs.7z', dest='output')
p.add_argument('-a', '--add', action='append', help='Additional files to pack', nargs='*', default=[], dest='add')


if __name__ == '__main__':
    try:
        args = p.parse_args()
        main(args.PROG[0], args.output, args.add, args.pdb)
    except Exception:
        from traceback import print_exc
        from sys import exit
        print_exc()
        exit(-1)

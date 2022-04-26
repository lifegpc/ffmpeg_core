from argparse import ArgumentParser
from hashlib import sha256 as _sha256
from os.path import exists
import sys
from time import time, strftime, gmtime
from typing import List


def sha256(data) -> str:
    if isinstance(data, str):
        data = data.encode()
    elif not isinstance(data, bytes):
        data = str(data).encode()
    s = _sha256()
    s.update(data)
    return s.hexdigest()


def hash_file(type, feature) -> str:
    fnl = []
    fn = f"build_{feature}_{type}.sh"
    if exists(fn):
        fnl.append(fn)
    fn = f'build_{feature}_{type}.bat'
    if exists(fn):
        fnl.append(fn)
    fn = f'download_{feature}_{type}.bat'
    if exists(fn):
        fnl.append(fn)
    if len(fnl) == 0:
        return ''
    s = _sha256()
    for fn in fnl:
        with open(fn, 'rb') as f:
            c = f.read(256)
            while len(c) > 0:
                s.update(c)
                c = f.read(256)
    return s.hexdigest()


try:
    p = ArgumentParser(description='Get the cache key which used in action/cache')
    p.add_argument('type', help='The build type')
    p.add_argument('features', help="The feature's name", action='append', nargs='+')
    args = p.parse_intermixed_args(sys.argv[1:])
    features: List[str] = args.features[0]
    d = ''
    now = time()
    for i in features:
        dt = strftime('%Y-%m', gmtime(now))
        h = hash_file(args.type, i)
        if len(h) > 0:
            d += f"{i}={dt}:{h}\n"
    print(d)
    print(f"::set-output name=cache_key::{sha256(d)}")
except Exception:
    from traceback import print_exc
    from sys import exit
    print_exc()
    exit(1)

from argparse import ArgumentParser, Namespace
from sys import exit as _exit
from traceback import print_exc
from urllib.parse import urlparse
from os.path import join
try:
    from requests import Session
except ImportError:
    _exit(2)
except Exception:
    print_exc()
    _exit(1)


def download_file(arg: Namespace):
    s = Session()
    fn = urlparse(arg.URL).path.split('/')[-1]
    output = fn
    if arg.output:
        output = arg.output
    elif arg.dest:
        output = join(arg.dest, fn)
    print(f'Downloading {arg.URL} to {output}')
    r = s.get(arg.URL, stream=True)
    if r.status_code >= 400:
        raise Exception(f'Failed to download {arg.URL}')
    with open(output, 'wb') as f:
        for chunk in r.iter_content(chunk_size=1024):
            if chunk:
                f.write(chunk)


try:
    p = ArgumentParser(description='Download resources from github.')
    p.add_argument('URL', help='The URL of the resource.')
    p.add_argument('-o', '--output', help='The full path of output file.')
    p.add_argument('-d', '--dest', help='The destination directory.')
    arg = p.parse_intermixed_args()
    download_file(arg)
except Exception:
    print_exc()
    _exit(1)

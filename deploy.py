import os
import os.path
import shutil
import subprocess
from shlex import split

def compile():
  if os.path.isdir("./out"):
    shutil.rmtree("./out")

  for path, subdirs, files in os.walk("./firmware"):
      for f in files:
          if f.endswith(".py") and f != "main.py":
            fpath = path + "/" + f
            out_fpath = "./out" + fpath[1:-3].replace("/firmware", "") + ".mpy"
            print(out_fpath)
            out_dir = os.path.dirname(out_fpath)
            if not os.path.isdir(out_dir):
                os.makedirs(out_dir)
            cmd = "mpy-cross -v -march=armv6m -s %s %s -o %s" % (
                fpath[1:],
                fpath,
                out_fpath,
            )
            # print(cmd)
            res = os.system(cmd)
            assert res == 0


def find_files(port, root):
  result = subprocess.run(split(f"mpremote {port} fs ls {root}"), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  if result.returncode != 0:
    exit(1)
  lines = result.stdout.decode('utf-8').split('\n')
  paths = [root + f.split(' ', 1)[1] for f in list(filter(lambda l: len(l) > 0 and not l.startswith('ls'), [x.strip() for x in lines]))]
  folders = list(filter(lambda f: f.endswith('/'), paths))
  
  nested_folders = [find_files(port, f) for f in folders]

  return paths + [item for sublist in nested_folders for item in sublist]

def clean(port):
  paths = sorted(find_files(port, '/'), key=len, reverse=True)

  for path in paths:
    print(f"Removing '{path}'")
    result = subprocess.run(split(f"mpremote {port} fs rm {path}"), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
      exit(1)

def deploy(port):
  if os.path.isdir("./out"):
    shutil.rmtree("./out")
  for path, subdirs, files in os.walk("./firmware"):
      for f in files:
        fpath = path + "/" + f
        if fpath.endswith(".py"):
            out_fpath = "./out" + fpath[1:-3].replace("/firmware", "") + ".py"
            out_dir = os.path.dirname(out_fpath)
            if not os.path.isdir(out_dir):
              os.makedirs(out_dir)
            shutil.copy(fpath, out_fpath)
            print(out_fpath)
  result = subprocess.run(split(f"mpremote {port} cp -r . :"), cwd="./out", stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  if result.returncode != 0:
    print(f"Failed upload: {result.stdout}")
    exit(1)          
  # cwd

#clean("a2")
#deploy("a2")
compile()
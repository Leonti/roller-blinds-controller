import os
import os.path
import shutil

if os.path.isdir("out"):
  shutil.rmtree("out")

for path, subdirs, files in os.walk("."):
    for f in files:
        if f.endswith(".py") and f != "deploy.py" and f != "main.py":
          fpath = path + "/" + f
          out_fpath = "./out" + fpath[1:-3] + ".mpy"
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
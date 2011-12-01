import os, sys, re
import shlex, subprocess

def create_random_file(out_filename, megabytes):
    if(not os.path.isfile(out_filename)):
        os.system("dd if=/dev/urandom of=%s bs=%dM count=2" % (out_filename, megabytes))

def compress_file(filename, megabytes, fn = "temp"):
    process_list = list()
    os.system("(rm -rf %s && mkdir %s && cd %s && split -b%dm ../%s part)" % (fn, fn, fn, megabytes, filename))
    for fl in os.listdir("./%s" % fn):
        if not fl.startswith("part"): continue
        args = shlex.split("gzip %s/%s &" % (fn, fl))
        process_list.append(subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.PIPE))
    for item in process_list: item.wait()
    for fl in os.listdir(fn):
        if(not fl.endswith(".gz")): os.remove(fl)

def decompress_file(filename, fn = "temp"):
    process_list = list()    
    for fl in os.listdir(fn):
        if(not fl.endswith(".gz")): continue
        args = shlex.split("gzip -d %s/%s &" % (fn, fl))
        process_list.append(subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.PIPE))
    for item in process_list: item.wait()
    cmd = " ".join(sorted([(fn + "/" + f) for f in os.listdir(fn) if f.startswith("part")])) 
    os.system("cat %s > %s" % (cmd, filename))
    os.system("rm -rf %s" % (fn)) 





in_filename = "a.log"
out_filename = "a.test"
chunk_size = 30

create_random_file(in_filename, 500) 
#compress_file(in_filename, chunk_size)
#decompress_file(out_filename)

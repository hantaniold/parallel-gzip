import os, sys, re
import shlex, subprocess

def compress_file(filename, megabytes, fn = "temp", gzip_exec = "gzip"):
    process_list = list()
    os.system("(rm -rf %s && mkdir %s && cd %s && split -b%dm ../%s part)" % (fn, fn, fn, megabytes, filename))
    for fl in os.listdir("./%s" % fn):
        if not fl.startswith("part"): continue
        args = shlex.split("%s %s/%s &" % (gzip_exec, fn, fl))
        process_list.append(subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.PIPE))
    for item in process_list: item.wait()
    for fl in os.listdir(fn):
        if(not fl.endswith(".gz")): os.remove(fl)

def decompress_file(filename, fn = "temp", gzip_exec = "gzip"):
    process_list = list()    
    for fl in os.listdir(fn):
        if(not fl.endswith(".gz")): continue
        args = shlex.split("%s -d %s/%s &" % (gzip_exec, fn, fl))
        process_list.append(subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.PIPE))
    for item in process_list: item.wait()
    cmd = " ".join(sorted([(fn + "/" + f) for f in os.listdir(fn) if f.startswith("part")])) 
    os.system("cat %s > %s" % (cmd, filename))
    os.system("rm -rf %s" % (fn)) 



# Basic usage

#in_filename = "a.log"
#out_filename = "a.test"
#chunk_size = 30

#create_random_file(in_filename, 500) 
#compress_file(in_filename, chunk_size)
#decompress_file(out_filename)

if __name__ == "__main__":

    gzip_exec = "gzip"
    if(os.path.isfile("../gzip/gzip")):
         gzip_exec = "../gzip/gzip"

    else:
        if(os.path.isfile("../gzip/init_script.sh")):
            os.system("(cd ../gzip && chmod u+x init_script.sh && ./init_script.sh)")

        if(os.path.isfile("../gzip/gzip")):
            gzip_exec = "../gzip/gzip"

    if(len(sys.argv) < 2):
        print "First Argument Must be File to Compress"
        sys.exit(1)
    if(not os.path.isfile(sys.argv[1])):
        print "First Argument is Not Valid File Name"
        sys.exit(1)


    default_chunk_size = 100
    compress_file(sys.argv[1], default_chunk_sizei, gzip_exec = gzip_exec)

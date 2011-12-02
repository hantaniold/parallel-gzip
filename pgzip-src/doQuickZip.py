import quickZip
import sys

if len(sys.argv) < 3:
	print "Usage: python doQuickZip.py <filename> <chunksize>"
	exit()
if sys.argv[1] == "-d":
	quickZip.decompress_file(sys.argv[2])
else:
	quickZip.compress_file(sys.argv[1],int(sys.argv[2]))

